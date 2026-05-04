#include "AugmentProvider.h"
#include "../../network/NetworkManager.h"
#include "../../util/BinaryLocator.h"
#include "../shared/CookieImporter.h"
#include "../shared/BrowserDetection.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>

AugmentProvider::AugmentProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> AugmentProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new AugmentCLIStrategy(this), new AugmentWebStrategy(this) };
}

// CLI Strategy
AugmentCLIStrategy::AugmentCLIStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool AugmentCLIStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !BinaryLocator::resolve("auggie").isEmpty();
}

bool AugmentCLIStrategy::shouldFallback(const ProviderFetchResult& result,
                                         const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

ProviderFetchResult AugmentCLIStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "cli";

    QString auggiePath = BinaryLocator::resolve("auggie");
    if (auggiePath.isEmpty()) {
        result.success = false;
        result.errorMessage = "auggie CLI not found in PATH.";
        return result;
    }

    QProcess proc;
    QStringList env = QProcessEnvironment::systemEnvironment().toStringList();
    proc.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    proc.start(auggiePath, {"account", "status"});
    if (!proc.waitForFinished(5000)) {
        proc.kill();
        result.success = false;
        result.errorMessage = "auggie account status timed out";
        return result;
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    if (output.isEmpty()) {
        result.success = false;
        result.errorMessage = "Empty output from auggie CLI";
        return result;
    }

    // "X remaining . Y / Z credits used"
    QRegularExpression re("(\\d+)\\s+remaining\\s*\\.\\s*(\\d+)\\s*/\\s*(\\d+)\\s*credits\\s+used");
    QRegularExpressionMatch match = re.match(output);

    if (!match.hasMatch()) {
        result.success = false;
        result.errorMessage = "Could not parse auggie output: " + output.left(200);
        return result;
    }

    int remaining = match.captured(1).toInt();
    int consumed = match.captured(2).toInt();
    int total = match.captured(3).toInt();

    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::augment;
    snap.identity = identity;

    RateWindow rw;
    rw.usedPercent = (total > 0) ? (static_cast<double>(consumed) / total * 100.0) : 0;
    rw.resetDescription = QString("%1 remaining (%2/%3 used)").arg(remaining).arg(consumed).arg(total);
    snap.primary = rw;

    result.usage = snap;
    result.success = true;
    return result;
}

// Web Strategy
AugmentWebStrategy::AugmentWebStrategy(QObject* parent) : IFetchStrategy(parent) {}

QString AugmentWebStrategy::resolveCookieHeader(const ProviderFetchContext& ctx) {
    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->isEmpty())
        return *ctx.manualCookieHeader;

    QString manual = ctx.settings.get("manualCookieHeader").toString().trimmed();
    if (!manual.isEmpty()) return manual;

    QStringList domains = {"augmentcode.com", "app.augmentcode.com"};
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

bool AugmentWebStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    return !resolveCookieHeader(ctx).isEmpty();
}

bool AugmentWebStrategy::shouldFallback(const ProviderFetchResult& result,
                                         const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

ProviderFetchResult AugmentWebStrategy::parseCreditsResponse(const QJsonObject& json) {
    ProviderFetchResult result;
    result.strategyID = "augment.web";
    result.strategyKind = ProviderFetchKind::Web;
    result.sourceLabel = "web";

    if (json.isEmpty()) {
        result.success = false;
        result.errorMessage = "Empty response from Augment API";
        return result;
    }

    int remaining = json["usageUnitsRemaining"].toInt(0);
    int consumed = json["usageUnitsConsumedThisBillingCycle"].toInt(0);
    int available = json["usageUnitsAvailable"].toInt(0);
    int total = (available > 0) ? available : (remaining + consumed);

    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::augment;
    snap.identity = identity;

    RateWindow rw;
    rw.usedPercent = (total > 0) ? (static_cast<double>(consumed) / total * 100.0) : 0;
    rw.resetDescription = QString("%1 remaining (%2/%3 used)").arg(remaining).arg(consumed).arg(total);
    snap.primary = rw;

    result.usage = snap;
    result.success = true;
    return result;
}

ProviderFetchResult AugmentWebStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "web";

    QString cookieHeader = resolveCookieHeader(ctx);
    if (cookieHeader.isEmpty()) {
        result.success = false;
        result.errorMessage = "No Augment cookie available.";
        return result;
    }

    QHash<QString, QString> headers;
    headers["Cookie"] = cookieHeader;
    headers["Accept"] = "application/json";

    QJsonObject json = NetworkManager::instance().getJsonSync(
        QUrl("https://app.augmentcode.com/api/credits"), headers, ctx.networkTimeoutMs);

    return parseCreditsResponse(json);
}
