#include "PerplexityProvider.h"
#include "../../network/NetworkManager.h"
#include "../shared/CookieImporter.h"
#include "../shared/BrowserDetection.h"
#include "../shared/ProviderCredentialStore.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

PerplexityProvider::PerplexityProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> PerplexityProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new PerplexityWebStrategy(this) };
}

PerplexityWebStrategy::PerplexityWebStrategy(QObject* parent) : IFetchStrategy(parent) {}

QString PerplexityWebStrategy::resolveCookieHeader(const ProviderFetchContext& ctx) {
    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty())
        return *ctx.manualCookieHeader;

    QString manual = ctx.settings.get("manualCookieHeader").toString().trimmed();
    if (!manual.isEmpty()) return manual;

    QStringList envVars = {"PERPLEXITY_SESSION_TOKEN", "perplexity_session_token", "PERPLEXITY_COOKIE"};
    for (const auto& var : envVars) {
        if (ctx.env.contains(var) && !ctx.env[var].isEmpty())
            return ctx.env[var];
    }

    QStringList domains = {"perplexity.ai", "www.perplexity.ai"};
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

bool PerplexityWebStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !resolveCookieHeader(ctx).isEmpty();
}

bool PerplexityWebStrategy::shouldFallback(const ProviderFetchResult& result,
                                            const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

ProviderFetchResult PerplexityWebStrategy::parseResponse(const QJsonObject& json) {
    ProviderFetchResult result;
    result.strategyID = "perplexity.web";
    result.strategyKind = ProviderFetchKind::Web;
    result.sourceLabel = "web";

    if (json.isEmpty()) {
        result.success = false;
        result.errorMessage = "Empty or invalid response from Perplexity API";
        return result;
    }

    int balanceCents = json["balance_cents"].toInt(0);
    int purchasedCents = json["current_period_purchased_cents"].toInt(0);
    int totalUsageCents = json["total_usage_cents"].toInt(0);
    qint64 renewalTs = static_cast<qint64>(json["renewal_date_ts"].toDouble());

    QJsonArray grants = json["credit_grants"].toArray();
    int recurringTotal = 0, promoTotal = 0;
    for (const auto& g : grants) {
        QJsonObject grant = g.toObject();
        int amount = grant["amount_cents"].toInt(0);
        QString type = grant["type"].toString();
        if (type == "recurring") recurringTotal += amount;
        else if (type == "promotional") promoTotal += amount;
    }

    int promoUsed = std::max(0, promoTotal - std::max(0, balanceCents - purchasedCents - recurringTotal));
    int recurringUsed = std::max(0, recurringTotal - std::max(0, balanceCents - purchasedCents));
    int purchasedUsed = std::max(0, purchasedCents - std::max(0, balanceCents - recurringTotal - promoTotal));

    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::perplexity;
    snap.identity = identity;

    RateWindow primary;
    primary.usedPercent = (recurringTotal > 0) ? (static_cast<double>(recurringUsed) / recurringTotal * 100.0) : 0;
    primary.windowMinutes = 43200;
    if (renewalTs > 0) primary.resetsAt = QDateTime::fromSecsSinceEpoch(renewalTs);
    primary.resetDescription = (recurringTotal < 5000) ? "Pro" : "Max";
    snap.primary = primary;

    RateWindow secondary;
    secondary.usedPercent = (promoTotal > 0) ? (static_cast<double>(promoUsed) / promoTotal * 100.0) : 100;
    secondary.resetDescription = "Promo credits";
    snap.secondary = secondary;

    RateWindow tertiary;
    tertiary.usedPercent = (purchasedCents > 0) ? (static_cast<double>(purchasedUsed) / purchasedCents * 100.0) : 100;
    tertiary.resetDescription = "Purchased credits";
    snap.tertiary = tertiary;

    result.usage = snap;
    result.success = true;
    return result;
}

ProviderFetchResult PerplexityWebStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "web";

    QString cookieHeader = resolveCookieHeader(ctx);
    if (cookieHeader.isEmpty()) {
        result.success = false;
        result.errorMessage = "No Perplexity cookie available. Log in to perplexity.ai in your browser or paste cookie manually.";
        return result;
    }

    QHash<QString, QString> headers;
    headers["Cookie"] = cookieHeader;
    headers["Accept"] = "application/json";
    headers["Origin"] = "https://www.perplexity.ai";
    headers["Referer"] = "https://www.perplexity.ai/account/usage";

    QJsonObject json = NetworkManager::instance().getJsonSync(
        QUrl("https://www.perplexity.ai/rest/billing/credits?version=2.18&source=default"),
        headers, ctx.networkTimeoutMs);

    return parseResponse(json);
}
