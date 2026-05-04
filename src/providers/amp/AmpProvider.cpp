#include "AmpProvider.h"
#include "../../network/NetworkManager.h"
#include "../shared/CookieImporter.h"
#include "../shared/BrowserDetection.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

AmpProvider::AmpProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> AmpProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new AmpWebStrategy(this) };
}

AmpWebStrategy::AmpWebStrategy(QObject* parent) : IFetchStrategy(parent) {}

QString AmpWebStrategy::resolveCookieHeader(const ProviderFetchContext& ctx) {
    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty())
        return *ctx.manualCookieHeader;

    QString manual = ctx.settings.get("manualCookieHeader").toString().trimmed();
    if (!manual.isEmpty()) return manual;

    QStringList domains = {"ampcode.com", "www.ampcode.com"};
    auto browsers = BrowserDetection::installedBrowsers();
    for (auto browser : browsers) {
        auto cookies = CookieImporter::importCookies(browser, domains);
        if (cookies.isEmpty()) continue;
        QString header;
        for (const auto& cookie : cookies) {
            if (!header.isEmpty()) header += "; ";
            header += cookie.name() + "=" + cookie.value();
        }
        if (!header.isEmpty()) return header;
    }
    return {};
}

bool AmpWebStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !resolveCookieHeader(ctx).isEmpty();
}

bool AmpWebStrategy::shouldFallback(const ProviderFetchResult& result,
                                     const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

ProviderFetchResult AmpWebStrategy::parseHTML(const QString& html) {
    ProviderFetchResult result;
    result.strategyID = "amp.web";
    result.strategyKind = ProviderFetchKind::Web;
    result.sourceLabel = "web";

    if (html.isEmpty()) {
        result.success = false;
        result.errorMessage = "Empty response from Amp";
        return result;
    }

    if (html.contains("/auth/sign-in") || html.contains("/login") || html.contains("/signin")) {
        result.success = false;
        result.errorMessage = "Not signed in. Log in to ampcode.com in your browser.";
        return result;
    }

    QRegularExpression re("freeTierUsage\\s*[:=]\\s*\\{([^}]+)\\}");
    QRegularExpressionMatch match = re.match(html);

    if (!match.hasMatch()) {
        QRegularExpression re2("getFreeTierUsage\\s*\\(\\s*\\)\\s*\\{[^}]*return\\s*\\{([^}]+)\\}");
        match = re2.match(html);
    }

    if (!match.hasMatch()) {
        result.success = false;
        result.errorMessage = "Could not parse Amp usage data from page";
        return result;
    }

    QString block = match.captured(1);

    QRegularExpression quotaRe("quota\\s*[:=]\\s*([\\d.]+)");
    QRegularExpression usedRe("used\\s*[:=]\\s*([\\d.]+)");
    QRegularExpression windowRe("windowHours\\s*[:=]\\s*([\\d.]+)");

    auto quotaMatch = quotaRe.match(block);
    auto usedMatch = usedRe.match(block);
    auto windowMatch = windowRe.match(block);

    if (!quotaMatch.hasMatch()) {
        result.success = false;
        result.errorMessage = "Could not find quota in Amp usage data";
        return result;
    }

    double quota = quotaMatch.captured(1).toDouble();
    double used = usedMatch.hasMatch() ? usedMatch.captured(1).toDouble() : 0;
    double windowHours = windowMatch.hasMatch() ? windowMatch.captured(1).toDouble() : 0;

    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::amp;
    snap.identity = identity;

    RateWindow rw;
    rw.usedPercent = (quota > 0) ? (used / quota * 100.0) : 0;
    if (windowHours > 0) rw.windowMinutes = static_cast<int>(windowHours * 60);
    rw.resetDescription = QString("%1 / %2").arg(used, 0, 'f', 1).arg(quota, 0, 'f', 1);
    snap.primary = rw;

    result.usage = snap;
    result.success = true;
    return result;
}

ProviderFetchResult AmpWebStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "web";

    QString cookieHeader = resolveCookieHeader(ctx);
    if (cookieHeader.isEmpty()) {
        result.success = false;
        result.errorMessage = "No Amp cookie available. Log in to ampcode.com in your browser.";
        return result;
    }

    QHash<QString, QString> headers;
    headers["Cookie"] = cookieHeader;
    headers["Accept"] = "text/html,application/xhtml+xml";

    QString html = NetworkManager::instance().getStringSync(
        QUrl("https://ampcode.com/settings"), headers, ctx.networkTimeoutMs);

    return parseHTML(html);
}
