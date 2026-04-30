#include "UsageStore.h"
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
#include "../util/UsagePaceText.h"
#include "../models/UsagePace.h"

#include <QDateTime>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>
#include <QUrlQuery>
#include <QtConcurrent>
#include <QFutureWatcher>

namespace {

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
{
    QObject::connect(&m_refreshTimer, &QTimer::timeout, this, &UsageStore::refresh);
    QObject::connect(&m_statusTimer, &QTimer::timeout, this, &UsageStore::refreshProviderStatuses);
    QObject::connect(m_pipeline, &ProviderPipeline::pipelineComplete,
                     this, [this](const ProviderFetchResult& /*result*/) {
    });
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

ProviderFetchContext UsageStore::buildFetchContextForProvider(const QString& providerId) const {
    ProviderFetchContext ctx;
    ctx.providerId = providerId;
    ctx.sourceMode = ProviderSourceMode::Auto;
    ctx.isAppRuntime = true;
    ctx.allowInteractiveAuth = false;
    ctx.networkTimeoutMs = ProviderPipeline::STRATEGY_TIMEOUT_MS;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const auto keys = env.keys();
    for (const auto& key : keys) {
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
                    auto stored = ProviderCredentialStore::read(descriptor.credentialTarget);
                    if (stored.has_value()) secret = QString::fromUtf8(stored.value()).trimmed();
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

    bool ok = false;
    int timeout = addSetting("networkTimeoutMs", ProviderPipeline::STRATEGY_TIMEOUT_MS).toInt(&ok);
    if (ok && timeout > 0) {
        ctx.networkTimeoutMs = timeout;
    }

    QString apiRegion = addSetting("apiRegion", "global").toString();
    if (providerId == "zai") {
        ctx.env["ZAI_API_REGION"] = apiRegion;
    }

    return ctx;
}

void UsageStore::updateProviderIDs() {
    m_providerIDs = ProviderRegistry::instance().enabledProviderIDs();
    emit providerIDsChanged();
}

QVariantMap UsageStore::snapshotData(const QString& id) const {
    auto snap = snapshot(id);
    auto* prov = ProviderRegistry::instance().provider(id);
    QVariantMap m;

    m["sessionLabel"] = Localization::providerLabel(prov ? prov->sessionLabel() : "Session");
    m["weeklyLabel"] = Localization::providerLabel(prov ? prov->weeklyLabel() : "Weekly");
    m["opusLabel"] = Localization::providerLabel(prov ? prov->opusLabel() : QString());
    m["supportsCredits"] = prov ? prov->supportsCredits() : false;
    m["displayName"] = providerDisplayName(id);

    if (snap.primary.has_value()) {
        m["primaryUsed"] = snap.primary->usedPercent;
        m["primaryRemaining"] = snap.primary->remainingPercent();
        if (snap.primary->resetsAt.has_value())
            m["primaryResetsAt"] = snap.primary->resetsAt->toMSecsSinceEpoch();
        if (snap.primary->resetDescription.has_value())
            m["primaryResetDesc"] = *snap.primary->resetDescription;
    } else {
        m["primaryUsed"] = 0.0;
        m["primaryRemaining"] = 100.0;
    }
    if (snap.secondary.has_value()) {
        m["secondaryUsed"] = snap.secondary->usedPercent;
        m["secondaryRemaining"] = snap.secondary->remainingPercent();
        m["hasSecondary"] = true;
        if (snap.secondary->resetsAt.has_value())
            m["secondaryResetsAt"] = snap.secondary->resetsAt->toMSecsSinceEpoch();
        if (snap.secondary->resetDescription.has_value())
            m["secondaryResetDesc"] = *snap.secondary->resetDescription;
    } else {
        m["secondaryUsed"] = 0.0;
        m["secondaryRemaining"] = 100.0;
        m["hasSecondary"] = false;
    }
    if (snap.tertiary.has_value()) {
        m["tertiaryUsed"] = snap.tertiary->usedPercent;
        m["tertiaryRemaining"] = snap.tertiary->remainingPercent();
        m["hasTertiary"] = true;
        if (snap.tertiary->resetDescription.has_value())
            m["tertiaryResetDesc"] = *snap.tertiary->resetDescription;
    } else {
        m["hasTertiary"] = false;
    }
    if (snap.identity.has_value()) {
        if (snap.identity->loginMethod.has_value())
            m["loginMethod"] = snap.identity->loginMethod.value();
    }

    bool hasUsage = snap.primary.has_value() || snap.secondary.has_value() || snap.tertiary.has_value();
    m["hasUsage"] = hasUsage;

    if (snap.providerCost.has_value()) {
        m["providerCostUsed"] = snap.providerCost->used;
        m["providerCostLimit"] = snap.providerCost->limit;
        m["providerCostCurrency"] = snap.providerCost->currencyCode;
        m["hasProviderCost"] = true;
    } else {
        m["hasProviderCost"] = false;
    }

    m["updatedAt"] = snap.updatedAt.toMSecsSinceEpoch();

    if (snap.zaiUsage.has_value()) {
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

    if (snap.openRouterUsage.has_value()) {
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

    if (snap.providerCost.has_value()) {
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

    QtConcurrent::run([this]() {
        CostUsageScanner scanner;

        QDate today = QDate::currentDate();
        QDate since = today.addDays(-29);

        // Pass empty to let scanner auto-detect via env vars
        CostUsageSnapshot claude = scanner.scanClaude({}, since, today);
        CostUsageSnapshot codex = scanner.scanCodex({}, since, today);

        // P1: Merge Pi agent session data (codex + claude portions)
        auto piResult = scanner.scanPi(since, today);

        CostUsageSnapshot combined;
        combined.updatedAt = QDateTime::currentDateTime();
        combined.sessionTokens = claude.sessionTokens + codex.sessionTokens
                               + piResult.codex.sessionTokens + piResult.claude.sessionTokens;
        combined.sessionCostUSD = claude.sessionCostUSD + codex.sessionCostUSD
                                + piResult.codex.sessionCostUSD + piResult.claude.sessionCostUSD;
        combined.last30DaysTokens = claude.last30DaysTokens + codex.last30DaysTokens
                                  + piResult.codex.last30DaysTokens + piResult.claude.last30DaysTokens;
        combined.last30DaysCostUSD = claude.last30DaysCostUSD + codex.last30DaysCostUSD
                                   + piResult.codex.last30DaysCostUSD + piResult.claude.last30DaysCostUSD;

        // Merge all daily entries by date
        QHash<QString, CostUsageDailyEntry> dayMap;
        auto mergeDailyInto = [&](const QVector<CostUsageDailyEntry>& daily) {
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
        mergeDailyInto(claude.daily);
        mergeDailyInto(codex.daily);
        mergeDailyInto(piResult.codex.daily);
        mergeDailyInto(piResult.claude.daily);

        for (auto it = dayMap.constBegin(); it != dayMap.constEnd(); ++it)
            combined.daily.append(it.value());
        std::sort(combined.daily.begin(), combined.daily.end(),
                  [](const CostUsageDailyEntry& a, const CostUsageDailyEntry& b) { return a.date < b.date; });

        QMetaObject::invokeMethod(this, [this, combined]() {
            m_costUsage = combined;
            m_costUsageRefreshing = false;
            emit costUsageRefreshingChanged();
            emit costUsageChanged();
        }, Qt::QueuedConnection);
    });
}

QVariantMap UsageStore::costUsageData() const {
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
    return m;
}

void UsageStore::refresh() {
    refreshCostUsage();

    if (m_isRefreshing) return;
    m_isRefreshing = true;
    emit refreshingChanged();

    auto ids = ProviderRegistry::instance().enabledProviderIDs();
    if (ids.isEmpty()) {
        m_isRefreshing = false;
        emit refreshingChanged();
        return;
    }

    m_pendingRefreshes = ids.size();
    for (const auto& id : ids) {
        refreshProvider(id);
    }
}

void UsageStore::refreshAll() {
    refresh();
}

void UsageStore::clearCache() {
    const auto ids = ProviderRegistry::instance().providerIDs();
    m_snapshots.clear();
    m_errors.clear();
    m_connectionTests.clear();
    m_snapshotRevision++;
    emit snapshotRevisionChanged();
    for (const auto& id : ids) {
        emit snapshotChanged(id);
        emit providerConnectionTestChanged(id);
    }
}

void UsageStore::refreshProvider(const QString& providerId) {
    auto* provider = ProviderRegistry::instance().provider(providerId);
    if (!provider) {
        m_pendingRefreshes--;
        if (m_pendingRefreshes <= 0) {
            m_isRefreshing = false;
            emit refreshingChanged();
        }
        return;
    }

    ProviderFetchContext ctx = buildFetchContextForProvider(providerId);
    if (!isSourceModeAllowed(providerId, ctx.sourceMode)) {
        m_errors[providerId] = QString("unsupported source mode: %1").arg(sourceModeToString(ctx.sourceMode));
        emit errorOccurred(providerId, providerError(providerId));
        m_pendingRefreshes--;
        if (m_pendingRefreshes <= 0) {
            m_isRefreshing = false;
            emit refreshingChanged();
        }
        return;
    }

    auto strategies = m_pipeline->resolveStrategies(provider, ctx);
    if (strategies.isEmpty()) {
        m_errors[providerId] = "no available fetch strategy";
        emit errorOccurred(providerId, providerError(providerId));
        m_pendingRefreshes--;
        if (m_pendingRefreshes <= 0) {
            m_isRefreshing = false;
            emit refreshingChanged();
        }
        return;
    }

    QtConcurrent::run([this, providerId, strategies, ctx]() {
        ProviderFetchResult result = m_pipeline->execute(strategies, ctx);

        QMetaObject::invokeMethod(this, [this, providerId, result]() {
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
                }
            } else {
                m_errors[providerId] = result.errorMessage;
                emit errorOccurred(providerId, providerError(providerId));
            }
            m_snapshotRevision++;
            emit snapshotRevisionChanged();
            emit snapshotChanged(providerId);

            m_pendingRefreshes--;
            if (m_pendingRefreshes <= 0) {
                m_isRefreshing = false;
                emit refreshingChanged();
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
    QVariantMap status;
    status["configured"] = false;
    status["source"] = "none";

    auto descriptor = settingDescriptor(providerId, key);
    if (!descriptor.has_value() || !descriptor->sensitive) return status;

    status["envVar"] = descriptor->envVar;
    status["credentialTarget"] = descriptor->credentialTarget;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!descriptor->envVar.isEmpty() && env.contains(descriptor->envVar)) {
        status["configured"] = true;
        status["source"] = "env";
        status["readOnly"] = true;
        return status;
    }

    if (!descriptor->credentialTarget.isEmpty()
        && ProviderCredentialStore::exists(descriptor->credentialTarget)) {
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
        bool ok = ProviderCredentialStore::write(descriptor->credentialTarget, {}, trimmed.toUtf8());
        emit providerConnectionTestChanged(providerId);
        return ok;
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
        bool ok = ProviderCredentialStore::remove(descriptor->credentialTarget);
        emit providerConnectionTestChanged(providerId);
        return ok;
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

    auto strategies = m_pipeline->resolveStrategies(provider, ctx);
    if (strategies.isEmpty()) {
        failNow("No available fetch strategy");
        return;
    }

    setProviderConnectionTest(providerId, {
        {"state", "testing"},
        {"message", "Testing connection..."},
        {"details", ""},
        {"startedAt", startedAt},
        {"finishedAt", 0},
        {"durationMs", 0}
    });
    QtConcurrent::run([this, providerId, strategies, ctx, startedAt]() {
        ProviderPipeline pipeline;
        ProviderFetchResult result = pipeline.execute(strategies, ctx);
        QMetaObject::invokeMethod(this, [this, providerId, result, startedAt]() {
            const qint64 finishedAt = QDateTime::currentDateTime().toMSecsSinceEpoch();
            if (result.success) {
                m_snapshots[providerId] = result.usage;
                m_errors.remove(providerId);
                m_snapshotRevision++;
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

    QtConcurrent::run([this, providerId, cancelFlag]() {
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
    
    const auto& snap = it.value();
    if (snap.primary.has_value()) {
        QVariantMap primary;
        primary["percent"] = snap.primary->usedPercent;
        primary["remaining"] = 100.0 - snap.primary->usedPercent;
        if (snap.primary->resetsAt.has_value() && snap.primary->resetsAt.value().isValid()) {
            primary["resetsAt"] = snap.primary->resetsAt.value().toString(Qt::ISODate);
        }
        result["primary"] = primary;
    }
    if (snap.secondary.has_value()) {
        QVariantMap secondary;
        secondary["percent"] = snap.secondary->usedPercent;
        secondary["remaining"] = 100.0 - snap.secondary->usedPercent;
        if (snap.secondary->resetsAt.has_value() && snap.secondary->resetsAt.value().isValid()) {
            secondary["resetsAt"] = snap.secondary->resetsAt.value().toString(Qt::ISODate);
        }
        result["secondary"] = secondary;
    }
    if (snap.tertiary.has_value()) {
        QVariantMap tertiary;
        tertiary["percent"] = snap.tertiary->usedPercent;
        tertiary["remaining"] = 100.0 - snap.tertiary->usedPercent;
        if (snap.tertiary->resetsAt.has_value() && snap.tertiary->resetsAt.value().isValid()) {
            tertiary["resetsAt"] = snap.tertiary->resetsAt.value().toString(Qt::ISODate);
        }
        result["tertiary"] = tertiary;
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

        QtConcurrent::run([this, id, endpoint]() {
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
