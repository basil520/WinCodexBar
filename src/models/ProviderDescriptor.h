#pragma once

#include "ProviderIdentitySnapshot.h"

#include <QString>
#include <QVector>
#include <optional>

struct ProviderMetadata {
    QString displayName;
    QString sessionLabel;
    QString weeklyLabel;
    std::optional<QString> opusLabel;
    bool supportsCredits = false;
    QString cliName;
    QString statusPageURL;
    QString statusLinkURL;
    QString statusWorkspaceProductID;
    QString dashboardURL;
    QString subscriptionDashboardURL;
};

struct ProviderBranding {
    QString iconSource;
    QString color;
};

struct ProviderTokenCostConfig {
    bool enabled = false;
};

struct ProviderFetchPlan {
    QVector<QString> allowedSourceModes;
    QString defaultSourceMode = QStringLiteral("auto");
};

struct ProviderCLIConfig {
    QString name;
    QVector<QString> aliases;
    QString versionArg;
};

struct ProviderTokenAccountConfig {
    bool supportsMultipleAccounts = false;
    QVector<QString> requiredCredentialTypes;
};

struct ProviderDescriptor {
    QString id;
    ProviderMetadata metadata;
    ProviderBranding branding;
    ProviderTokenCostConfig tokenCost;
    ProviderFetchPlan fetchPlan;
    ProviderCLIConfig cli;
    ProviderTokenAccountConfig tokenAccounts;
};
