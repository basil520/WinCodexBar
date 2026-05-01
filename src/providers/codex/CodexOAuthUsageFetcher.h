#pragma once

#include "../../models/CodexUsageResponse.h"

#include <QString>
#include <QJsonObject>
#include <QHash>
#include <optional>

class CodexOAuthUsageFetcher {
public:
    enum class FetchError {
        Unauthorized,
        InvalidResponse,
        ServerError,
        NetworkError
    };

    struct FetchResult {
        bool success;
        std::optional<CodexUsageResponse> response;
        FetchError error;
        QString errorMessage;
        int statusCode;
    };

    static FetchResult fetchUsage(
        const QString& accessToken,
        const QString& accountId,
        const QHash<QString, QString>& env);

    static QUrl resolveUsageURL(const QHash<QString, QString>& env);

private:
    static const QString DefaultChatGPTBaseURL;
    static const QString ChatGPTUsagePath;
    static const QString CodexUsagePath;

    static QString resolveChatGPTBaseURL(const QHash<QString, QString>& env);
    static QString normalizeChatGPTBaseURL(const QString& value);
    static QString parseChatGPTBaseURL(const QString& contents);
    static QString loadConfigContents(const QHash<QString, QString>& env);
};
