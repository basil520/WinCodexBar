#include "ProviderPipeline.h"
#include "IProvider.h"

#include <QtAlgorithms>

namespace {

bool sourceModeAcceptsKind(ProviderSourceMode mode, int kind)
{
    switch (mode) {
    case ProviderSourceMode::Auto:
        return true;
    case ProviderSourceMode::Web:
        return kind == ProviderFetchKind::Web || kind == ProviderFetchKind::WebDashboard;
    case ProviderSourceMode::CLI:
        return kind == ProviderFetchKind::CLI;
    case ProviderSourceMode::OAuth:
        return kind == ProviderFetchKind::OAuth;
    case ProviderSourceMode::API:
        return kind == ProviderFetchKind::APIToken;
    }
    return true;
}

} // namespace

ProviderPipeline::ProviderPipeline(QObject* parent)
    : QObject(parent) {}

QVector<IFetchStrategy*> ProviderPipeline::resolveStrategies(
    IProvider* provider,
    const ProviderFetchContext& ctx)
{
    if (!provider) return {};
    QVector<IFetchStrategy*> strategies = provider->createStrategies(ctx);
    if (ctx.sourceMode == ProviderSourceMode::Auto) {
        return strategies;
    }

    QVector<IFetchStrategy*> filtered;
    for (auto* strategy : strategies) {
        if (!strategy) continue;
        if (sourceModeAcceptsKind(ctx.sourceMode, strategy->kind())) {
            filtered.append(strategy);
        } else {
            delete strategy;
        }
    }
    return filtered;
}

ProviderFetchResult ProviderPipeline::execute(
    const QVector<IFetchStrategy*>& strategies,
    const ProviderFetchContext& ctx)
{
    ProviderFetchResult lastResult;
    lastResult.success = false;

    for (auto* strategy : strategies) {
        if (!strategy) continue;
        if (!strategy->isAvailable(ctx)) continue;

        emit strategyStarted(strategy->id());

        ProviderFetchResult result = strategy->fetchSync(ctx);
        if (result.success) {
            emit pipelineComplete(result);
            return result;
        }
        lastResult = result;
        if (!strategy->shouldFallback(result, ctx)) {
            break;
        }
    }

    emit pipelineComplete(lastResult);
    return lastResult;
}

ProviderFetchResult ProviderPipeline::executeProvider(
    IProvider* provider,
    const ProviderFetchContext& ctx)
{
    QVector<IFetchStrategy*> strategies = resolveStrategies(provider, ctx);
    if (strategies.isEmpty()) {
        ProviderFetchResult result;
        result.success = false;
        result.errorMessage = "No available fetch strategy";
        emit pipelineComplete(result);
        return result;
    }

    struct StrategyCleanup {
        QVector<IFetchStrategy*>& strategies;
        ~StrategyCleanup() { qDeleteAll(strategies); }
    } cleanup{strategies};

    ProviderFetchResult result = execute(strategies, ctx);
    if (!result.success && result.errorMessage.trimmed().isEmpty()) {
        result.errorMessage = "No available fetch strategy";
    }
    return result;
}
