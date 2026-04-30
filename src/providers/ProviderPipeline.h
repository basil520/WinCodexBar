#pragma once

#include <QObject>
#include <QVector>

#include "IFetchStrategy.h"
#include "ProviderFetchContext.h"
#include "ProviderFetchResult.h"

class IProvider;

class ProviderPipeline : public QObject {
    Q_OBJECT
public:
    explicit ProviderPipeline(QObject* parent = nullptr);

    QVector<IFetchStrategy*> resolveStrategies(
        IProvider* provider,
        const ProviderFetchContext& ctx
    );

    ProviderFetchResult execute(
        const QVector<IFetchStrategy*>& strategies,
        const ProviderFetchContext& ctx
    );

    static constexpr int STRATEGY_TIMEOUT_MS = 15000;
    static constexpr int PIPELINE_TIMEOUT_MS = 30000;

signals:
    void strategyStarted(const QString& strategyID);
    void strategyFailed(const QString& strategyID, const QString& error);
    void pipelineComplete(const ProviderFetchResult& result);
};
