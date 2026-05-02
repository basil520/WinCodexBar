#pragma once

#include "../models/UsageSnapshot.h"
#include "../models/CreditsSnapshot.h"
#include "ProviderFetchKind.h"
#include "ProviderFetchAttempt.h"

#include <QString>
#include <QVector>
#include <QJsonObject>
#include <optional>

struct ProviderFetchResult {
    UsageSnapshot usage;
    std::optional<CreditsSnapshot> credits;
    QString sourceLabel;
    QString strategyID;
    int strategyKind = ProviderFetchKind::CLI;
    bool success = false;
    QString errorMessage;
    QVector<ProviderFetchAttempt> attempts;
    std::optional<QJsonObject> dashboard;
};
