#include "OllamaProvider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/CookieImporter.h"
#include <QRegularExpression>
#include <algorithm>

OllamaProvider::OllamaProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> OllamaProvider::createStrategies(const ProviderFetchContext& ctx) {
    return { new OllamaWebStrategy(const_cast<OllamaProvider*>(this)) };
}

OllamaWebStrategy::OllamaWebStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool OllamaWebStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty()) {
        return true;
    }
    return CookieImporter::hasUsableProfileData(CookieImporter::Chrome) ||
           CookieImporter::hasUsableProfileData(CookieImporter::Edge);
}

static QString buildOllamaCookieHeader() {
    for (auto browser : CookieImporter::importOrder()) {
        if (!CookieImporter::isBrowserInstalled(browser)) continue;
        auto cookies = CookieImporter::importCookies(browser, {"ollama.com", "www.ollama.com"});
        if (cookies.isEmpty()) continue;
        QStringList parts;
        for (auto& c : cookies) parts.append(c.name() + "=" + c.value());
        return parts.join("; ");
    }
    return {};
}

ProviderFetchResult OllamaWebStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = "ollama.web";
    result.strategyKind = 1;
    result.sourceLabel = "web";

    QString cookie = ctx.manualCookieHeader.value_or(QString());
    if (cookie.isEmpty()) {
        cookie = buildOllamaCookieHeader();
    }
    if (cookie.isEmpty()) { result.errorMessage = "no cookies found"; return result; }

        QHash<QString,QString> headers;
        headers["Cookie"] = cookie;
        headers["Accept"] = "text/html";
        headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Chrome/143.0.0.0 Safari/537.36";
        headers["Referer"] = "https://ollama.com/settings";
        headers["Origin"] = "https://ollama.com";

        QString html = NetworkManager::instance().getStringSync(QUrl("https://ollama.com/settings"), headers, ctx.networkTimeoutMs);
        if (html.isEmpty()) { result.errorMessage = "empty HTML"; return result; }

        static QRegularExpression planRe("Cloud Usage\\s*</span>\\s*<span[^>]*>([^<]+)</span>");
        static QRegularExpression emailRe("id=\"header-email\"[^>]*>([^<]+)<");
        static QRegularExpression pctRe("([0-9]+(?:\\.[0-9]+)?)\\s*%\\s*used", QRegularExpression::CaseInsensitiveOption);
        static QRegularExpression widthRe("width:\\s*([0-9]+(?:\\.[0-9]+)?)%");

        QString plan, email;
        auto pm = planRe.match(html);
        if (pm.hasMatch()) plan = pm.captured(1).trimmed();
        auto em = emailRe.match(html);
        if (em.hasMatch() && em.captured(1).contains('@')) email = em.captured(1).trimmed();

        double sessionPct = 0, weeklyPct = 0;
        QStringList lines = html.split('\n');
        int sessionIdx = -1, weeklyIdx = -1;
        for (int i = 0; i < lines.size(); i++) {
            if (lines[i].contains("Hourly usage", Qt::CaseInsensitive) ||
                lines[i].contains("Session usage", Qt::CaseInsensitive)) sessionIdx = i;
            if (lines[i].contains("Weekly usage", Qt::CaseInsensitive)) weeklyIdx = i;
        }

        for (int i = sessionIdx; i >= 0 && i < lines.size() && i < sessionIdx + 5; i++) {
            auto m = pctRe.match(lines[i]);
            if (m.hasMatch()) { sessionPct = m.captured(1).toDouble(); break; }
        }
        if (sessionIdx >= 0 && sessionPct == 0) {
            for (int i = sessionIdx; i < std::min<int>(sessionIdx + 5, lines.size()); i++) {
                auto m = widthRe.match(lines[i]);
                if (m.hasMatch()) { sessionPct = m.captured(1).toDouble(); break; }
            }
        }
        for (int i = weeklyIdx; i >= 0 && i < lines.size() && i < weeklyIdx + 5; i++) {
            auto m = pctRe.match(lines[i]);
            if (m.hasMatch()) { weeklyPct = m.captured(1).toDouble(); break; }
        }

        UsageSnapshot snap;
        snap.updatedAt = QDateTime::currentDateTime();

        if (sessionPct > 0 || weeklyPct > 0) {
            snap.primary = RateWindow{std::clamp(sessionPct, 0.0, 100.0), std::nullopt, std::nullopt, QString(), std::nullopt};
            snap.secondary = RateWindow{std::clamp(weeklyPct, 0.0, 100.0), std::nullopt, std::nullopt, QString(), std::nullopt};
        }

        ProviderIdentitySnapshot id;
        id.providerID = UsageProvider::ollama;
        if (!plan.isEmpty()) id.loginMethod = plan;
        if (!email.isEmpty()) id.accountEmail = email;
        snap.identity = id;

        result.usage = snap;
        result.success = true;
        return result;
}
