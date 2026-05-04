#include "CLIUsageCommand.h"
#include "../providers/ProviderRegistry.h"
#include "../providers/ProviderPipeline.h"
#include "../providers/IProvider.h"
#include "../account/TokenAccountStore.h"
#include "../runtime/ProviderRuntimeManager.h"
#include "../runtime/IProviderRuntime.h"
#include "../app/SettingsStore.h"
#include <QJsonArray>
#include <QDateTime>

int CLIUsageCommand::execute(const CLIUsageOptions& opts, CLIRenderer& out)
{
    QStringList providerIds;
    if (opts.providerId == "all") {
        for (auto* p : ProviderRegistry::instance().allProviders()) {
            if (SettingsStore().isProviderEnabled(p->id())) {
                providerIds.append(p->id());
            }
        }
    } else {
        providerIds.append(opts.providerId);
    }

    QJsonArray results;
    QJsonArray errors;

    for (const QString& id : providerIds) {
        IProvider* provider = ProviderRegistry::instance().provider(id);
        if (!provider) continue;

        ProviderFetchContext ctx;
        ctx.providerId = id;
        ctx.sourceMode = sourceModeFromString(opts.sourceMode);
        ctx.networkTimeoutMs = opts.webTimeoutMs;
        ctx.isAppRuntime = false;
        ctx.allowInteractiveAuth = false;

        // Resolve account and credentials
        TokenAccountStore* accountStore = TokenAccountStore::instance();
        QString accountId = accountStore->defaultAccountId(id);
        if (!accountId.isEmpty()) {
            auto accOpt = accountStore->account(accountId);
            if (accOpt.has_value()) {
                ctx.accountID = accOpt->accountId;
                ctx.accountCredentials = accOpt->credentials;
                if (accOpt->sourceMode != ProviderSourceMode::Auto) {
                    ctx.sourceMode = accOpt->sourceMode;
                }
            }
        }

        ProviderFetchResult result;
        // Prefer runtime if available
        if (ProviderRuntimeManager* rtMgr = ProviderRuntimeManager::instance()) {
            if (IProviderRuntime* runtime = rtMgr->runtimeFor(id)) {
                if (runtime->state() == RuntimeState::Running || runtime->state() == RuntimeState::Stopped) {
                    result = runtime->fetch(ctx);
                }
            }
        }
        if (!result.success && result.errorMessage.isEmpty()) {
            // Fallback to direct pipeline
            ProviderPipeline pipeline;
            result = pipeline.executeProvider(provider, ctx);
        }

        if (result.success) {
            QJsonObject obj;
            obj["id"] = id;
            obj["displayName"] = provider->displayName();
            obj["success"] = true;

            if (result.usage.primary.has_value()) {
                QJsonObject primary;
                primary["usedPercent"] = result.usage.primary->usedPercent;
                primary["remainingPercent"] = result.usage.primary->remainingPercent();
                if (result.usage.primary->resetsAt.has_value()) {
                    primary["resetsAt"] = result.usage.primary->resetsAt.value().toString(Qt::ISODate);
                }
                obj["primary"] = primary;
            }
            if (result.usage.secondary.has_value()) {
                QJsonObject secondary;
                secondary["usedPercent"] = result.usage.secondary->usedPercent;
                secondary["remainingPercent"] = result.usage.secondary->remainingPercent();
                if (result.usage.secondary->resetsAt.has_value()) {
                    secondary["resetsAt"] = result.usage.secondary->resetsAt.value().toString(Qt::ISODate);
                }
                obj["secondary"] = secondary;
            }
            if (result.credits.has_value()) {
                QJsonObject credits;
                credits["remainingUSD"] = result.credits->remaining;
                obj["credits"] = credits;
            }
            results.append(obj);

            if (!out.isJsonMode()) {
                out.heading(provider->displayName());
                if (result.usage.primary.has_value()) {
                    out.line(QString("  Primary: %1% used (%2 remaining)")
                             .arg(result.usage.primary->usedPercent, 0, 'f', 1)
                             .arg(result.usage.primary->remainingPercent(), 0, 'f', 1));
                }
                if (result.usage.secondary.has_value()) {
                    out.line(QString("  Secondary: %1% used (%2 remaining)")
                             .arg(result.usage.secondary->usedPercent, 0, 'f', 1)
                             .arg(result.usage.secondary->remainingPercent(), 0, 'f', 1));
                }
                if (result.credits.has_value()) {
                    out.line(QString("  Credits: $%1 remaining").arg(result.credits->remaining, 0, 'f', 2));
                }
                out.blank();
            }
        } else {
            QJsonObject errObj;
            errObj["provider"] = id;
            errObj["error"] = result.errorMessage;
            errors.append(errObj);

            if (!out.isJsonMode()) {
                out.error(QString("%1: %2").arg(id, result.errorMessage));
            }
        }
    }

    if (out.isJsonMode()) {
        QJsonObject root;
        root["providers"] = results;
        root["errors"] = errors;
        root["checkedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        out.setRootObject(root);
    } else {
        out.line(QString("%1 providers checked, %2 errors.").arg(results.size()).arg(errors.size()));
    }
    out.flush();

    return errors.isEmpty() ? 0 : 1;
}
