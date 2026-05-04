#pragma once

#include <QString>
#include <optional>
#include <QJsonObject>

/**
 * @brief Resolves and validates Kimi authentication tokens from various sources.
 *
 * Mirrors the upstream KimiTokenResolver Swift class.
 */
class KimiTokenResolver {
public:
    /**
     * @brief Extract a JWT token from raw user input.
     *
     * Supports:
     * - Direct JWT (eyJ... with 3 parts)
     * - Cookie header: kimi-auth=...
     * - Key-value: kimi-auth: ...
     * - curl -H 'Cookie: kimi-auth=...'
     * - curl -H 'Authorization: Bearer ...'
     */
    static std::optional<QString> extractKimiAuthToken(const QString& raw);

    /**
     * @brief Decode the payload section of a JWT token into a JSON object.
     */
    static QJsonObject decodeJWTPayload(const QString& jwt);

    /**
     * @brief Check if a string looks like a valid JWT token.
     */
    static bool isValidJWT(const QString& token);
};
