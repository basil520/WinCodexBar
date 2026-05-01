#pragma once

#include "../../models/CodexUsageResponse.h"

#include <QString>
#include <QJsonObject>
#include <optional>

class CodexTokenRefresher {
public:
    enum class RefreshError {
        Expired,
        Revoked,
        Reused,
        NetworkError,
        InvalidResponse
    };

    struct RefreshResult {
        bool success;
        std::optional<CodexOAuthCredentials> credentials;
        RefreshError error;
        QString errorMessage;
    };

    static RefreshResult refresh(const CodexOAuthCredentials& credentials);

private:
    static const QString RefreshEndpoint;
    static const QString ClientID;

    static QString extractErrorCode(const QJsonObject& json);
    static RefreshError classifyError(const QString& errorCode);
};
