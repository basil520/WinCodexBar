#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"

#include <QObject>
#include <QString>

class VertexAIProvider : public IProvider {
    Q_OBJECT
public:
    explicit VertexAIProvider(QObject* parent = nullptr);

    QString id() const override { return "vertexai"; }
    QString displayName() const override { return "Vertex AI"; }
    QString sessionLabel() const override { return "Usage"; }
    QString weeklyLabel() const override { return "Quota"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QString brandColor() const override { return "#4285F4"; }
    QString statusLinkURL() const override { return "https://status.cloud.google.com"; }
    QVector<QString> supportedSourceModes() const override { return {"oauth"}; }
};

class VertexAIOAuthStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit VertexAIOAuthStrategy(QObject* parent = nullptr);

    QString id() const override { return "vertexai.oauth"; }
    int kind() const override { return ProviderFetchKind::OAuth; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

private:
    static QString loadADC();
    static QString resolveProjectId();
};
