#pragma once

#include "../../models/CreditsSnapshot.h"

#include <QString>
#include <QHash>
#include <QDateTime>
#include <optional>

class CodexCreditsFetcher {
public:
    struct FetchResult {
        bool success = false;
        std::optional<CreditsSnapshot> credits;
        QString errorMessage;
    };

    explicit CodexCreditsFetcher(const QHash<QString, QString>& env = {});

    FetchResult fetchCreditsSync(int timeoutMs = 15000);

    void setTimeout(int timeoutMs) { m_timeoutMs = timeoutMs; }
    void setEnvironment(const QHash<QString, QString>& env) { m_env = env; }

private:
    QHash<QString, QString> m_env;
    int m_timeoutMs = 15000;

    FetchResult fetchViaOAuthSync(int timeoutMs);
    FetchResult fetchViaRPCSync(int timeoutMs);
    FetchResult fetchViaPTYSync(int timeoutMs);

    static std::optional<double> parseCreditsFromJson(const QJsonObject& json);
    static double parseBalanceValue(const QJsonValue& value);
};
