#include "UsageStore.h"
#include "BatchUpdateController.h"
#include "Localization.h"
#include "PlanUtilizationHistoryStore.h"
#include "SessionQuotaNotifications.h"
#include "../providers/ProviderRegistry.h"
#include "../providers/ProviderPipeline.h"
#include "../providers/ProviderFetchContext.h"
#include "../providers/shared/ProviderCredentialStore.h"
#include "../app/SettingsStore.h"
#include "../network/NetworkManager.h"
#include "../util/CostUsageScanner.h"
#include "../util/CostUsageCache.h"
#include "../util/UsagePaceText.h"
#include "../models/UsagePace.h"
#include "../providers/codex/ManagedCodexAccountService.h"
#include "../providers/codex/CodexCreditsFetcher.h"
#include "../providers/codex/CodexDashboardCache.h"
#include "../account/TokenAccountStore.h"
#include "../runtime/ProviderRuntimeManager.h"
#include "../runtime/IProviderRuntime.h"

#include <QDateTime>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>
#include <QUrlQuery>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QElapsedTimer>

// Performance probe: logs when a scoped block exceeds thresholdMicros.
// Only active in debug builds to avoid overhead in production.
#ifdef QT_DEBUG
struct PerfProbe {
    QElapsedTimer timer;
    const char* name;
    qint64 thresholdMicros;
    PerfProbe(const char* n, qint64 t) : name(n), thresholdMicros(t) { timer.start(); }
    ~PerfProbe() {
        qint64 elapsed = timer.nsecsElapsed() / 1000;
        if (elapsed >= thresholdMicros) {
            qDebug() << "[PERF]" << name << "took" << elapsed << "us";
        }
    }
};
#define PERF_PROBE(name, thresholdMicros) PerfProbe _perfProbe(name, thresholdMicros);
#else
#define PERF_PROBE(name, thresholdMicros)
#endif

namespace {

// Lazily-populated snapshot of system environment variables.
// Populated once on first access and then cached for the process lifetime.
// This avoids repeated QProcessEnvironment::systemEnvironment() calls which
// invoke GetEnvironmentStrings() on Windows each time.
// After initial population, any env vars set via qputenv() after cache
// creation are NOT reflected. Call rebuildSystemEnvCache() to force a refresh.
QHash<QString, QString> g_systemEnvCache;
bool g_systemEnvCachePopulated = false;

const QHash<QString, QString>& cachedSystemEnv()
{
    if (!g_systemEnvCachePopulated) {
        auto systemEnv = QProcessEnvironment::systemEnvironment();
        const auto keys = systemEnv.keys();
        for (const auto& key : keys) {
            g_systemEnvCache.insert(key, systemEnv.value(key));
        }
        g_systemEnvCachePopulated = true;
    }
    return g_systemEnvCache;
}

void rebuildSystemEnvCache()
{
    g_systemEnvCache.clear();
    auto systemEnv = QProcessEnvironment::systemEnvironment();
    const auto keys = systemEnv.keys();
    for (const auto& key : keys) {
        g_systemEnvCache.insert(key, systemEnv.value(key));
    }
    g_systemEnvCachePopulated = true;
}

bool isSourceModeAllowed(const QString& providerId, ProviderSourceMode mode)
{
    if (mode == ProviderSourceMode::Auto) return true;
    auto desc = ProviderRegistry::instance().descriptor(providerId);
    if (!desc.has_value()) return true;
    return desc->fetchPlan.allowedSourceModes.contains(sourceModeToString(mode));
}

QString statusEndpointFor(const QString& statusPageURL)
{
    QUrl url(statusPageURL);
    if (!url.isValid() || url.host().isEmpty()) return {};
    url.setPath("/api/v2/status.json");
    url.setQuery(QString());
    url.setFragment({});
    return url.toString();
}

QString mappedStatusFromIndicator(const QString& indicator)
{
    if (indicator == "none") return "ok";
    if (indicator == "minor" || indicator == "maintenance") return "degraded";
    if (indicator == "major" || indicator == "critical") return "outage";
    return "unknown";
}

} // namespace

UsageStore::UsageStore(QObject* parent)
    : QObject(parent)
    , m_pipeline(new ProviderPipeline(this))
    , m_historyStore(new PlanUtilizationHistoryStore(this))
    , m_threadPool(new QThreadPool(this))
{
    m_threadPool->setMaxThreadCount(qMax(4, QThread::idealThreadCount()));
    m_threadPool->setExpiryTimeout(30000);

    // Initialize batch update controller to avoid signal storm
    m_batchUpdater = new BatchUpdateController(this);
    connect(m_batchUpdater, &BatchUpdateController::batchUpdateReady,
            this, &UsageStore::onBatchUpdateReady);
    connect(m_batchUpdater, &BatchUpdateController::batchFinished,
            this, &UsageStore::onBatchFinished);

    QObject::connect(&m_refreshTimer, &QTimer::timeout, this, &UsageStore::refresh);
    QObject::connect(&m_statusTimer, &QTimer::timeout, this, &UsageStore::refreshProviderStatuses);
    QObject::connect(m_pipeline, &ProviderPipeline::pipelineComplete,
                     this, [this](const ProviderFetchResult& /*result*/) {
    });

    // Initialize Codex multi-account service
    m_codexAccountService = new ManagedCodexAccountService(cachedSystemEnv(), this);
    QObject::connect(m_codexAccountService, &ManagedCodexAccountService::accountsChanged,
                     this, &UsageStore::codexAccountsChanged);
    QObject::connect(m_codexAccountService, &ManagedCodexAccountService::accountsChanged,
                     this, &UsageStore::codexAccountStateChanged);
    QObject::connect(m_codexAccountService, &ManagedCodexAccountService::activeAccountChanged,
                     this, &UsageStore::codexActiveAccountChanged);
    QObject::connect(m_codexAccountService, &ManagedCodexAccountService::activeAccountChanged,
                     this, [this](const QString&) {
        emit codexAccountStateChanged();
    });
    QObject::connect(m_codexAccountService, &ManagedCodexAccountService::authenticationStarted,
                     this, &UsageStore::codexAuthenticationStarted);
    QObject::connect(m_codexAccountService, &ManagedCodexAccountService::authenticationStarted,
                     this, [this](const QString&) { emit codexAccountStateChanged(); });
    QObject::connect(m_codexAccountService, &ManagedCodexAccountService::authenticationFinished,
                     this, &UsageStore::codexAuthenticationFinished);
    QObject::connect(m_codexAccountService, &ManagedCodexAccountService::authenticationFinished,
                     this, [this](const QString&, bool success) {
        emit codexAccountStateChanged();
        Q_UNUSED(success)
    });
    QObject::connect(m_codexAccountService, &ManagedCodexAccountService::authenticationStateChanged,
                     this, &UsageStore::codexAccountStateChanged);
    QObject::connect(m_codexAccountService, &ManagedCodexAccountService::removalStarted,
                     this, &UsageStore::codexRemovalStarted);
    QObject::connect(m_codexAccountService, &ManagedCodexAccountService::removalStarted,
                     this, [this](const QString&) { emit codexAccountStateChanged(); });
    QObject::connect(m_codexAccountService, &ManagedCodexAccountService::removalFinished,
                     this, &UsageStore::codexRemovalFinished);
    QObject::connect(m_codexAccountService, &ManagedCodexAccountService::removalFinished,
                     this, [this](const QString&, bool) { emit codexAccountStateChanged(); });
}

void UsageStore::setSettingsStore(SettingsStore* s) {
    if (m_settingsStore == s) return;
    if (m_settingsStore) {
        disconnect(m_settingsStore, nullptr, this, nullptr);
    }
    m_settingsStore = s;
    if (m_settingsStore) {
        connect(m_settingsStore, &SettingsStore::statusChecksEnabledChanged,
                this, &UsageStore::configureStatusPolling);
        auto notifyDisplaySettingsChanged = [this]() {
            m_snapshotRevision++;
            m_snapshotDataCache.clear();
            emit snapshotRevisionChanged();
            // snapshotRevisionChanged() is sufficient; TrayPanel/PlanUtilizationChart
            // bind to snapshotRevision. No need to emit snapshotChanged(id) individually.
        };
        connect(m_settingsStore, &SettingsStore::usageBarsShowUsedChanged,
                this, notifyDisplaySettingsChanged);
        connect(m_settingsStore, &SettingsStore::resetTimesShowAbsoluteChanged,
                this, notifyDisplaySettingsChanged);
        connect(m_settingsStore, &SettingsStore::showOptionalCreditsAndExtraUsageChanged,
                this, notifyDisplaySettingsChanged);
    }
    configureStatusPolling();
}

void UsageStore::configureStatusPolling() {
    const bool enabled = m_settingsStore ? m_settingsStore->statusChecksEnabled() : true;
    if (!enabled) {
        m_statusTimer.stop();
        return;
    }
    m_statusTimer.start(5 * 60 * 1000);
}

UsageSnapshot UsageStore::snapshot(const QString& providerId) const {
    return m_snapshots.value(providerId, UsageSnapshot{});
}

bool UsageStore::isProviderEnabled(const QString& id) const {
    return ProviderRegistry::instance().isProviderEnabled(id);
}

void UsageStore::setProviderEnabled(const QString& id, bool enabled) {
    ProviderRegistry::instance().setProviderEnabled(id, enabled);
    if (m_settingsStore) {
        m_settingsStore->setProviderEnabled(id, enabled);
    }
    updateProviderIDs();
}

QString UsageStore::providerDisplayName(const QString& id) const {
    auto desc = ProviderRegistry::instance().descriptor(id);
    if (desc.has_value()) return desc->metadata.displayName;
    return id;
}

QStringList UsageStore::providerIDs() const {
    return m_providerIDs;
}

void UsageStore::rebuildSystemEnvCache()
{
    ::rebuildSystemEnvCache();
}

void UsageStore::preloadCredentials() {
    const auto ids = ProviderRegistry::instance().providerIDs();
    for (const auto& id : ids) {
        auto* provider = ProviderRegistry::instance().provider(id);
        if (!provider) continue;
        for (const auto& desc : provider->settingsDescriptors()) {
            if (!desc.sensitive || desc.credentialTarget.isEmpty()) continue;
            {
                QMutexLocker locker(&m_credentialCacheMutex);
                if (m_credentialCache.contains(desc.credentialTarget)) continue;
                if (m_credentialMissing.contains(desc.credentialTarget)) continue;
            }
            auto stored = ProviderCredentialStore::read(desc.credentialTarget);
            {
                QMutexLocker locker(&m_credentialCacheMutex);
                if (stored.has_value()) {
                    m_credentialCache[desc.credentialTarget] = {stored.value(), QDateTime::currentDateTime()};
                } else {
                    m_credentialMissing[desc.credentialTarget] = true;
                }
            }
        }
    }
}

ProviderFetchContext UsageStore::buildFetchContextForProvider(const QString& providerId) const {
    PERF_PROBE("buildFetchContextForProvider", 2000);
    ProviderFetchContext ctx;
    ctx.providerId = providerId;
    ctx.sourceMode = ProviderSourceMode::Auto;
    ctx.isAppRuntime = true;
    ctx.allowInteractiveAuth = false;
    ctx.networkTimeoutMs = ProviderPipeline::STRATEGY_TIMEOUT_MS;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const auto envKeys = env.keys();
    for (const auto& key : envKeys) {
        ctx.env[key] = env.value(key);
    }

    auto addSetting = [&](const QString& key, const QVariant& defaultValue = QVariant()) {
        QVariant value = m_settingsStore
            ? m_settingsStore->providerSetting(providerId, key, defaultValue)
            : defaultValue;
        if (value.isValid()) {
            ctx.settings.set(key, value);
        }
        return value;
    };

    auto* provider = ProviderRegistry::instance().provider(providerId);
    if (provider) {
        for (const auto& descriptor : provider->settingsDescriptors()) {
            if (descriptor.sensitive) {
                QString secret;
                if (!descriptor.envVar.isEmpty() && ctx.env.contains(descriptor.envVar)) {
                    secret = ctx.env.value(descriptor.envVar).trimmed();
                }
                if (secret.isEmpty() && !descriptor.credentialTarget.isEmpty()) {
                    // Use cached credential to avoid blocking main thread with WinCred API
                    bool cacheHit = false;
                    bool cacheExpired = true;
                    bool isMissing = false;
                    {
                        QMutexLocker locker(&m_credentialCacheMutex);
                        auto cacheIt = m_credentialCache.find(descriptor.credentialTarget);
                        if (cacheIt != m_credentialCache.end()) {
                            cacheHit = true;
                            cacheExpired = cacheIt.value().cachedAt.msecsTo(QDateTime::currentDateTime()) >= CREDENTIAL_CACHE_TTL_MS;
                            if (!cacheExpired) {
                                secret = QString::fromUtf8(cacheIt.value().data).trimmed();
                            }
                        }
                        isMissing = m_credentialMissing.contains(descriptor.credentialTarget);
                    }
                    if (!cacheHit && !isMissing) {
                        auto stored = ProviderCredentialStore::read(descriptor.credentialTarget);
                        {
                            QMutexLocker locker(&m_credentialCacheMutex);
                            if (stored.has_value()) {
                                secret = QString::fromUtf8(stored.value()).trimmed();
                                m_credentialCache[descriptor.credentialTarget] = {stored.value(), QDateTime::currentDateTime()};
                            } else {
                                m_credentialMissing[descriptor.credentialTarget] = true;
                            }
                        }
                    }
                }
                if (secret.isEmpty()) {
                    secret = addSetting(descriptor.key, descriptor.defaultValue).toString().trimmed();
                } else {
                    ctx.settings.set(descriptor.key, secret);
                }
            } else {
                addSetting(descriptor.key, descriptor.defaultValue);
            }
        }
    }

    QString sourceMode = addSetting("sourceMode", "auto").toString();
    if (sourceMode == "auto") {
        if (providerId == "codex") {
            sourceMode = addSetting("codexDataSource", "auto").toString();
        } else if (providerId == "claude") {
            sourceMode = addSetting("claudeDataSource", "auto").toString();
        }
    }
    ctx.settings.set("sourceMode", sourceMode);
    ctx.sourceMode = sourceModeFromString(sourceMode);

    QString cookieSource = addSetting("cookieSource", "auto").toString();
    if (providerId == "cursor" && cookieSource == "auto") {
        cookieSource = addSetting("cursorCookieSource", "auto").toString();
    }
    ctx.settings.set("cookieSource", cookieSource);

    QString manualCookie = ctx.settings.get("manualCookieHeader").toString().trimmed();
    if (manualCookie.isEmpty()) {
        manualCookie = addSetting("manualCookieHeader", "").toString().trimmed();
    }
    if (!manualCookie.isEmpty()) {
        ctx.manualCookieHeader = manualCookie;
    }

    QString accountId = addSetting("accountID", "").toString().trimmed();
    ctx.accountID = accountId;

    if (providerId == "codex" && m_codexAccountService) {
        const QString activeId = m_codexAccountService->activeAccountID();
        if (!activeId.isEmpty()) {
            ctx.accountID = activeId;
        }

        const QString managedHome = m_codexAccountService->activeManagedHomePath();
        if (!managedHome.isEmpty()) {
            ctx.env["CODEX_HOME"] = managedHome;
        }
    }

    bool ok = false;
    int timeout = addSetting("networkTimeoutMs", ProviderPipeline::STRATEGY_TIMEOUT_MS).toInt(&ok);
    if (ok && timeout > 0) {
        ctx.networkTimeoutMs = timeout;
    }

    QString apiRegion = addSetting("apiRegion", "global").toString();
    if (providerId == "zai") {
        ctx.env["ZAI_API_REGION"] = apiRegion;
    }

    // Token Account resolution
    TokenAccountStore* accountStore = TokenAccountStore::instance();
    QString resolvedAccountId = ctx.accountID;
    if (resolvedAccountId.isEmpty()) {
        resolvedAccountId = accountStore->defaultAccountId(providerId);
    }
    if (!resolvedAccountId.isEmpty()) {
        auto accOpt = accountStore->account(resolvedAccountId);
        if (accOpt.has_value()) {
            const TokenAccount& acc = accOpt.value();
            ctx.accountID = acc.accountId;
            ctx.accountCredentials = acc.credentials;
            // Per-account source mode override (if not Auto)
            if (acc.sourceMode != ProviderSourceMode::Auto) {
                ctx.sourceMode = acc.sourceMode;
            }
        }
    }

    return ctx;
}

void UsageStore::updateProviderIDs() {
    m_providerIDs = ProviderRegistry::instance().enabledProviderIDs();
    m_providerListCacheValid = false;
    emit providerIDsChanged();
}

QVariantMap UsageStore::snapshotData(const QString& id) const {
    PERF_PROBE("snapshotData", 1000);
    auto cacheIt = m_snapshotDataCache.find(id);
    if (cacheIt != m_snapshotDataCache.end()) {
        return cacheIt.value();
    }

    auto snap = snapshot(id);
    auto* prov = ProviderRegistry::instance().provider(id);
    QVariantMap m;
    const bool showUsedPercent = m_settingsStore ? m_settingsStore->usageBarsShowUsed() : false;
    const bool showAbsoluteResetTimes = m_settingsStore ? m_settingsStore->resetTimesShowAbsolute() : false;
    const bool showOptionalFields = m_settingsStore ? m_settingsStore->showOptionalCreditsAndExtraUsage() : true;

    auto resetDisplay = [&](const RateWindow& rw) -> QString {
        if (showAbsoluteResetTimes && rw.resetsAt.has_value() && rw.resetsAt->isValid()) {
            return rw.resetsAt->toLocalTime().toString("yyyy-MM-dd hh:mm");
        }
        return rw.resetDescription.value_or(QString());
    };

    auto addWindowFields = [&](const QString& prefix, const RateWindow& rw, bool isDetailProvider) {
        const double remaining = rw.remainingPercent();
        m[prefix + "Used"] = rw.usedPercent;
        m[prefix + "Remaining"] = remaining;
        m[prefix + "DisplayPercent"] = showUsedPercent ? rw.usedPercent : remaining;
        m[prefix + "DisplayIsUsed"] = showUsedPercent;
        if (rw.resetsAt.has_value())
            m[prefix + "ResetsAt"] = rw.resetsAt->toMSecsSinceEpoch();

        // For detail-only providers (deepseek/warp/kilo/abacus),
        // resetDescription contains balance/credit detail, not a reset time.
        // Extract it to a separate detail field and avoid "Resets" rendering.
        if (isDetailProvider && rw.resetDescription.has_value()) {
            QString detail = rw.resetDescription.value().trimmed();
            if (!detail.isEmpty())
                m[prefix + "Detail"] = detail;
        } else {
            const QString resetText = resetDisplay(rw);
            if (!resetText.isEmpty())
                m[prefix + "ResetDesc"] = resetText;
        }
    };

    m["sessionLabel"] = Localization::providerLabel(prov ? prov->sessionLabel() : "Session");
    m["weeklyLabel"] = Localization::providerLabel(prov ? prov->weeklyLabel() : "Weekly");
    m["opusLabel"] = Localization::providerLabel(prov ? prov->opusLabel() : QString());
    m["supportsCredits"] = prov ? prov->supportsCredits() : false;
    m["displayName"] = providerDisplayName(id);

    const bool isDetailProvider = (id == "deepseek" || id == "warp" || id == "kilo" || id == "abacus");

    if (snap.primary.has_value()) {
        addWindowFields("primary", *snap.primary, isDetailProvider);
    } else {
        m["primaryUsed"] = 0.0;
        m["primaryRemaining"] = 100.0;
        m["primaryDisplayPercent"] = showUsedPercent ? 0.0 : 100.0;
        m["primaryDisplayIsUsed"] = showUsedPercent;
    }
    if (snap.secondary.has_value()) {
        addWindowFields("secondary", *snap.secondary, false);
        m["hasSecondary"] = true;
    } else {
        m["secondaryUsed"] = 0.0;
        m["secondaryRemaining"] = 100.0;
        m["secondaryDisplayPercent"] = showUsedPercent ? 0.0 : 100.0;
        m["secondaryDisplayIsUsed"] = showUsedPercent;
        m["hasSecondary"] = false;
    }
    if (snap.tertiary.has_value()) {
        addWindowFields("tertiary", *snap.tertiary, false);
        m["hasTertiary"] = true;
    } else {
        m["hasTertiary"] = false;
    }
    if (snap.identity.has_value()) {
        if (snap.identity->loginMethod.has_value())
            m["loginMethod"] = snap.identity->loginMethod.value();
    }

    bool hasUsage = snap.primary.has_value() || snap.secondary.has_value() || snap.tertiary.has_value();
    m["hasUsage"] = hasUsage;

    if (showOptionalFields && snap.providerCost.has_value()) {
        m["providerCostUsed"] = snap.providerCost->used;
        m["providerCostLimit"] = snap.providerCost->limit;
        m["providerCostCurrency"] = snap.providerCost->currencyCode;
        m["hasProviderCost"] = true;
    } else {
        m["hasProviderCost"] = false;
    }

    m["updatedAt"] = snap.updatedAt.toMSecsSinceEpoch();

    if (showOptionalFields && snap.zaiUsage.has_value()) {
        QVariantMap zai;
        const auto& z = *snap.zaiUsage;
        if (z.tokenLimit.has_value()) {
            QVariantMap tl;
            tl["usedPercent"] = z.tokenLimit->usedPercent();
            tl["windowDescription"] = z.tokenLimit->windowDescription();
            tl["windowLabel"] = z.tokenLimit->windowLabel();
            if (z.tokenLimit->usage.has_value()) tl["usage"] = *z.tokenLimit->usage;
            if (z.tokenLimit->currentValue.has_value()) tl["currentValue"] = *z.tokenLimit->currentValue;
            if (z.tokenLimit->remaining.has_value()) tl["remaining"] = *z.tokenLimit->remaining;
            QVariantList details;
            for (auto& d : z.tokenLimit->usageDetails) {
                QVariantMap dm;
                dm["modelCode"] = d.modelCode;
                dm["usage"] = d.usage;
                details.append(dm);
            }
            tl["usageDetails"] = details;
            zai["tokenLimit"] = tl;
        }
        if (z.timeLimit.has_value()) {
            QVariantMap tl;
            tl["usedPercent"] = z.timeLimit->usedPercent();
            tl["windowDescription"] = z.timeLimit->windowDescription();
            tl["windowLabel"] = z.timeLimit->windowLabel();
            QVariantList details;
            for (auto& d : z.timeLimit->usageDetails) {
                QVariantMap dm;
                dm["modelCode"] = d.modelCode;
                dm["usage"] = d.usage;
                details.append(dm);
            }
            tl["usageDetails"] = details;
            zai["timeLimit"] = tl;
        }
        if (z.sessionTokenLimit.has_value()) {
            QVariantMap sl;
            sl["usedPercent"] = z.sessionTokenLimit->usedPercent();
            sl["windowDescription"] = z.sessionTokenLimit->windowDescription();
            sl["windowLabel"] = z.sessionTokenLimit->windowLabel();
            zai["sessionTokenLimit"] = sl;
        }
        if (z.planName.has_value()) zai["planName"] = *z.planName;
        m["zaiUsage"] = zai;
    }

    if (showOptionalFields && snap.openRouterUsage.has_value()) {
        QVariantMap oru;
        const auto& o = *snap.openRouterUsage;
        oru["totalCredits"] = o.totalCredits;
        oru["totalUsage"] = o.totalUsage;
        oru["balance"] = o.balance;
        oru["usedPercent"] = o.usedPercent;
        oru["keyQuotaStatus"] = static_cast<int>(o.keyQuotaStatus());
        if (o.keyLimit.has_value()) oru["keyLimit"] = *o.keyLimit;
        if (o.keyUsage.has_value()) oru["keyUsage"] = *o.keyUsage;
        if (o.hasValidKeyQuota()) {
            oru["keyRemaining"] = o.keyRemaining();
            oru["keyUsedPercent"] = o.keyUsedPercent();
        }
        if (o.rateLimit.has_value()) {
            QVariantMap rl;
            rl["requests"] = o.rateLimit->requests;
            rl["interval"] = o.rateLimit->interval;
            oru["rateLimit"] = rl;
        }
        m["openRouterUsage"] = oru;
    }

    if (showOptionalFields && snap.providerCost.has_value()) {
        QVariantMap pc;
        pc["used"] = snap.providerCost->used;
        pc["limit"] = snap.providerCost->limit;
        pc["currencyCode"] = snap.providerCost->currencyCode;
        if (snap.providerCost->period.has_value()) pc["period"] = *snap.providerCost->period;
        m["providerCost"] = pc;
    }

    m["updatedAt"] = snap.updatedAt.toMSecsSinceEpoch();

    auto addPaceFields = [&](const QString& prefix, const RateWindow& rw) {
        auto pace = UsagePace::weekly(rw, snap.updatedAt);
        if (!pace.has_value()) return;
        auto detail = UsagePaceText::weeklyDetail(*pace);
        m[prefix + "PacePercent"] = pace->expectedUsedPercent;
        m[prefix + "PaceOnTop"] = pace->actualUsedPercent <= pace->expectedUsedPercent;
        m[prefix + "PaceLeftLabel"] = detail.leftLabel;
        m[prefix + "PaceRightLabel"] = detail.rightLabel;
        m[prefix + "PaceStage"] = static_cast<int>(pace->stage);
    };

    if (snap.primary.has_value()) addPaceFields("primary", *snap.primary);
    if (snap.secondary.has_value()) addPaceFields("secondary", *snap.secondary);

    m["error"] = Localization::providerError(m_errors.value(id, ""));

    // Codex-specific: Consumer Projection
    if (id == "codex") {
        CodexConsumerProjection::Context ctx;
        ctx.snapshot = snap;
        ctx.rawUsageError = m_errors.value(id);
        ctx.now = QDateTime::currentDateTime();

        if (m_codexCreditsCache.snapshot.has_value() &&
            m_codexCreditsCache.accountKey == currentCodexAccountKey()) {
            ctx.credits = &m_codexCreditsCache.snapshot.value();
            ctx.rawCreditsError = m_codexCreditsCache.lastError;
        }

        auto projection = CodexConsumerProjection::make(
            CodexConsumerProjection::Surface::LiveCard, ctx);

        // Override primary/secondary with projected lanes
        auto sessionWindow = CodexConsumerProjection::rateWindow(projection, CodexConsumerProjection::RateLane::Session);
        if (sessionWindow.has_value()) {
            addWindowFields("primary", *sessionWindow, false);
        }
        auto weeklyWindow = CodexConsumerProjection::rateWindow(projection, CodexConsumerProjection::RateLane::Weekly);
        if (weeklyWindow.has_value()) {
            addWindowFields("secondary", *weeklyWindow, false);
            m["hasSecondary"] = true;
        }

        // Dashboard visibility
        m["dashboardVisibility"] = static_cast<int>(projection.dashboardVisibility);

        // Menu bar fallback
        m["menuBarFallback"] = static_cast<int>(projection.menuBarFallback);

        // User-facing errors (override raw error)
        if (!projection.userFacingErrors.usage.isEmpty()) {
            m["error"] = projection.userFacingErrors.usage;
        }

        // Credits data - try from projection first, then from providerCost
        if (projection.credits.has_value() && projection.credits->snapshot.has_value()) {
            m["hasCredits"] = true;
            m["creditsRemaining"] = projection.credits->snapshot->remaining;
            if (!projection.credits->userFacingError.isEmpty()) {
                m["creditsError"] = projection.credits->userFacingError;
            }
        } else if (snap.providerCost.has_value()) {
            // Fallback: use providerCost directly (credits are stored here)
            m["hasCredits"] = true;
            m["creditsRemaining"] = snap.providerCost->used;
        } else {
            m["hasCredits"] = false;
        }

        // For Codex, providerCost represents credits, not "extra usage"
        if (snap.providerCost.has_value() && 
            snap.providerCost->period.has_value() && 
            snap.providerCost->period.value() == "Credits") {
            m["hasProviderCost"] = false;  // Hide "Extra usage" for Codex credits
        }

        // Exhausted lane flag
        m["hasExhaustedRateLane"] = CodexConsumerProjection::hasExhaustedRateLane(projection);
    }

    m_snapshotDataCache[id] = m;
    return m;
}

void UsageStore::setCostUsageEnabled(bool v) {
    if (m_costUsageEnabled != v) {
        m_costUsageEnabled = v;
        emit costUsageEnabledChanged();
        if (v) refreshCostUsage();
    }
}

void UsageStore::refreshCostUsage() {
    if (!m_costUsageEnabled || m_costUsageRefreshing) return;
    m_costUsageRefreshing = true;
    emit costUsageRefreshingChanged();

    QtConcurrent::run(m_threadPool, [this]() {
        PERF_PROBE("refreshCostUsage_worker", 5000);

        CostUsageCache& cache = CostUsageCache::instance();
        cache.load();

        CostUsageScanner scanner(&cache);

        QDate today = QDate::currentDate();
        QDate since = today.addDays(-29);

        CostUsageSnapshot claude = scanner.scanClaude({}, since, today);
        CostUsageSnapshot codex = scanner.scanCodex({}, since, today);
        auto piResult = scanner.scanPi(since, today);

        cache.save();

        // Per-provider storage
        QHash<QString, CostUsageSnapshot> perProvider;

        // Claude (merge claude scan + pi claude)
        CostUsageSnapshot mergedClaude;
        mergedClaude.updatedAt = QDateTime::currentDateTime();
        mergedClaude.sessionTokens = claude.sessionTokens + piResult.claude.sessionTokens;
        mergedClaude.sessionCostUSD = claude.sessionCostUSD + piResult.claude.sessionCostUSD;
        mergedClaude.last30DaysTokens = claude.last30DaysTokens + piResult.claude.last30DaysTokens;
        mergedClaude.last30DaysCostUSD = claude.last30DaysCostUSD + piResult.claude.last30DaysCostUSD;
        {
            QHash<QString, CostUsageDailyEntry> dayMap;
            auto mergeDaily = [&](const QVector<CostUsageDailyEntry>& daily) {
                for (auto& d : daily) {
                    auto& e = dayMap[d.date];
                    e.date = d.date;
                    e.inputTokens += d.inputTokens;
                    e.cacheReadTokens += d.cacheReadTokens;
                    e.cacheCreationTokens += d.cacheCreationTokens;
                    e.outputTokens += d.outputTokens;
                    e.costUSD += d.costUSD;
                    for (auto& m : d.models) e.models.append(m);
                }
            };
            mergeDaily(claude.daily);
            mergeDaily(piResult.claude.daily);
            for (auto it = dayMap.constBegin(); it != dayMap.constEnd(); ++it)
                mergedClaude.daily.append(it.value());
            std::sort(mergedClaude.daily.begin(), mergedClaude.daily.end(),
                      [](const CostUsageDailyEntry& a, const CostUsageDailyEntry& b) { return a.date < b.date; });
        }
        if (mergedClaude.last30DaysTokens > 0)
            perProvider["claude"] = mergedClaude;

        // Codex (merge codex scan + pi codex)
        CostUsageSnapshot mergedCodex;
        mergedCodex.updatedAt = QDateTime::currentDateTime();
        mergedCodex.sessionTokens = codex.sessionTokens + piResult.codex.sessionTokens;
        mergedCodex.sessionCostUSD = codex.sessionCostUSD + piResult.codex.sessionCostUSD;
        mergedCodex.last30DaysTokens = codex.last30DaysTokens + piResult.codex.last30DaysTokens;
        mergedCodex.last30DaysCostUSD = codex.last30DaysCostUSD + piResult.codex.last30DaysCostUSD;
        {
            QHash<QString, CostUsageDailyEntry> dayMap;
            auto mergeDaily = [&](const QVector<CostUsageDailyEntry>& daily) {
                for (auto& d : daily) {
                    auto& e = dayMap[d.date];
                    e.date = d.date;
                    e.inputTokens += d.inputTokens;
                    e.cacheReadTokens += d.cacheReadTokens;
                    e.cacheCreationTokens += d.cacheCreationTokens;
                    e.outputTokens += d.outputTokens;
                    e.costUSD += d.costUSD;
                    for (auto& m : d.models) e.models.append(m);
                }
            };
            mergeDaily(codex.daily);
            mergeDaily(piResult.codex.daily);
            for (auto it = dayMap.constBegin(); it != dayMap.constEnd(); ++it)
                mergedCodex.daily.append(it.value());
            std::sort(mergedCodex.daily.begin(), mergedCodex.daily.end(),
                      [](const CostUsageDailyEntry& a, const CostUsageDailyEntry& b) { return a.date < b.date; });
        }
        if (mergedCodex.last30DaysTokens > 0)
            perProvider["codex"] = mergedCodex;

        // OpenCode DB (always scan, merge into respective providers)
        auto openCodeDBResults = scanner.scanOpenCodeDB(since, today);
        for (auto it = openCodeDBResults.constBegin(); it != openCodeDBResults.constEnd(); ++it) {
            QString providerId = it.key();
            CostUsageSnapshot snap = it.value();
            if (snap.last30DaysTokens <= 0) continue;
            if (perProvider.contains(providerId)) {
                // Merge with existing provider data
                auto& existing = perProvider[providerId];
                existing.sessionTokens += snap.sessionTokens;
                existing.sessionCostUSD += snap.sessionCostUSD;
                existing.last30DaysTokens += snap.last30DaysTokens;
                existing.last30DaysCostUSD += snap.last30DaysCostUSD;
                // Merge daily entries
                QHash<QString, CostUsageDailyEntry> dayMap;
                for (auto& d : existing.daily) dayMap[d.date] = d;
                for (auto& d : snap.daily) {
                    auto& e = dayMap[d.date];
                    e.date = d.date;
                    e.inputTokens += d.inputTokens;
                    e.cacheReadTokens += d.cacheReadTokens;
                    e.cacheCreationTokens += d.cacheCreationTokens;
                    e.outputTokens += d.outputTokens;
                    e.costUSD += d.costUSD;
                    for (auto& m : d.models) e.models.append(m);
                }
                existing.daily = dayMap.values();
                std::sort(existing.daily.begin(), existing.daily.end(),
                          [](const CostUsageDailyEntry& a, const CostUsageDailyEntry& b) { return a.date < b.date; });
            } else {
                perProvider[providerId] = snap;
            }
        }

        // OpenCode Go (separate from opencode.db)
        CostUsageSnapshot opencodego = scanner.scanOpenCodeGo(since, today);
        if (opencodego.last30DaysTokens > 0)
            perProvider["opencodego"] = opencodego;

        // Global aggregate (backward compat)
        CostUsageSnapshot combined;
        combined.updatedAt = QDateTime::currentDateTime();
        for (auto it = perProvider.constBegin(); it != perProvider.constEnd(); ++it) {
            combined.sessionTokens += it.value().sessionTokens;
            combined.sessionCostUSD += it.value().sessionCostUSD;
            combined.last30DaysTokens += it.value().last30DaysTokens;
            combined.last30DaysCostUSD += it.value().last30DaysCostUSD;
        }
        {
            QHash<QString, CostUsageDailyEntry> dayMap;
            auto mergeDaily = [&](const QVector<CostUsageDailyEntry>& daily) {
                for (auto& d : daily) {
                    auto& e = dayMap[d.date];
                    e.date = d.date;
                    e.inputTokens += d.inputTokens;
                    e.cacheReadTokens += d.cacheReadTokens;
                    e.cacheCreationTokens += d.cacheCreationTokens;
                    e.outputTokens += d.outputTokens;
                    e.costUSD += d.costUSD;
                    for (auto& m : d.models) e.models.append(m);
                }
            };
            mergeDaily(mergedClaude.daily);
            mergeDaily(mergedCodex.daily);
            for (auto it = openCodeDBResults.constBegin(); it != openCodeDBResults.constEnd(); ++it)
                mergeDaily(it.value().daily);
            mergeDaily(opencodego.daily);
            for (auto it = dayMap.constBegin(); it != dayMap.constEnd(); ++it)
                combined.daily.append(it.value());
            std::sort(combined.daily.begin(), combined.daily.end(),
                      [](const CostUsageDailyEntry& a, const CostUsageDailyEntry& b) { return a.date < b.date; });
        }

        // Build model summaries per provider
        QVector<ProviderCostUsageSnapshot> allProviders;
        for (auto it = perProvider.constBegin(); it != perProvider.constEnd(); ++it) {
            ProviderCostUsageSnapshot pcs;
            pcs.providerId = it.key();
            pcs.snapshot = it.value();
            QHash<QString, CostUsageModelBreakdown> modelMap;
            for (auto& entry : it.value().daily) {
                for (auto& model : entry.models) {
                    auto& agg = modelMap[model.modelName];
                    agg.modelName = model.modelName;
                    agg.inputTokens += model.inputTokens;
                    agg.cacheReadTokens += model.cacheReadTokens;
                    agg.cacheCreationTokens += model.cacheCreationTokens;
                    agg.outputTokens += model.outputTokens;
                    agg.costUSD += model.costUSD;
                }
            }
            pcs.modelSummary = modelMap.values();
            std::sort(pcs.modelSummary.begin(), pcs.modelSummary.end(),
                      [](const CostUsageModelBreakdown& a, const CostUsageModelBreakdown& b) { return a.costUSD > b.costUSD; });
            allProviders.append(pcs);
        }
        std::sort(allProviders.begin(), allProviders.end(),
                  [](const ProviderCostUsageSnapshot& a, const ProviderCostUsageSnapshot& b) {
                      return a.snapshot.last30DaysCostUSD > b.snapshot.last30DaysCostUSD;
                  });

        QMetaObject::invokeMethod(this, [this, combined, perProvider, allProviders]() {
            PERF_PROBE("refreshCostUsage_callback", 2000);
            m_costUsage = combined;
            m_perProviderCostUsage = perProvider;
            m_allProviderCostUsage = allProviders;
            m_costUsageRefreshing = false;
            m_costUsageDataCacheValid = false;
            m_providerCostUsageListCacheValid = false;
            emit costUsageRefreshingChanged();
            emit costUsageChanged();
        }, Qt::QueuedConnection);
    });
}

QVariantMap UsageStore::costUsageData() const {
    PERF_PROBE("costUsageData", 1000);
    if (m_costUsageDataCacheValid) {
        return m_costUsageDataCache;
    }
    QVariantMap m;
    m["sessionTokens"] = m_costUsage.sessionTokens;
    m["sessionCostUSD"] = m_costUsage.sessionCostUSD;
    m["last30DaysTokens"] = m_costUsage.last30DaysTokens;
    m["last30DaysCostUSD"] = m_costUsage.last30DaysCostUSD;
    m["updatedAt"] = m_costUsage.updatedAt.toMSecsSinceEpoch();
    m["hasData"] = m_costUsage.last30DaysTokens > 0;

    QVariantList dailyList;
    for (auto& d : m_costUsage.daily) {
        if (d.totalTokens() == 0) continue;
        QVariantMap dm;
        dm["date"] = d.date;
        dm["totalTokens"] = d.totalTokens();
        dm["costUSD"] = d.costUSD;

        QVariantList models;
        for (auto& md : d.models) {
            QVariantMap mm;
            mm["name"] = md.modelName;
            mm["tokens"] = md.totalTokens();
            mm["costUSD"] = md.costUSD;
            models.append(mm);
        }
        dm["models"] = models;
        dailyList.append(dm);
    }
    m["daily"] = dailyList;
    m_costUsageDataCacheValid = true;
    m_costUsageDataCache = m;
    return m;
}

QVariantList UsageStore::providerCostUsageList() const {
    PERF_PROBE("providerCostUsageList", 1000);
    if (m_providerCostUsageListCacheValid) {
        return m_providerCostUsageListCache;
    }
    QVariantList result;
    for (auto& pcs : m_allProviderCostUsage) {
        QVariantMap m;
        m["providerId"] = pcs.providerId;
        m["sessionTokens"] = pcs.snapshot.sessionTokens;
        m["sessionCostUSD"] = pcs.snapshot.sessionCostUSD;
        m["last30DaysTokens"] = pcs.snapshot.last30DaysTokens;
        m["last30DaysCostUSD"] = pcs.snapshot.last30DaysCostUSD;

        QVariantList models;
        for (auto& model : pcs.modelSummary) {
            QVariantMap mm;
            mm["name"] = model.modelName;
            mm["tokens"] = model.totalTokens();
            mm["costUSD"] = model.costUSD;
            models.append(mm);
        }
        m["models"] = models;

        QVariantList daily;
        for (auto& d : pcs.snapshot.daily) {
            if (d.totalTokens() == 0) continue;
            QVariantMap dm;
            dm["date"] = d.date;
            dm["totalTokens"] = d.totalTokens();
            dm["costUSD"] = d.costUSD;
            daily.append(dm);
        }
        m["daily"] = daily;
        result.append(m);
    }
    m_providerCostUsageListCacheValid = true;
    m_providerCostUsageListCache = result;
    return result;
}

QVariantMap UsageStore::providerCostUsageData(const QString& providerId) const {
    PERF_PROBE("providerCostUsageData", 1000);
    auto it = m_perProviderCostUsage.constFind(providerId);
    if (it == m_perProviderCostUsage.constEnd()) return QVariantMap();

    CostUsageSnapshot snap = it.value();
    QVariantMap m;
    m["providerId"] = providerId;
    m["sessionTokens"] = snap.sessionTokens;
    m["sessionCostUSD"] = snap.sessionCostUSD;
    m["last30DaysTokens"] = snap.last30DaysTokens;
    m["last30DaysCostUSD"] = snap.last30DaysCostUSD;
    m["hasData"] = snap.last30DaysTokens > 0;
    m["updatedAt"] = snap.updatedAt.toMSecsSinceEpoch();

    QVariantList daily;
    for (auto& d : snap.daily) {
        if (d.totalTokens() == 0) continue;
        QVariantMap dm;
        dm["date"] = d.date;
        dm["totalTokens"] = d.totalTokens();
        dm["costUSD"] = d.costUSD;
        QVariantList models;
        for (auto& md : d.models) {
            QVariantMap mm;
            mm["name"] = md.modelName;
            mm["tokens"] = md.totalTokens();
            mm["costUSD"] = md.costUSD;
            models.append(mm);
        }
        dm["models"] = models;
        daily.append(dm);
    }
    m["daily"] = daily;

    for (auto& pcs : m_allProviderCostUsage) {
        if (pcs.providerId == providerId) {
            QVariantList models;
            for (auto& model : pcs.modelSummary) {
                QVariantMap mm;
                mm["name"] = model.modelName;
                mm["tokens"] = model.totalTokens();
                mm["costUSD"] = model.costUSD;
                models.append(mm);
            }
            m["models"] = models;
            break;
        }
    }

    if (!snap.errorMessage.isEmpty())
        m["errorMessage"] = snap.errorMessage;

    return m;
}

void UsageStore::refresh() {
    refreshCostUsage();
    auto ids = ProviderRegistry::instance().enabledProviderIDs();
    doRefresh(ids);
}

void UsageStore::refreshAll() {
    refreshCostUsage();
    auto ids = ProviderRegistry::instance().providerIDs();
    doRefresh(ids);
}

void UsageStore::doRefresh(const QStringList& ids) {
    PERF_PROBE("doRefresh", 1000);
    auto currentGuard = currentCodexAccountRefreshGuard();
    if (currentGuard != m_lastCodexRefreshGuard) {
        clearCodexOpenAIWebState();
        m_lastCodexRefreshGuard = currentGuard;
    }

    if (m_isRefreshing) return;
    m_isRefreshing = true;
    m_batchRefreshInProgress = ids.size() > 1;
    emit refreshingChanged();

    // Begin batch UI update cycle to avoid signal storm
    if (m_batchUpdater) {
        m_batchUpdater->beginBatch();
    }

    if (ids.isEmpty()) {
        m_isRefreshing = false;
        m_batchRefreshInProgress = false;
        emit refreshingChanged();
        return;
    }

    QDateTime refreshStartedAt = QDateTime::currentDateTime();

    // Parallel credits refresh for Codex (mirrors original CodexBar behavior)
    if (ids.contains("codex") && isProviderEnabled("codex")) {
        auto expectedGuard = currentCodexAccountRefreshGuard();
        m_lastCodexRefreshGuard = expectedGuard;

        // If identity is unresolved, wait for usage refresh to establish identity first
        if (expectedGuard.identity.isEmpty()) {
            ++m_pendingCreditsRefresh;
            // Fire-and-forget delayed credits refresh after usage establishes identity
            QtConcurrent::run(m_threadPool, [this, refreshStartedAt, expectedGuard]() mutable {
                auto snap = waitForCodexSnapshot(refreshStartedAt, 3000);
                if (!snap.updatedAt.isValid()) {
                    // Usage refresh failed or timed out; still try credits with current guard
                }
                auto updatedGuard = currentCodexAccountRefreshGuard();
                if (!updatedGuard.isEmpty()) {
                    expectedGuard = updatedGuard;
                }

                QHash<QString, QString> creditsEnv = cachedSystemEnv();
                if (m_codexAccountService) {
                    QString managedHome = m_codexAccountService->activeManagedHomePath();
                    if (!managedHome.isEmpty()) {
                        creditsEnv.insert("CODEX_HOME", managedHome);
                    }
                }

                CodexCreditsFetcher fetcher(creditsEnv);
                auto result = fetcher.fetchCreditsSync(ProviderPipeline::STRATEGY_TIMEOUT_MS);
                QMetaObject::invokeMethod(this, [this, result, expectedGuard]() {
                    applyCodexCreditsFetchResult(result, expectedGuard);
                }, Qt::QueuedConnection);
            });
        } else {
            // Identity known; proceed immediately
            QHash<QString, QString> creditsEnv = cachedSystemEnv();
            if (m_codexAccountService) {
                QString managedHome = m_codexAccountService->activeManagedHomePath();
                if (!managedHome.isEmpty()) {
                    creditsEnv.insert("CODEX_HOME", managedHome);
                }
            }

            ++m_pendingCreditsRefresh;
            QtConcurrent::run(m_threadPool, [this, creditsEnv, expectedGuard]() {
                CodexCreditsFetcher fetcher(creditsEnv);
                auto result = fetcher.fetchCreditsSync(ProviderPipeline::STRATEGY_TIMEOUT_MS);
                QMetaObject::invokeMethod(this, [this, result, expectedGuard]() {
                    applyCodexCreditsFetchResult(result, expectedGuard);
                }, Qt::QueuedConnection);
            });
        }
    }

    for (const auto& id : ids) {
        refreshProvider(id);
    }
}

void UsageStore::clearCache() {
    const auto ids = ProviderRegistry::instance().providerIDs();
    m_snapshots.clear();
    m_errors.clear();
    m_connectionTests.clear();
    {
        QMutexLocker locker(&m_credentialCacheMutex);
        m_credentialCache.clear();
        m_credentialMissing.clear();
    }
    m_snapshotDataCache.clear();
    m_snapshotRevision++;
    emit snapshotRevisionChanged();
    // Batch emit connection test changes to avoid signal storm
    for (const auto& id : ids) {
        emit providerConnectionTestChanged(id);
    }
}

void UsageStore::refreshProvider(const QString& providerId) {
    // Ensure count is sane when called individually (outside doRefresh).
    if (m_pendingRefreshes <= 0) {
        m_batchRefreshInProgress = false;
        if (!m_isRefreshing) {
            m_isRefreshing = true;
            emit refreshingChanged();
        }
    }
    m_pendingRefreshes++;

    auto* provider = ProviderRegistry::instance().provider(providerId);
    if (!provider) {
        m_pendingRefreshes--;
        if (m_pendingRefreshes <= 0) {
            m_batchRefreshInProgress = false;
            m_isRefreshing = false;
            if (m_batchUpdater) {
                m_batchUpdater->endBatch();
            } else {
                emit snapshotRevisionChanged();
                emit refreshingChanged();
            }
        }
        return;
    }

    // buildFetchContextForProvider is moved into the worker thread to avoid
    // blocking the main thread with WinCred API calls on cache miss.
    QtConcurrent::run(m_threadPool, [this, providerId, provider]() {
        ProviderFetchContext ctx = buildFetchContextForProvider(providerId);

        if (!isSourceModeAllowed(providerId, ctx.sourceMode)) {
            ProviderFetchResult errResult;
            errResult.success = false;
            errResult.errorMessage = QString("unsupported source mode: %1")
                .arg(sourceModeToString(ctx.sourceMode));
            QMetaObject::invokeMethod(this, [this, providerId, errResult]() {
                m_errors[providerId] = errResult.errorMessage;
                emit errorOccurred(providerId, providerError(providerId));
                m_pendingRefreshes--;
                if (m_pendingRefreshes <= 0) {
                    m_batchRefreshInProgress = false;
                    m_isRefreshing = false;
                    if (m_batchUpdater) {
                        m_batchUpdater->endBatch();
                    } else {
                        emit snapshotRevisionChanged();
                        emit refreshingChanged();
                    }
                }
            }, Qt::QueuedConnection);
            return;
        }

        ProviderFetchResult result;
        // Use ProviderRuntime if available (GenericRuntime is a thin wrapper around Pipeline)
        bool usedRuntime = false;
        if (ProviderRuntimeManager* rtMgr = ProviderRuntimeManager::instance()) {
            if (IProviderRuntime* runtime = rtMgr->runtimeFor(providerId)) {
                if (runtime->state() == RuntimeState::Running) {
                    result = runtime->fetch(ctx);
                    usedRuntime = true;
                }
            }
        }

        if (!usedRuntime) {
            // Fall back to direct pipeline execution for backward compatibility
            ProviderPipeline pipeline;
            result = pipeline.executeProvider(provider, ctx);
        }

        QMetaObject::invokeMethod(this, [this, providerId, result]() {
            PERF_PROBE("refreshProvider_callback", 2000);
            m_lastFetchAttempts[providerId] = result.attempts;
            if (providerId == "codex") {
                emit codexFetchAttemptsChanged();
            }
            if (result.dashboard.has_value()) {
                m_dashboardData[providerId] = result.dashboard->toVariantMap();
            } else {
                m_dashboardData.remove(providerId);
            }

            if (result.success) {
                m_snapshots[providerId] = result.usage;
                m_errors.remove(providerId);
                if (m_historyStore) {
                    m_historyStore->recordSample(providerId, result.usage);
                }

                auto sessionWindow = result.usage.primary;
                if (!sessionWindow.has_value() && result.usage.secondary.has_value()) {
                    sessionWindow = result.usage.secondary;
                }
                if (sessionWindow.has_value()) {
                    double currentRemaining = sessionWindow->remainingPercent();
                    auto prevRemaining = m_lastKnownSessionRemaining.value(providerId);
                    auto t = SessionQuotaNotificationLogic::transition(prevRemaining, currentRemaining);
                    bool notificationsEnabled = m_settingsStore
                        ? m_settingsStore->sessionQuotaNotificationsEnabled()
                        : true;
                    if (t != SessionQuotaTransition::None && notificationsEnabled) {
                        QString name = providerDisplayName(providerId);
                        SessionQuotaNotifier::post(t, name);
                    }
                    m_lastKnownSessionRemaining[providerId] = currentRemaining;
                    if (providerId == "codex" && !result.sourceLabel.isEmpty() &&
                        m_lastKnownSessionWindowSource != result.sourceLabel) {
                        QString label = result.sourceLabel;
                        bool hasAttachedDashboard = result.dashboard.has_value()
                            && result.dashboard->toVariantMap().value("visibility", "hidden").toString() == "attached";
                        if (hasAttachedDashboard) {
                            label += " + openai-web";
                        }
                        m_lastKnownSessionWindowSource = label;
                        emit lastKnownSessionWindowSourceChanged();
                    }
                }

                // Codex-specific: refresh credits after successful usage fetch.
                // This MUST be async — fetchCreditsSync blocks the main thread for 600-1400ms.
                if (providerId == "codex") {
                    auto guard = currentCodexAccountRefreshGuard();
                    // If doRefresh already scheduled an async credits fetch, avoid duplicate work.
                    if (m_pendingCreditsRefresh == 0) {
                        QString accountKey = currentCodexAccountKey();
                        bool cacheFresh = !accountKey.isEmpty() &&
                            m_codexCreditsCache.accountKey == accountKey &&
                            m_codexCreditsCache.updatedAt.isValid() &&
                            m_codexCreditsCache.updatedAt.secsTo(QDateTime::currentDateTime()) < 300;
                        if (!cacheFresh) {
                            QHash<QString, QString> creditsEnv = cachedSystemEnv();
                            if (m_codexAccountService) {
                                QString managedHome = m_codexAccountService->activeManagedHomePath();
                                if (!managedHome.isEmpty()) {
                                    creditsEnv.insert("CODEX_HOME", managedHome);
                                }
                            }
                            ++m_pendingCreditsRefresh;
                            QtConcurrent::run(m_threadPool, [this, creditsEnv, guard]() {
                                CodexCreditsFetcher fetcher(creditsEnv);
                                auto result = fetcher.fetchCreditsSync(ProviderPipeline::STRATEGY_TIMEOUT_MS);
                                QMetaObject::invokeMethod(this, [this, result, guard]() {
                                    applyCodexCreditsFetchResult(result, guard);
                                }, Qt::QueuedConnection);
                            });
                        }
                    }
                }
            } else {
                m_errors[providerId] = result.errorMessage;
                emit errorOccurred(providerId, providerError(providerId));
            }

            // Granular cache eviction: only remove the entry for this provider
            // instead of clearing the entire cache.
            m_snapshotDataCache.remove(providerId);
            if (m_batchUpdater) {
                m_batchUpdater->markDirty(providerId);
            } else {
                emit snapshotChanged(providerId);
            }

            m_pendingRefreshes--;
            if (m_pendingRefreshes <= 0 && m_pendingCreditsRefresh <= 0) {
                m_batchRefreshInProgress = false;
                m_isRefreshing = false;
                if (m_batchUpdater) {
                    m_batchUpdater->endBatch();
                } else {
                    m_snapshotRevision++;
                    m_snapshotDataCache.clear();
                    emit snapshotRevisionChanged();
                    emit refreshingChanged();
                }
            }
        }, Qt::QueuedConnection);
    });
}

void UsageStore::startAutoRefresh(int intervalMinutes) {
    m_refreshTimer.start(intervalMinutes * 60 * 1000);
}

void UsageStore::stopAutoRefresh() {
    m_refreshTimer.stop();
}

QString UsageStore::error(const QString& providerId) const {
    return Localization::providerError(m_errors.value(providerId, {}));
}

QString UsageStore::providerError(const QString& providerId) const {
    return Localization::providerError(m_errors.value(providerId, {}));
}

QStringList UsageStore::allProviderIDs() const {
    return ProviderRegistry::instance().providerIDs();
}

QVariantList UsageStore::utilizationChartData(const QString& providerId, const QString& seriesName) const {
    if (!m_historyStore) return {};
    return m_historyStore->chartData(providerId, seriesName);
}

QVariantList UsageStore::providerList() const {
    PERF_PROBE("providerList", 5000);
    if (m_providerListCacheValid) {
        return m_providerListCache;
    }
    QVariantList list;
    auto ids = ProviderRegistry::instance().providerIDs();
    
    // Sort by provider order from settings if available
    if (m_settingsStore) {
        QStringList order = m_settingsStore->providerOrder();
        if (!order.isEmpty()) {
            std::sort(ids.begin(), ids.end(), [&](const QString& a, const QString& b) {
                int idxA = order.indexOf(a);
                int idxB = order.indexOf(b);
                if (idxA == -1 && idxB == -1) return a < b;
                if (idxA == -1) return false;
                if (idxB == -1) return true;
                return idxA < idxB;
            });
        }
    }
    
    for (const auto& id : ids) {
        auto* prov = ProviderRegistry::instance().provider(id);
        auto desc = ProviderRegistry::instance().descriptor(id);
        QVariantMap entry;
        entry["id"] = id;
        entry["enabled"] = ProviderRegistry::instance().isProviderEnabled(id);
        if (prov) {
            entry["name"] = desc.has_value() ? desc->metadata.displayName : prov->displayName();
            entry["sessionLabel"] = Localization::providerLabel(
                desc.has_value() ? desc->metadata.sessionLabel : prov->sessionLabel());
            entry["weeklyLabel"] = Localization::providerLabel(
                desc.has_value() ? desc->metadata.weeklyLabel : prov->weeklyLabel());
            entry["supportsCredits"] = desc.has_value() ? desc->metadata.supportsCredits : prov->supportsCredits();
            entry["dashboardURL"] = desc.has_value() ? desc->metadata.dashboardURL : prov->dashboardURL();
            entry["statusPageURL"] = desc.has_value() ? desc->metadata.statusPageURL : prov->statusPageURL();
            entry["brandColor"] = prov->brandColor();
        } else {
            entry["name"] = id;
            entry["sessionLabel"] = Localization::providerLabel("Session");
            entry["weeklyLabel"] = Localization::providerLabel("Weekly");
            entry["supportsCredits"] = false;
        }
        
        // Add usage data if available
        auto snapIt = m_snapshots.find(id);
        if (snapIt != m_snapshots.end()) {
            const auto& snap = snapIt.value();
            QVariantMap usage;
            if (snap.primary.has_value()) {
                usage["percent"] = snap.primary->usedPercent;
                usage["remaining"] = 100.0 - snap.primary->usedPercent;
            }
            entry["usage"] = usage;
        }
        
        // Add status
        auto statusIt = m_providerStatuses.find(id);
        if (statusIt != m_providerStatuses.end()) {
            entry["status"] = statusIt.value().value("state", "unknown").toString();
        }
        
        list.append(entry);
    }
    m_providerListCacheValid = true;
    m_providerListCache = list;
    return list;
}

void UsageStore::moveProvider(int fromIndex, int toIndex) {
    if (!m_settingsStore) return;
    if (fromIndex == toIndex) return;
    
    auto list = providerList();
    if (fromIndex < 0 || fromIndex >= list.size()) return;
    if (toIndex < 0 || toIndex >= list.size()) return;
    
    QStringList order = m_settingsStore->providerOrder();
    if (order.isEmpty()) {
        // Initialize order from current list
        for (const auto& item : list) {
            order.append(item.toMap().value("id").toString());
        }
    }
    
    if (fromIndex < order.size() && toIndex < order.size()) {
        order.move(fromIndex, toIndex);
        m_settingsStore->setProviderOrder(order);
        emit providerIDsChanged();
    }
}

QVariantMap UsageStore::providerDescriptorData(const QString& id) const {
    PERF_PROBE("providerDescriptorData", 1000);
    QVariantMap data;
    auto desc = ProviderRegistry::instance().descriptor(id);
    if (!desc.has_value()) return data;
    data["id"] = desc->id;
    data["displayName"] = desc->metadata.displayName;
    data["sessionLabel"] = Localization::providerLabel(desc->metadata.sessionLabel);
    data["weeklyLabel"] = Localization::providerLabel(desc->metadata.weeklyLabel);
    data["dashboardURL"] = desc->metadata.dashboardURL;
    data["subscriptionDashboardURL"] = desc->metadata.subscriptionDashboardURL;
    data["statusPageURL"] = desc->metadata.statusPageURL;
    data["supportsCredits"] = desc->metadata.supportsCredits;
    data["cliName"] = desc->metadata.cliName;
    data["enabled"] = ProviderRegistry::instance().isProviderEnabled(id);
    data["settingsFields"] = providerSettingsFields(id);
    auto* prov = ProviderRegistry::instance().provider(id);
    if (prov) {
        data["brandColor"] = prov->brandColor();
    }
    QVariantList modes;
    for (const auto& mode : desc->fetchPlan.allowedSourceModes) modes.append(mode);
    data["sourceModes"] = modes;
    return data;
}

QVariantList UsageStore::providerSettingsFields(const QString& id) const {
    PERF_PROBE("providerSettingsFields", 1000);
    QVariantList list;
    auto* provider = ProviderRegistry::instance().provider(id);
    if (!provider) return list;
    auto descriptors = provider->settingsDescriptors();
    for (const auto& d : descriptors) {
        QVariantMap field;
        field["key"] = d.key;
        field["label"] = Localization::providerSettingLabel(d.label);
        field["type"] = d.type;
        field["defaultValue"] = d.defaultValue;
        field["value"] = d.sensitive
            ? QVariant()
            : (m_settingsStore ? m_settingsStore->providerSetting(id, d.key, d.defaultValue) : d.defaultValue);
        field["credentialTarget"] = d.credentialTarget;
        field["envVar"] = d.envVar;
        field["placeholder"] = d.placeholder;
        field["helpText"] = d.helpText;
        field["multiline"] = d.multiline;
        field["sensitive"] = d.sensitive;
        if (d.sensitive) {
            field["secretStatus"] = providerSecretStatus(id, d.key);
        }
        QVariantList options;
        for (const auto& option : d.options) {
            QVariantMap opt;
            opt["value"] = option.value;
            opt["label"] = Localization::providerSettingLabel(option.label);
            options.append(opt);
        }
        field["options"] = options;
        list.append(field);
    }
    return list;
}

void UsageStore::setProviderSetting(const QString& providerId, const QString& key, const QVariant& value) {
    auto descriptor = settingDescriptor(providerId, key);
    if (descriptor.has_value() && descriptor->sensitive) {
        return;
    }
    if (!m_settingsStore) {
        return;
    }
    m_settingsStore->setProviderSetting(providerId, key, value);
}

std::optional<ProviderSettingsDescriptor> UsageStore::settingDescriptor(const QString& providerId,
                                                                        const QString& key) const
{
    auto* provider = ProviderRegistry::instance().provider(providerId);
    if (!provider) return std::nullopt;
    for (const auto& descriptor : provider->settingsDescriptors()) {
        if (descriptor.key == key) return descriptor;
    }
    return std::nullopt;
}

QVariantMap UsageStore::providerSecretStatus(const QString& providerId, const QString& key) const {
    PERF_PROBE("providerSecretStatus", 500);
    QVariantMap status;
    status["configured"] = false;
    status["source"] = "none";

    auto descriptor = settingDescriptor(providerId, key);
    if (!descriptor.has_value() || !descriptor->sensitive) return status;

    status["envVar"] = descriptor->envVar;
    status["credentialTarget"] = descriptor->credentialTarget;

    // Check env var — use cached snapshot to avoid QProcessEnvironment::systemEnvironment()
    // on the main thread. Rebuild once on first access; tests call rebuildSystemEnvCache()
    // after qputenv() to pick up new variables.
    if (!descriptor->envVar.isEmpty()) {
        const auto& cachedEnv = cachedSystemEnv();
        if (cachedEnv.contains(descriptor->envVar)) {
            status["configured"] = true;
            status["source"] = "env";
            status["readOnly"] = true;
            return status;
        }
    }

    // Use cached credential state to avoid blocking main thread with WinCred API.
    // On cache miss, fall back to ProviderCredentialStore::exists() since this is
    // needed for correct operation. The startup preloadCredentials() call populates
    // the cache so this fallback rarely fires in production.
    bool credentialExists = false;
    if (!descriptor->credentialTarget.isEmpty()) {
        QMutexLocker locker(&m_credentialCacheMutex);
        if (m_credentialCache.contains(descriptor->credentialTarget)) {
            credentialExists = true;
        } else if (!m_credentialMissing.contains(descriptor->credentialTarget)) {
            // Cache miss — check synchronously and cache the result
            locker.unlock();
            credentialExists = ProviderCredentialStore::exists(descriptor->credentialTarget);
            locker.relock();
            if (credentialExists) {
                // Don't cache the secret data here (only the existence);
                // the full read will happen in buildFetchContextForProvider on the worker thread.
                // But mark it as not-missing so we don't keep re-checking.
                m_credentialMissing[descriptor->credentialTarget] = !credentialExists;
            } else {
                m_credentialMissing[descriptor->credentialTarget] = true;
            }
        }
    }

    if (credentialExists) {
        status["configured"] = true;
        status["source"] = "credential";
    } else if (descriptor->credentialTarget.isEmpty()
               && m_settingsStore
               && m_settingsStore->providerSetting(providerId, key).isValid()
               && !m_settingsStore->providerSetting(providerId, key).toString().isEmpty()) {
        status["configured"] = true;
        status["source"] = "settings";
    }
    return status;
}

bool UsageStore::setProviderSecret(const QString& providerId,
                                   const QString& key,
                                   const QString& value)
{
    auto descriptor = settingDescriptor(providerId, key);
    if (!descriptor.has_value() || !descriptor->sensitive) {
        return false;
    }

    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) return false;

    if (!descriptor->credentialTarget.isEmpty()) {
        // Move WinCred write to background thread to avoid blocking UI
        QString target = descriptor->credentialTarget;
        QtConcurrent::run(m_threadPool, [this, target, trimmed, providerId]() {
            bool ok = ProviderCredentialStore::write(target, {}, trimmed.toUtf8());
            if (ok) {
                QMutexLocker locker(&m_credentialCacheMutex);
                m_credentialCache[target] = {trimmed.toUtf8(), QDateTime::currentDateTime()};
                m_credentialMissing.remove(target);
            }
            QMetaObject::invokeMethod(this, [this, providerId]() {
                emit providerConnectionTestChanged(providerId);
            }, Qt::QueuedConnection);
        });
        return true; // Optimistic success
    } else {
        // Fall back to settings store when no credential target is configured
        if (m_settingsStore) {
            m_settingsStore->setProviderSetting(providerId, key, trimmed);
        }
        emit providerConnectionTestChanged(providerId);
        return true;
    }
}

bool UsageStore::clearProviderSecret(const QString& providerId, const QString& key) {
    auto descriptor = settingDescriptor(providerId, key);
    if (!descriptor.has_value() || !descriptor->sensitive) {
        return false;
    }

    if (!descriptor->credentialTarget.isEmpty()) {
        // Move WinCred remove to background thread to avoid blocking UI
        QString target = descriptor->credentialTarget;
        QtConcurrent::run(m_threadPool, [this, target, providerId]() {
            ProviderCredentialStore::remove(target);
            {
                QMutexLocker locker(&m_credentialCacheMutex);
                m_credentialCache.remove(target);
                m_credentialMissing[target] = true;
            }
            QMetaObject::invokeMethod(this, [this, providerId]() {
                emit providerConnectionTestChanged(providerId);
            }, Qt::QueuedConnection);
        });
        return true; // Optimistic success
    } else {
        // Fall back to settings store when no credential target is configured
        if (m_settingsStore) {
            m_settingsStore->setProviderSetting(providerId, key, QVariant());
        }
        emit providerConnectionTestChanged(providerId);
        return true;
    }
}

void UsageStore::setProviderConnectionTest(const QString& providerId, const QVariantMap& state) {
    m_connectionTests[providerId] = state;
    emit providerConnectionTestChanged(providerId);
}

QVariantMap UsageStore::providerConnectionTest(const QString& providerId) const {
    return m_connectionTests.value(providerId, QVariantMap{
        {"state", "idle"},
        {"message", ""},
        {"details", ""},
        {"startedAt", 0},
        {"finishedAt", 0},
        {"durationMs", 0}
    });
}

void UsageStore::testProviderConnection(const QString& providerId) {
    const qint64 startedAt = QDateTime::currentDateTime().toMSecsSinceEpoch();
    auto failNow = [&](const QString& message, const QString& details = QString()) {
        const qint64 finishedAt = QDateTime::currentDateTime().toMSecsSinceEpoch();
        setProviderConnectionTest(providerId, {
            {"state", "failed"},
            {"message", message},
            {"details", details.isEmpty() ? message : details},
            {"startedAt", startedAt},
            {"finishedAt", finishedAt},
            {"durationMs", finishedAt - startedAt}
        });
    };

    auto* provider = ProviderRegistry::instance().provider(providerId);
    if (!provider) {
        failNow("Unknown provider");
        return;
    }

    ProviderFetchContext ctx = buildFetchContextForProvider(providerId);
    ctx.allowInteractiveAuth = false;
    if (!isSourceModeAllowed(providerId, ctx.sourceMode)) {
        failNow(QString("Unsupported source mode: %1").arg(sourceModeToString(ctx.sourceMode)));
        return;
    }

    qDebug() << "[TestConnection] Starting test for provider:" << providerId;
    setProviderConnectionTest(providerId, {
        {"state", "testing"},
        {"message", "Testing connection..."},
        {"details", ""},
        {"startedAt", startedAt},
        {"finishedAt", 0},
        {"durationMs", 0}
    });
    QtConcurrent::run(m_threadPool, [this, providerId, provider, ctx, startedAt]() {
        ProviderPipeline pipeline;
        ProviderFetchResult result = pipeline.executeProvider(provider, ctx);
        QMetaObject::invokeMethod(this, [this, providerId, result, startedAt]() {
            PERF_PROBE("testProviderConnection_callback", 2000);
            m_lastFetchAttempts[providerId] = result.attempts;
            if (providerId == "codex") {
                emit codexFetchAttemptsChanged();
            }
            if (result.dashboard.has_value()) {
                m_dashboardData[providerId] = result.dashboard->toVariantMap();
            } else {
                m_dashboardData.remove(providerId);
            }
            const qint64 finishedAt = QDateTime::currentDateTime().toMSecsSinceEpoch();
            qDebug() << "[TestConnection] Provider:" << providerId << "success:" << result.success << "error:" << result.errorMessage;
            if (result.success) {
                m_snapshots[providerId] = result.usage;
                m_errors.remove(providerId);
                m_snapshotRevision++;
                m_snapshotDataCache.clear();
                emit snapshotRevisionChanged();
                emit snapshotChanged(providerId);
                setProviderConnectionTest(providerId, {
                    {"state", "succeeded"},
                    {"message", "Connection OK"},
                    {"details", ""},
                    {"startedAt", startedAt},
                    {"finishedAt", finishedAt},
                    {"durationMs", finishedAt - startedAt}
                });
            } else {
                QString message = result.errorMessage.trimmed();
                if (message.isEmpty()) message = "Connection failed";
                setProviderConnectionTest(providerId, {
                    {"state", "failed"},
                    {"message", message},
                    {"details", result.errorMessage},
                    {"startedAt", startedAt},
                    {"finishedAt", finishedAt},
                    {"durationMs", finishedAt - startedAt}
                });
            }
        }, Qt::QueuedConnection);
    });
}

void UsageStore::setProviderLoginState(const QString& providerId, const QVariantMap& state) {
    m_loginStates[providerId] = state;
    emit providerLoginStateChanged(providerId);
}

QVariantMap UsageStore::providerLoginState(const QString& providerId) const {
    return m_loginStates.value(providerId, QVariantMap{{"state", "idle"}});
}

void UsageStore::startProviderLogin(const QString& providerId) {
    if (providerId != "copilot") {
        setProviderLoginState(providerId, {{"state", "failed"}, {"message", "Login is not available for this provider"}});
        return;
    }

    auto cancelFlag = QSharedPointer<QAtomicInt>::create(0);
    m_loginCancelFlags[providerId] = cancelFlag;
    setProviderLoginState(providerId, {{"state", "starting"}, {"message", "Requesting device code..."}});

    QtConcurrent::run(m_threadPool, [this, providerId, cancelFlag]() {
        QUrlQuery deviceBody;
        deviceBody.addQueryItem("client_id", "Iv1.b507a08c87ecfe98");
        deviceBody.addQueryItem("scope", "read:user");

        QJsonObject deviceResp = NetworkManager::instance().postFormSync(
            QUrl("https://github.com/login/device/code"),
            deviceBody.toString(QUrl::FullyEncoded).toUtf8(),
            {{"Accept", "application/json"}});

        const QString deviceCode = deviceResp.value("device_code").toString();
        const QString userCode = deviceResp.value("user_code").toString();
        const QString verificationUri = deviceResp.value("verification_uri").toString(
            "https://github.com/login/device");
        int interval = deviceResp.value("interval").toInt(5);
        const int expiresIn = deviceResp.value("expires_in").toInt(900);

        if (deviceCode.isEmpty() || userCode.isEmpty()) {
            QMetaObject::invokeMethod(this, [this, providerId]() {
                setProviderLoginState(providerId, {{"state", "failed"}, {"message", "Could not start GitHub device login"}});
            }, Qt::QueuedConnection);
            return;
        }

        QMetaObject::invokeMethod(this, [this, providerId, userCode, verificationUri, expiresIn]() {
            setProviderLoginState(providerId, {
                {"state", "verification"},
                {"message", "Enter the code in GitHub to authorize Copilot."},
                {"userCode", userCode},
                {"verificationUri", verificationUri},
                {"expiresIn", expiresIn}
            });
        }, Qt::QueuedConnection);

        const QDateTime deadline = QDateTime::currentDateTimeUtc().addSecs(expiresIn);
        while (QDateTime::currentDateTimeUtc() < deadline) {
            if (cancelFlag->loadAcquire() != 0) {
                QMetaObject::invokeMethod(this, [this, providerId]() {
                    setProviderLoginState(providerId, {{"state", "cancelled"}, {"message", "Login cancelled"}});
                }, Qt::QueuedConnection);
                return;
            }
            QThread::sleep(static_cast<unsigned long>(qMax(1, interval)));

            QUrlQuery tokenBody;
            tokenBody.addQueryItem("client_id", "Iv1.b507a08c87ecfe98");
            tokenBody.addQueryItem("device_code", deviceCode);
            tokenBody.addQueryItem("grant_type", "urn:ietf:params:oauth:grant-type:device_code");

            QJsonObject tokenResp = NetworkManager::instance().postFormSync(
                QUrl("https://github.com/login/oauth/access_token"),
                tokenBody.toString(QUrl::FullyEncoded).toUtf8(),
                {{"Accept", "application/json"}});

            const QString errorType = tokenResp.value("error").toString();
            if (errorType == "authorization_pending") continue;
            if (errorType == "slow_down") {
                interval += 5;
                continue;
            }
            if (!errorType.isEmpty()) {
                QMetaObject::invokeMethod(this, [this, providerId, errorType]() {
                    setProviderLoginState(providerId, {{"state", "failed"}, {"message", errorType}});
                }, Qt::QueuedConnection);
                return;
            }

            const QString accessToken = tokenResp.value("access_token").toString();
            if (!accessToken.isEmpty()) {
                ProviderCredentialStore::write("com.codexbar.oauth.copilot", {}, accessToken.toUtf8());
                QMetaObject::invokeMethod(this, [this, providerId]() {
                    setProviderLoginState(providerId, {{"state", "succeeded"}, {"message", "Copilot login complete"}});
                    testProviderConnection(providerId);
                }, Qt::QueuedConnection);
                return;
            }
        }

        QMetaObject::invokeMethod(this, [this, providerId]() {
            setProviderLoginState(providerId, {{"state", "failed"}, {"message", "GitHub device login expired"}});
        }, Qt::QueuedConnection);
    });
}

void UsageStore::cancelProviderLogin(const QString& providerId) {
    auto flag = m_loginCancelFlags.value(providerId);
    if (flag) flag->storeRelease(1);
}

void UsageStore::setProviderStatus(const QString& providerId, const QVariantMap& status) {
    m_providerStatuses[providerId] = status;
    m_statusRevision++;
    m_providerListCacheValid = false;
    emit statusRevisionChanged();
    emit providerStatusChanged(providerId);
}

QVariantMap UsageStore::providerStatus(const QString& providerId) const {
    return m_providerStatuses.value(providerId, QVariantMap{{"state", "unknown"}});
}

QVariantMap UsageStore::providerUsageSnapshot(const QString& providerId) const {
    QVariantMap result;
    auto it = m_snapshots.find(providerId);
    if (it == m_snapshots.end()) return result;

    const bool showUsedPercent = m_settingsStore ? m_settingsStore->usageBarsShowUsed() : false;
    const bool isDetailProvider = (providerId == "deepseek" || providerId == "warp" || providerId == "kilo" || providerId == "abacus");

    auto metricMap = [&](const RateWindow& rw) {
        QVariantMap metric;
        const double remaining = rw.remainingPercent();
        metric["percent"] = showUsedPercent ? rw.usedPercent : remaining;
        metric["usedPercent"] = rw.usedPercent;
        metric["remaining"] = remaining;
        metric["displayIsUsed"] = showUsedPercent;
        if (rw.resetsAt.has_value() && rw.resetsAt.value().isValid()) {
            metric["resetsAt"] = rw.resetsAt.value().toString(Qt::ISODate);
        }
        return metric;
    };

    const auto& snap = it.value();
    if (snap.primary.has_value()) {
        result["primary"] = metricMap(*snap.primary);
        if (isDetailProvider && snap.primary->resetDescription.has_value()) {
            QString detail = snap.primary->resetDescription.value().trimmed();
            if (!detail.isEmpty()) result["detail"] = detail;
        }
    }
    if (snap.secondary.has_value()) {
        result["secondary"] = metricMap(*snap.secondary);
    }
    if (snap.tertiary.has_value()) {
        result["tertiary"] = metricMap(*snap.tertiary);
    }
    return result;
}

void UsageStore::refreshProviderStatuses() {
    const bool enabled = m_settingsStore ? m_settingsStore->statusChecksEnabled() : true;
    if (!enabled) return;

    const auto ids = ProviderRegistry::instance().providerIDs();
    for (const auto& id : ids) {
        auto desc = ProviderRegistry::instance().descriptor(id);
        if (!desc.has_value() || desc->metadata.statusPageURL.isEmpty()) continue;

        const QString endpoint = statusEndpointFor(desc->metadata.statusPageURL);
        if (endpoint.isEmpty()) {
            setProviderStatus(id, {{"state", "unknown"}});
            continue;
        }

        QtConcurrent::run(m_threadPool, [this, id, endpoint]() {
            QJsonObject json = NetworkManager::instance().getJsonSync(QUrl(endpoint), {}, 8000);
            QString state = "unknown";
            QString description;
            if (!json.isEmpty()) {
                QJsonObject status = json.value("status").toObject();
                state = mappedStatusFromIndicator(status.value("indicator").toString());
                description = status.value("description").toString();
            }
            QMetaObject::invokeMethod(this, [this, id, state, description]() {
                QVariantMap map;
                map["state"] = state;
                map["description"] = description;
                map["updatedAt"] = QDateTime::currentDateTime().toMSecsSinceEpoch();
                setProviderStatus(id, map);
            }, Qt::QueuedConnection);
        });
    }
}

// ============================================================================
// Codex Multi-Account Management
// ============================================================================

QVariantList UsageStore::codexAccounts() const
{
    QVariantList result;
    if (!m_codexAccountService) return result;

    auto accounts = m_codexAccountService->visibleAccounts();
    for (const auto& account : accounts) {
        QVariantMap map;
        map["id"] = account.id;
        map["displayName"] = account.displayName;
        map["email"] = account.email;
        map["workspaceLabel"] = account.workspaceLabel;
        map["isLive"] = account.isLive;
        map["isActive"] = account.isActive;
        map["storedAccountID"] = account.storedAccountID;
        result.append(map);
    }
    return result;
}

QVariantMap UsageStore::codexAccountState() const
{
    QVariantMap state;
    state["accounts"] = codexAccounts();
    state["activeAccountID"] = codexActiveAccountID();
    state["isAuthenticating"] = isCodexAuthenticating();
    state["isRemoving"] = isCodexRemoving();
    state["authenticatingAccountID"] = codexAuthenticatingAccountID();
    state["removingAccountID"] = codexRemovingAccountID();
    state["hasUnreadableStore"] = hasCodexUnreadableStore();

    QString authState = "idle";
    if (m_codexAccountService) {
        if (m_codexAccountService->isAuthenticating()) {
            authState = m_codexAccountService->authUserCode().isEmpty()
                ? QStringLiteral("starting")
                : QStringLiteral("verification");
        } else if (!m_codexAccountService->authError().isEmpty()) {
            authState = QStringLiteral("failed");
        } else if (!m_codexAccountService->authMessage().isEmpty()) {
            authState = QStringLiteral("succeeded");
        }

        state["authState"] = authState;
        state["authMessage"] = m_codexAccountService->authMessage();
        state["authError"] = m_codexAccountService->authError();
        state["verificationUri"] = m_codexAccountService->authVerificationUri();
        state["userCode"] = m_codexAccountService->authUserCode();
    } else {
        state["authState"] = authState;
        state["authMessage"] = QString();
        state["authError"] = QString();
        state["verificationUri"] = QString();
        state["userCode"] = QString();
    }

    return state;
}

QString UsageStore::codexActiveAccountID() const
{
    if (!m_codexAccountService) return "live-system";
    return m_codexAccountService->activeAccountID();
}

void UsageStore::setCodexActiveAccount(const QString& accountID)
{
    if (!m_codexAccountService) return;

    QString previousID = m_codexAccountService->activeAccountID();
    if (previousID == accountID) return;

    clearCodexOpenAIWebState();
    m_lastCodexRefreshGuard = {}; // force guard re-evaluation on next refresh

    m_codexAccountService->setActiveAccount(accountID);

    emit codexActiveAccountChanged(accountID);
    emit codexAccountsChanged();

    refreshProvider("codex");
}

bool UsageStore::addCodexAccount(const QString& email, const QString& homePath)
{
    if (!m_codexAccountService) return false;

    clearCodexOpenAIWebState();
    m_lastCodexRefreshGuard = {};

    if (email.isEmpty()) {
        return m_codexAccountService->authenticateNewAccount();
    }

    return m_codexAccountService->addAccount(email, homePath);
}

void UsageStore::cancelCodexAuthentication()
{
    if (!m_codexAccountService) return;
    m_codexAccountService->cancelAuthentication();
}

bool UsageStore::removeCodexAccount(const QString& accountID)
{
    if (!m_codexAccountService) return false;
    return m_codexAccountService->removeAccount(accountID);
}

bool UsageStore::reauthenticateCodexAccount(const QString& accountID)
{
    if (!m_codexAccountService) return false;
    return m_codexAccountService->reauthenticateAccount(accountID);
}

bool UsageStore::promoteCodexAccount(const QString& accountID)
{
    if (!m_codexAccountService) return false;

    // Clear state before promotion
    clearCodexOpenAIWebState();
    m_lastCodexRefreshGuard = {};

    bool ok = m_codexAccountService->promoteAccount(accountID);

    if (ok) {
        refreshProvider("codex");
    }

    return ok;
}

bool UsageStore::isCodexAuthenticating() const
{
    if (!m_codexAccountService) return false;
    return m_codexAccountService->isAuthenticating();
}

bool UsageStore::isCodexRemoving() const
{
    if (!m_codexAccountService) return false;
    return m_codexAccountService->isRemoving();
}

QString UsageStore::codexAuthenticatingAccountID() const
{
    if (!m_codexAccountService) return QString();
    return m_codexAccountService->authenticatingAccountID();
}

QString UsageStore::codexRemovingAccountID() const
{
    if (!m_codexAccountService) return QString();
    return m_codexAccountService->removingAccountID();
}

bool UsageStore::hasCodexUnreadableStore() const
{
    if (!m_codexAccountService) return false;
    return m_codexAccountService->hasUnreadableStore();
}

// ============================================================================
// Codex Account Refresh Guard (mirrors original CodexBar)
// ============================================================================

UsageStore::CodexAccountRefreshGuard UsageStore::currentCodexAccountRefreshGuard() const
{
    CodexAccountRefreshGuard guard;
    guard.accountKey = currentCodexAccountKey();

    if (!m_codexAccountService) {
        guard.source = "liveSystem";
        auto snap = snapshot("codex");
        if (snap.identity.has_value() && snap.identity->accountEmail.has_value()) {
            guard.identity = snap.identity->accountEmail.value().toLower();
        }
        return guard;
    }

    QString activeId = m_codexAccountService->activeAccountID();
    if (activeId.isEmpty() || activeId == "live-system") {
        guard.source = "liveSystem";
        auto snap = snapshot("codex");
        if (snap.identity.has_value() && snap.identity->accountEmail.has_value()) {
            guard.identity = snap.identity->accountEmail.value().toLower();
        }
    } else {
        guard.source = "managedAccount";
        guard.identity = activeId.toLower();
    }
    return guard;
}

bool UsageStore::shouldApplyCodexScopedNonUsageResult(const CodexAccountRefreshGuard& expectedGuard) const
{
    auto currentGuard = currentCodexAccountRefreshGuard();
    if (currentGuard.source != expectedGuard.source) return false;
    if (expectedGuard.identity.isEmpty()) return false;
    return currentGuard.identity == expectedGuard.identity;
}

UsageSnapshot UsageStore::waitForCodexSnapshot(const QDateTime& minimumUpdatedAt, int timeoutMs) const
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        if (NetworkManager::isShuttingDown()) break;
        auto snap = snapshot("codex");
        if (snap.updatedAt.isValid() && snap.updatedAt >= minimumUpdatedAt) {
            return snap;
        }
        // Short sleep for faster response; max 3s default instead of 6s.
        QThread::msleep(50);
    }
    return snapshot("codex");
}

// ============================================================================
// Codex Credits Cache & Consumer Projection
// ============================================================================

QString UsageStore::currentCodexAccountKey() const
{
    auto snap = snapshot("codex");
    if (snap.identity.has_value() && snap.identity->accountEmail.has_value()) {
        QString email = snap.identity->accountEmail.value().toLower().trimmed();
        QByteArray hashBytes = QCryptographicHash::hash(email.toUtf8(), QCryptographicHash::Sha256);
        return "codex:v1:email-hash:" + QString::fromLatin1(hashBytes.toHex());
    }
    if (snap.identity.has_value() && snap.identity->loginMethod.has_value()) {
        QString method = snap.identity->loginMethod.value().toLower().trimmed();
        QByteArray hashBytes = QCryptographicHash::hash(method.toUtf8(), QCryptographicHash::Sha256);
        return "codex:v1:method-hash:" + QString::fromLatin1(hashBytes.toHex());
    }
    return "codex:v1:unresolved";
}

std::optional<CreditsSnapshot> UsageStore::cachedCodexCredits() const
{
    if (m_codexCreditsCache.accountKey == currentCodexAccountKey()) {
        return m_codexCreditsCache.snapshot;
    }
    return std::nullopt;
}

QString UsageStore::codexCreditsError() const
{
    if (m_codexCreditsCache.accountKey == currentCodexAccountKey()) {
        return m_codexCreditsCache.lastError;
    }
    return QString();
}

void UsageStore::refreshCodexCredits(const CodexAccountRefreshGuard& expectedGuard)
{
    if (!isProviderEnabled("codex")) return;

    QString accountKey = currentCodexAccountKey();
    if (accountKey.isEmpty()) return;

    // Check cache freshness (5 minute cooldown)
    if (m_codexCreditsCache.accountKey == accountKey &&
        m_codexCreditsCache.updatedAt.isValid() &&
        m_codexCreditsCache.updatedAt.secsTo(QDateTime::currentDateTime()) < 300) {
        return;
    }

    m_codexCreditsRefreshing = true;

    // Stage 4: test injection point
    if (_test_codexCreditsLoaderOverride) {
        auto overrideResult = _test_codexCreditsLoaderOverride();
        CodexCreditsFetcher::FetchResult result;
        if (overrideResult.has_value()) {
            result.success = true;
            result.credits = overrideResult;
        } else {
            result.success = false;
            result.errorMessage = "test override returned no credits";
        }
        applyCodexCreditsFetchResult(result, expectedGuard);
        m_codexCreditsRefreshing = false;
        return;
    }

    // Build environment
    QHash<QString, QString> env = cachedSystemEnv();

    // If a managed account is active, point CODEX_HOME at its home path
    if (m_codexAccountService) {
        QString managedHome = m_codexAccountService->activeManagedHomePath();
        if (!managedHome.isEmpty()) {
            env.insert("CODEX_HOME", managedHome);
        }
    }

    CodexCreditsFetcher fetcher(env);
    auto result = fetcher.fetchCreditsSync(ProviderPipeline::STRATEGY_TIMEOUT_MS);
    applyCodexCreditsFetchResult(result, expectedGuard);

    m_codexCreditsRefreshing = false;
}

void UsageStore::applyCodexCreditsFetchResult(const CodexCreditsFetcher::FetchResult& result,
                                               const CodexAccountRefreshGuard& expectedGuard)
{
    // Stage 2: decrement pending counter
    if (m_pendingCreditsRefresh > 0) {
        --m_pendingCreditsRefresh;
    }

    // Stage 1: guard check — discard result if account changed during fetch
    if (!expectedGuard.isEmpty() && !shouldApplyCodexScopedNonUsageResult(expectedGuard)) {
        qDebug() << "[UsageStore] Discarding stale credits result (account changed during fetch)";
        // Still emit refreshingChanged if everything is done
        if (m_pendingRefreshes <= 0 && m_pendingCreditsRefresh <= 0 && m_isRefreshing) {
            m_isRefreshing = false;
            emit refreshingChanged();
        }
        return;
    }

    QString accountKey = currentCodexAccountKey();
    if (accountKey.isEmpty()) {
        if (m_pendingRefreshes <= 0 && m_pendingCreditsRefresh <= 0 && m_isRefreshing) {
            m_isRefreshing = false;
            emit refreshingChanged();
        }
        return;
    }

    QDateTime now = QDateTime::currentDateTime();

    if (result.success && result.credits.has_value()) {
        m_codexCreditsCache.snapshot = result.credits;
        m_codexCreditsCache.accountKey = accountKey;
        m_codexCreditsCache.updatedAt = now;
        m_codexCreditsCache.failureStreak = 0;
        m_codexCreditsCache.lastError.clear();
        emit codexCreditsChanged();

        // Stage 4: plan history backfill if usage snapshot is stale
        if (m_historyStore) {
            auto snap = snapshot("codex");
            if (snap.updatedAt.isValid()) {
                m_historyStore->recordSample("codex", snap, accountKey);
            }
        }
    } else {
        QString errorMsg = result.errorMessage;
        if (errorMsg.isEmpty()) {
            errorMsg = "Codex credits are still loading; will retry shortly.";
        }

        m_codexCreditsCache.failureStreak++;
        m_codexCreditsCache.lastError = errorMsg;
        m_codexCreditsCache.accountKey = accountKey;
        m_codexCreditsCache.updatedAt = now;

        // Emit only if we have no stale cached value to show
        if (!m_codexCreditsCache.snapshot.has_value()) {
            emit codexCreditsChanged();
        }
    }

    // Stage 2: signal refreshing completion if all work is done
    if (m_pendingRefreshes <= 0 && m_pendingCreditsRefresh <= 0 && m_isRefreshing) {
        m_isRefreshing = false;
        emit refreshingChanged();
    }
}

QVariantMap UsageStore::codexConsumerProjectionData() const
{
    PERF_PROBE("codexConsumerProjectionData", 2000);
    QVariantMap m;
    auto snap = snapshot("codex");

    CodexConsumerProjection::Context ctx;
    ctx.snapshot = snap;
    ctx.rawUsageError = m_errors.value("codex");
    ctx.now = QDateTime::currentDateTime();

    if (m_codexCreditsCache.snapshot.has_value() &&
        m_codexCreditsCache.accountKey == currentCodexAccountKey()) {
        ctx.credits = const_cast<CreditsSnapshot*>(&m_codexCreditsCache.snapshot.value());
        ctx.rawCreditsError = m_codexCreditsCache.lastError;
    }

    auto projection = CodexConsumerProjection::make(
        CodexConsumerProjection::Surface::LiveCard, ctx);

    m["dashboardVisibility"] = static_cast<int>(projection.dashboardVisibility);
    m["menuBarFallback"] = static_cast<int>(projection.menuBarFallback);
    m["hasExhaustedRateLane"] = CodexConsumerProjection::hasExhaustedRateLane(projection);

    // Credits
    if (projection.credits.has_value() && projection.credits->snapshot.has_value()) {
        m["hasCredits"] = true;
        m["creditsRemaining"] = projection.credits->snapshot->remaining;
        m["creditsError"] = projection.credits->userFacingError;
    } else {
        m["hasCredits"] = false;
    }

    // Consumer Projection: supplemental metrics
    QVariantList supplementalMetrics;
    for (const auto& metric : projection.supplementalMetrics) {
        supplementalMetrics.append(static_cast<int>(metric));
    }
    m["supplementalMetrics"] = supplementalMetrics;

    // Consumer Projection: buy credits
    m["canShowBuyCredits"] = projection.canShowBuyCredits;

    // Consumer Projection: usage breakdown
    m["hasUsageBreakdown"] = projection.hasUsageBreakdown;

    // Consumer Projection: credits history
    m["hasCreditsHistory"] = projection.hasCreditsHistory;

    // Consumer Projection: plan utilization lanes
    QVariantList planLanes;
    for (const auto& lane : projection.planUtilizationLanes) {
        QVariantMap laneMap;
        laneMap["role"] = static_cast<int>(lane.role);
        laneMap["usedPercent"] = lane.window.usedPercent;
        laneMap["remainingPercent"] = lane.window.remainingPercent();
        if (lane.window.resetsAt.has_value())
            laneMap["resetsAt"] = lane.window.resetsAt->toMSecsSinceEpoch();
        if (lane.window.windowMinutes.has_value())
            laneMap["windowMinutes"] = *lane.window.windowMinutes;
        planLanes.append(laneMap);
    }
    m["planUtilizationLanes"] = planLanes;

    // Consumer Projection: user-facing errors
    QVariantMap userErrors;
    userErrors["usage"] = projection.userFacingErrors.usage;
    userErrors["credits"] = projection.userFacingErrors.credits;
    userErrors["dashboard"] = projection.userFacingErrors.dashboard;
    m["userFacingErrors"] = userErrors;

    return m;
}

QVariantList UsageStore::codexFetchAttempts() const
{
    QVariantList list;
    auto it = m_lastFetchAttempts.find("codex");
    if (it == m_lastFetchAttempts.end()) return list;
    for (const auto& attempt : it.value()) {
        list.append(attempt.toMap());
    }
    return list;
}

QString UsageStore::lastKnownSessionWindowSource() const
{
    return m_lastKnownSessionWindowSource;
}

void UsageStore::clearCodexOpenAIWebState()
{
    // Clear dashboard HTML cache
    CodexDashboardCache::clear();

    // Clear Codex-specific cached state
    m_codexCreditsCache = {};
    m_snapshots.remove("codex");
    m_errors.remove("codex");
    m_connectionTests.remove("codex");
    m_dashboardData.remove("codex");
    m_lastKnownSessionRemaining.remove("codex");
    m_lastKnownSessionWindowSource.clear();
    m_lastFetchAttempts.remove("codex");

    emit snapshotChanged("codex");
    m_snapshotDataCache.remove("codex");
    emit snapshotRevisionChanged();
    emit codexCreditsChanged();
    emit codexFetchAttemptsChanged();
}

void UsageStore::shutdown()
{
    stopAutoRefresh();
    if (m_threadPool) {
        m_threadPool->clear();
    }
    if (m_historyStore) {
        m_historyStore->stopSaveTimer();
    }
    NetworkManager::setShuttingDown(true);
    CostUsageScanner::setShuttingDown(true);
}

QVariantMap UsageStore::providerDashboardData(const QString& providerId) const
{
    return m_dashboardData.value(providerId);
}

// Token Account management implementations

QVariantList UsageStore::tokenAccountsForProvider(const QString& providerId) const
{
    QVariantList result;
    auto accounts = TokenAccountStore::instance()->accountsForProvider(providerId);
    for (const auto& acc : accounts) {
        result.append(acc.toVariantMap());
    }
    return result;
}

QString UsageStore::addTokenAccount(const QString& providerId, const QString& displayName, int sourceMode)
{
    TokenAccount account;
    account.providerId = providerId;
    account.displayName = displayName.isEmpty() ? providerId : displayName;
    account.sourceMode = static_cast<ProviderSourceMode>(sourceMode);
    account.visibility = AccountVisibility::Visible;
    account.createdAt = QDateTime::currentDateTimeUtc();
    account.lastUsedAt = account.createdAt;

    QString accountId = TokenAccountStore::instance()->addAccount(account);

    // If this is the first account for this provider, set it as default
    if (TokenAccountStore::instance()->accountCountForProvider(providerId) == 1) {
        TokenAccountStore::instance()->setDefaultAccountId(providerId, accountId);
    }

    return accountId;
}

bool UsageStore::removeTokenAccount(const QString& accountId)
{
    return TokenAccountStore::instance()->removeAccount(accountId);
}

bool UsageStore::setTokenAccountVisibility(const QString& accountId, int visibility)
{
    auto accOpt = TokenAccountStore::instance()->account(accountId);
    if (!accOpt.has_value()) return false;
    TokenAccount acc = accOpt.value();
    acc.visibility = static_cast<AccountVisibility>(visibility);
    return TokenAccountStore::instance()->updateAccount(accountId, acc);
}

bool UsageStore::setTokenAccountSourceMode(const QString& accountId, int sourceMode)
{
    auto accOpt = TokenAccountStore::instance()->account(accountId);
    if (!accOpt.has_value()) return false;
    TokenAccount acc = accOpt.value();
    acc.sourceMode = static_cast<ProviderSourceMode>(sourceMode);
    return TokenAccountStore::instance()->updateAccount(accountId, acc);
}

bool UsageStore::setDefaultTokenAccount(const QString& providerId, const QString& accountId)
{
    TokenAccountStore::instance()->setDefaultAccountId(providerId, accountId);
    return true;
}

// ============================================================================
// BatchUpdateController slots
// ============================================================================

void UsageStore::onBatchUpdateReady(const QStringList& providerIds)
{
    PERF_PROBE("onBatchUpdateReady", 2000);

    // 逐个发射 snapshotChanged（QML 中 ProviderDetailView 等需要这个粒度）
    for (const auto& id : providerIds) {
        emit snapshotChanged(id);
    }

    m_snapshotRevision++;
    for (const auto& id : providerIds) {
        m_snapshotDataCache.remove(id);
    }
    m_providerListCacheValid = false;
    emit snapshotRevisionChanged();
}

void UsageStore::onBatchFinished()
{
    emit refreshingChanged();
}

QString UsageStore::defaultTokenAccount(const QString& providerId) const
{
    return TokenAccountStore::instance()->defaultAccountId(providerId);
}
