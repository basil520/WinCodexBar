#include "AlibabaProvider.h"
#include "../../network/NetworkManager.h"
#include "../../providers/shared/CookieImporter.h"
#include "../../providers/shared/ProviderCredentialStore.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QUrl>

AlibabaProvider::AlibabaProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> AlibabaProvider::createStrategies(const ProviderFetchContext& ctx) {
    QVector<IFetchStrategy*> strategies;
    
    QString mode = ctx.settings.get("sourceMode", "auto").toString();
    
    if (mode == "auto" || mode == "api") {
        strategies.append(new AlibabaAPIStrategy());
    }
    if (mode == "auto" || mode == "web") {
        strategies.append(new AlibabaWebStrategy());
    }
    
    return strategies;
}

QVector<ProviderSettingsDescriptor> AlibabaProvider::settingsDescriptors() const {
    return {
        {"sourceMode", "Data source", "picker", QVariant(QStringLiteral("auto")),
         {{"auto", "Auto"}, {"web", "Web Cookie"}, {"api", "API Key"}}},
        {"apiRegion", "Region", "picker", QVariant(QStringLiteral("international")),
         {{"international", "International"}, {"china", "China (Mainland)"}}},
        {"apiKey", "API Key", "secret", QVariant(),
         {}, "com.codexbar.apikey.alibaba", {}, "sk-...",
         "Stored in Windows Credential Manager", true, true},
        {"manualCookieHeader", "Manual cookie header", "secret", QVariant(),
         {}, "com.codexbar.cookie.alibaba", {}, "cookie=value; ...",
         "Stored in Windows Credential Manager", true, true}
    };
}

// ============================================================================
// AlibabaWebStrategy
// ============================================================================

AlibabaWebStrategy::AlibabaWebStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool AlibabaWebStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty()) {
        return true;
    }
    return CookieImporter::hasUsableProfileData(CookieImporter::Chrome) ||
           CookieImporter::hasUsableProfileData(CookieImporter::Edge);
}

QString AlibabaWebStrategy::buildCookieHeader() {
    QStringList domains = {"aliyun.com", "www.aliyun.com", "alibabacloud.com", "www.alibabacloud.com"};
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

bool AlibabaWebStrategy::shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

ProviderFetchResult AlibabaWebStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "web";

    QString cookieHeader = ctx.manualCookieHeader.value_or(QString());
    if (cookieHeader.isEmpty()) {
        cookieHeader = buildCookieHeader();
    }
    if (cookieHeader.isEmpty()) {
        result.errorMessage = "no cookies found";
        return result;
    }

    // Try to fetch usage from Alibaba Cloud console
    // Note: Alibaba Cloud doesn't have a public usage API for general AI services
    // This is a best-effort implementation that checks the console API
    QString region = ctx.settings.get("apiRegion", "international").toString();
    QString baseUrl = (region == "china") 
        ? "https://account.aliyun.com" 
        : "https://account.alibabacloud.com";

    QHash<QString, QString> headers;
    headers["Cookie"] = cookieHeader;
    headers["Accept"] = "application/json";
    headers["Referer"] = baseUrl + "/";
    headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36";

    // Attempt to get user info to verify cookie is valid
    QJsonObject userJson = NetworkManager::instance().getJsonSync(
        QUrl(baseUrl + "/login/login_result.htm"), headers, ctx.networkTimeoutMs);
    
    if (userJson.isEmpty()) {
        result.errorMessage = "empty or invalid response from Alibaba web";
        return result;
    }

    // Build a basic snapshot indicating web access works
    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();
    
    RateWindow primary;
    primary.usedPercent = 0;
    primary.resetDescription = "Web cookie authenticated";
    snap.primary = primary;
    
    // Extract identity if available
    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::alibaba;
    QString email = userJson.value("email").toString();
    if (email.isEmpty()) email = userJson.value("loginId").toString();
    if (!email.isEmpty()) {
        identity.accountEmail = email;
    }
    snap.identity = identity;

    result.usage = snap;
    result.success = true;
    return result;
}

// ============================================================================
// AlibabaAPIStrategy
// ============================================================================

AlibabaAPIStrategy::AlibabaAPIStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool AlibabaAPIStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !resolveAPIKey(ctx).isEmpty();
}

QString AlibabaAPIStrategy::resolveAPIKey(const ProviderFetchContext& ctx) {
    // 1. Check environment variable
    QString envKey = ctx.env.value("ALIBABA_API_KEY");
    if (!envKey.isEmpty()) return envKey;

    // 2. Check Windows Credential Manager
    auto cred = ProviderCredentialStore::read("com.codexbar.apikey.alibaba");
    if (cred.has_value() && !cred->isEmpty()) return QString::fromUtf8(*cred);

    // 3. Check settings (not recommended for security, but supported)
    QString settingsKey = ctx.settings.get("apiKey").toString();
    if (!settingsKey.isEmpty()) return settingsKey;

    return {};
}

QString AlibabaAPIStrategy::resolveRegion(const ProviderFetchContext& ctx) {
    return ctx.settings.get("apiRegion", "international").toString();
}

bool AlibabaAPIStrategy::shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

ProviderFetchResult AlibabaAPIStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "api";

    QString apiKey = resolveAPIKey(ctx);
    if (apiKey.isEmpty()) {
        result.errorMessage = "API key not configured";
        return result;
    }

    QString region = resolveRegion(ctx);
    
    // Alibaba Cloud API endpoint
    // Note: This is a generic implementation. Specific AI services (Qwen, etc.) 
    // may have different endpoints and usage APIs.
    QString baseUrl = (region == "china")
        ? "https://dashscope.aliyuncs.com"
        : "https://dashscope-intl.aliyuncs.com";

    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + apiKey;
    headers["Content-Type"] = "application/json";
    headers["Accept"] = "application/json";

    // Try to get usage info from DashScope API
    QJsonObject json = NetworkManager::instance().getJsonSync(
        QUrl(baseUrl + "/api/v1/users/me"), headers, ctx.networkTimeoutMs);

    if (json.isEmpty()) {
        result.errorMessage = "empty or invalid response from Alibaba API";
        return result;
    }

    if (json.contains("error")) {
        QString errorMsg = json.value("error").toObject().value("message").toString();
        if (errorMsg.isEmpty()) errorMsg = "API error";
        result.errorMessage = errorMsg;
        return result;
    }

    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    // Parse basic user info
    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::alibaba;
    QString email = json.value("email").toString();
    if (!email.isEmpty()) identity.accountEmail = email;
    snap.identity = identity;

    // Try to get quota/usage info
    QJsonObject quotaJson = NetworkManager::instance().getJsonSync(
        QUrl(baseUrl + "/api/v1/users/quota"), headers, ctx.networkTimeoutMs);

    RateWindow primary;
    primary.usedPercent = 0;
    
    if (!quotaJson.isEmpty() && !quotaJson.contains("error")) {
        double totalQuota = quotaJson.value("total_quota").toDouble(0);
        double usedQuota = quotaJson.value("used_quota").toDouble(0);
        
        if (totalQuota > 0) {
            primary.usedPercent = (usedQuota / totalQuota) * 100.0;
            primary.resetDescription = QString("%1 / %2 tokens used").arg(usedQuota).arg(totalQuota);
        } else {
            primary.resetDescription = "Quota information unavailable";
        }
    } else {
        primary.resetDescription = "API authenticated";
    }
    
    snap.primary = primary;

    result.usage = snap;
    result.success = true;
    return result;
}
