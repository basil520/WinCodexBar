#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QFuture>

#include "IFetchStrategy.h"
#include "ProviderFetchContext.h"
#include "ProviderFetchResult.h"
#include "../models/ProviderDescriptor.h"

class IProvider : public QObject {
    Q_OBJECT
public:
    explicit IProvider(QObject* parent = nullptr) : QObject(parent) {}
    ~IProvider() override = default;

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual QString sessionLabel() const = 0;
    virtual QString weeklyLabel() const = 0;
    virtual QString opusLabel() const { return {}; }
    virtual bool supportsCredits() const { return false; }
    virtual QString cliName() const { return {}; }

    virtual QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) = 0;

    virtual bool defaultEnabled() const = 0;

    virtual QVector<ProviderSettingsDescriptor> settingsDescriptors() const { return {}; }

    virtual QString statusPageURL() const { return {}; }
    virtual QString statusLinkURL() const { return {}; }
    virtual QString statusWorkspaceProductID() const { return {}; }
    virtual QString dashboardURL() const { return {}; }
    virtual QString subscriptionDashboardURL() const { return {}; }
    virtual QVector<QString> supportedSourceModes() const { return {"auto"}; }
    virtual QString brandColor() const { return "#6b6bff"; }

    // Token Account support
    virtual bool supportsMultipleAccounts() const { return false; }
    virtual QVector<QString> requiredCredentialTypes() const { return {}; }
};
