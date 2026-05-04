#pragma once

#include <QString>

/**
 * @brief Structured error types for Kimi provider operations.
 *
 * Mirrors the upstream KimiAPIError Swift enum.
 */
enum class KimiAPIError {
    MissingToken,
    InvalidToken,
    InvalidRequest,
    NetworkError,
    APIError,
    ParseFailed
};

inline QString kimiErrorMessage(KimiAPIError error, const QString& detail = QString()) {
    switch (error) {
    case KimiAPIError::MissingToken:
        return "Kimi auth token is missing. Please add your JWT token from the Kimi console.";
    case KimiAPIError::InvalidToken:
        return detail.isEmpty()
            ? "Kimi auth token is invalid or expired. Please refresh your token."
            : QString("Kimi auth token is invalid or expired (%1). Please refresh your token.").arg(detail);
    case KimiAPIError::InvalidRequest:
        return detail.isEmpty() ? "Invalid request" : "Invalid request: " + detail;
    case KimiAPIError::NetworkError:
        return detail.isEmpty() ? "Kimi network error" : "Kimi network error: " + detail;
    case KimiAPIError::APIError:
        return detail.isEmpty() ? "Kimi API error" : "Kimi API error: " + detail;
    case KimiAPIError::ParseFailed:
        return detail.isEmpty()
            ? "Failed to parse Kimi usage data"
            : "Failed to parse Kimi usage data: " + detail;
    }
    return "Unknown Kimi error";
}
