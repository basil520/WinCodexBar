#include "GeminiProvider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/ProviderCredentialStore.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QProcessEnvironment>

GeminiProvider::GeminiProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> GeminiProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new GeminiOAuthStrategy(this), new GeminiAPIStrategy(this) };
}

// OAuth Strategy
GeminiOAuthStrategy::GeminiOAuthStrategy(QObject* parent) : IFetchStrategy(parent) {}

std::optional<GeminiCredentials> GeminiOAuthStrategy::loadCredentials() {
    QString path = QDir::homePath() + "/.gemini/oauth_creds.json";
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) return std::nullopt;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return std::nullopt;

    QJsonObject obj = doc.object();
    GeminiCredentials creds;
    creds.accessToken = obj["access_token"].toString();
    creds.refreshToken = obj["refresh_token"].toString();
    creds.clientId = obj["client_id"].toString();
    creds.clientSecret = obj["client_secret"].toString();

    if (creds.accessToken.isEmpty() && creds.refreshToken.isEmpty()) return std::nullopt;
    return creds;
}

std::optional<QString> GeminiOAuthStrategy::refreshAccessToken(const GeminiCredentials& creds) {
    if (creds.refreshToken.isEmpty() || creds.clientId.isEmpty()) return std::nullopt;

    QHash<QString, QString> headers;
    headers["Content-Type"] = "application/x-www-form-urlencoded";

    QUrlQuery params;
    params.addQueryItem("client_id", creds.clientId);
    if (!creds.clientSecret.isEmpty())
        params.addQueryItem("client_secret", creds.clientSecret);
    params.addQueryItem("refresh_token", creds.refreshToken);
    params.addQueryItem("grant_type", "refresh_token");

    QJsonObject json = NetworkManager::instance().postFormSync(
        QUrl("https://oauth2.googleapis.com/token"),
        params.toString().toUtf8(), headers, 15000);

    QString token = json["access_token"].toString();
    return token.isEmpty() ? std::nullopt : std::make_optional(token);
}

bool GeminiOAuthStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return loadCredentials().has_value();
}

bool GeminiOAuthStrategy::shouldFallback(const ProviderFetchResult& result,
                                          const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

ProviderFetchResult GeminiOAuthStrategy::parseResponse(const QJsonObject& json) {
    ProviderFetchResult result;
    result.strategyID = "gemini.oauth";
    result.strategyKind = ProviderFetchKind::OAuth;
    result.sourceLabel = "oauth";

    QJsonArray buckets = json["buckets"].toArray();
    if (buckets.isEmpty()) {
        result.success = false;
        result.errorMessage = "No quota buckets in Gemini response";
        return result;
    }

    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::gemini;
    snap.identity = identity;

    for (const auto& b : buckets) {
        QJsonObject bucket = b.toObject();
        double remaining = bucket["remainingFraction"].toDouble(1.0);
        QString modelId = bucket["modelId"].toString();
        QString resetTime = bucket["resetTime"].toString();

        RateWindow rw;
        rw.usedPercent = 100.0 - (remaining * 100.0);
        if (!resetTime.isEmpty()) {
            rw.resetsAt = QDateTime::fromString(resetTime, Qt::ISODate);
        }
        rw.resetDescription = modelId;

        if (modelId.contains("pro", Qt::CaseInsensitive)) {
            if (!snap.primary.has_value()) snap.primary = rw;
        } else if (modelId.contains("flash-lite", Qt::CaseInsensitive)) {
            if (!snap.tertiary.has_value()) snap.tertiary = rw;
        } else {
            if (!snap.secondary.has_value()) snap.secondary = rw;
        }
    }

    if (!snap.primary.has_value() && snap.secondary.has_value()) {
        snap.primary = snap.secondary;
        snap.secondary.reset();
    }

    result.usage = snap;
    result.success = true;
    return result;
}

ProviderFetchResult GeminiOAuthStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "oauth";

    auto creds = loadCredentials();
    if (!creds.has_value()) {
        result.success = false;
        result.errorMessage = "Gemini OAuth credentials not found. Run 'gemini' CLI to authenticate.";
        return result;
    }

    QString accessToken = creds->accessToken;
    if (accessToken.isEmpty()) {
        auto refreshed = refreshAccessToken(*creds);
        if (!refreshed.has_value()) {
            result.success = false;
            result.errorMessage = "Failed to refresh Gemini OAuth token.";
            return result;
        }
        accessToken = *refreshed;
    }

    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + accessToken;
    headers["Accept"] = "application/json";

    QJsonObject json = NetworkManager::instance().postJsonSync(
        QUrl("https://cloudcode-pa.googleapis.com/v1internal:retrieveUserQuota"),
        QJsonObject(), headers, ctx.networkTimeoutMs);

    return parseResponse(json);
}

// API Key Strategy (fallback)
GeminiAPIStrategy::GeminiAPIStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool GeminiAPIStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return ctx.env.contains("GEMINI_API_KEY") ||
           ProviderCredentialStore::read("com.codexbar.apikey.gemini").has_value();
}

bool GeminiAPIStrategy::shouldFallback(const ProviderFetchResult& result,
                                        const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

ProviderFetchResult GeminiAPIStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "api";

    result.success = false;
    result.errorMessage = "Gemini API key mode does not support usage quotas. Use OAuth mode (run 'gemini' CLI to authenticate).";
    return result;
}
