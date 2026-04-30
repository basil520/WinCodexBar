#include "MistralProvider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/CookieImporter.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QDate>
#include <algorithm>

MistralProvider::MistralProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> MistralProvider::createStrategies(const ProviderFetchContext& ctx) {
    return { new MistralWebStrategy(const_cast<MistralProvider*>(this)) };
}

MistralWebStrategy::MistralWebStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool MistralWebStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty()) {
        return true;
    }
    return CookieImporter::hasUsableProfileData(CookieImporter::Chrome) ||
           CookieImporter::hasUsableProfileData(CookieImporter::Edge);
}

static QString buildCookieHeader() {
    QStringList domains = {"mistral.ai", "admin.mistral.ai", "auth.mistral.ai"};
    for (auto browser : CookieImporter::importOrder()) {
        if (!CookieImporter::isBrowserInstalled(browser)) continue;
        auto cookies = CookieImporter::importCookies(browser, domains);
        if (cookies.isEmpty()) continue;
        QStringList parts;
        for (auto& c : cookies) parts.append(c.name() + "=" + c.value());
        return parts.join("; ");
    }
    return {};
}

ProviderFetchResult MistralWebStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = "mistral.web";
    result.strategyKind = 1;
    result.sourceLabel = "web";

    QString cookieHeader = ctx.manualCookieHeader.value_or(QString());
    if (cookieHeader.isEmpty()) {
        cookieHeader = buildCookieHeader();
    }
    if (cookieHeader.isEmpty()) {
        result.errorMessage = "no cookies found";
        return result;
    }

        QDate now = QDate::currentDate();
        QString url = QString("https://admin.mistral.ai/api/billing/v2/usage?month=%1&year=%2")
            .arg(now.month()).arg(now.year());

        QHash<QString,QString> headers;
        headers["Cookie"] = cookieHeader;
        headers["Accept"] = "application/json";
        headers["Referer"] = "https://admin.mistral.ai/organization/usage";
        headers["Origin"] = "https://admin.mistral.ai";

        QJsonObject json = NetworkManager::instance().getJsonSync(QUrl(url), headers, ctx.networkTimeoutMs);
        if (json.isEmpty()) { result.errorMessage = "empty response"; return result; }

        double totalCost = 0;
        QJsonObject prices;
        auto pricesArr = json["prices"].toArray();
        for (const auto& p : pricesArr) {
            auto po = p.toObject();
            QString key = po["billing_metric"].toString() + "::" + po["billing_group"].toString();
            prices[key] = po["price"];
        }

        auto sumCategory = [&](const QJsonObject& cat) {
            auto models = cat["models"].toObject();
            for (auto mit = models.begin(); mit != models.end(); ++mit) {
                auto model = mit.value().toObject();
                for (const char* group : {"input","output","cached"}) {
                    auto arr = model[group].toArray();
                    for (int ei = 0; ei < arr.size(); ei++) {
                        auto e = arr[ei].toObject();
                        double tokens = e["value_paid"].toDouble(e["value"].toDouble(0));
                        QString pk = e["billing_metric"].toString() + "::" + e["billing_group"].toString();
                        double price = prices[pk].toString().toDouble(0);
                        totalCost += tokens * price;
                    }
                }
            }
        };

        QStringList categories = {"completion","ocr","connectors","audio"};
        for (auto& c : categories) sumCategory(json[c].toObject());

        UsageSnapshot snap;
        snap.updatedAt = QDateTime::currentDateTime();
        RateWindow primary;
        primary.usedPercent = 0;
        QString curr = json["currency_symbol"].toString("€");
        primary.resetDescription = totalCost > 0 ? QString("%1%2 this month").arg(curr).arg(totalCost, 0, 'f', 4)
                                                   : "No usage this month";
        snap.primary = primary;

        result.usage = snap;
        result.success = true;
        return result;
}
