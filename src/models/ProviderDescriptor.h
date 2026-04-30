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
};

struct ProviderCLIConfig {
    QString name;
    QVector<QString> aliases;
    QString versionArg;
};

struct ProviderDescriptor {
    QString id;
    ProviderMetadata metadata;
    ProviderBranding branding;
    ProviderTokenCostConfig tokenCost;
    ProviderFetchPlan fetchPlan;
    ProviderCLIConfig cli;
};
