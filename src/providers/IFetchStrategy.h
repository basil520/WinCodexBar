#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QFuture>
#include <QVariant>
#include <QtConcurrent>

#include "ProviderFetchContext.h"
#include "ProviderFetchResult.h"

struct ProviderSettingOption {
    QString value;
    QString label;
};

struct ProviderSettingsDescriptor {
    QString key;
    QString label;
    QString type;
    QVariant defaultValue;
    QVector<ProviderSettingOption> options;
    QString credentialTarget;
    QString envVar;
    QString placeholder;
    QString helpText;
    bool multiline = false;
    bool sensitive = false;
};

class IFetchStrategy : public QObject {
    Q_OBJECT
public:
    explicit IFetchStrategy(QObject* parent = nullptr) : QObject(parent) {}
    ~IFetchStrategy() override = default;

    virtual QString id() const = 0;
    virtual int kind() const = 0;
    virtual bool isAvailable(const ProviderFetchContext& ctx) const = 0;
    virtual QFuture<ProviderFetchResult> fetch(const ProviderFetchContext& ctx) {
        return QtConcurrent::run([this, ctx]() {
            return fetchSync(ctx);
        });
    }

    virtual ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) {
        Q_UNUSED(ctx)
        ProviderFetchResult r;
        r.success = false;
        r.errorMessage = "fetchSync not implemented";
        return r;
    }

    virtual bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const = 0;

signals:
    void progressChanged(const QString& message, int percent);
};
