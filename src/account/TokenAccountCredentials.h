#pragma once

#include "SecureString.h"
#include <QUrl>
#include <QDateTime>
#include <QString>
#include <optional>

struct APICredentials {
    SecureString apiKey;
    QUrl baseUrl;              // Optional override (e.g., OpenRouter custom endpoint)
    QString organizationId;    // Optional (OpenAI org)

    bool isValid() const { return !apiKey.isEmpty(); }

    QVariantMap toVariantMap() const {
        QVariantMap m;
        if (!apiKey.isEmpty()) m["apiKey"] = QString("<encrypted>");
        if (!baseUrl.isEmpty()) m["baseUrl"] = baseUrl.toString();
        if (!organizationId.isEmpty()) m["organizationId"] = organizationId;
        return m;
    }
};

struct WebCredentials {
    QString cookieDomain;      // e.g. "github.com"
    QString cookieName;        // e.g. "user_session"
    SecureString cookieValue;
    QDateTime expiry;

    bool isValid() const { return !cookieValue.isEmpty() && !cookieDomain.isEmpty(); }

    QVariantMap toVariantMap() const {
        QVariantMap m;
        m["cookieDomain"] = cookieDomain;
        m["cookieName"] = cookieName;
        if (!cookieValue.isEmpty()) m["cookieValue"] = QString("<encrypted>");
        if (expiry.isValid()) m["expiry"] = expiry.toString(Qt::ISODate);
        return m;
    }
};

struct OAuthCredentials {
    SecureString accessToken;
    SecureString refreshToken;
    QDateTime expiresAt;
    QString tokenType;         // "Bearer"

    bool isValid() const { return !accessToken.isEmpty(); }

    QVariantMap toVariantMap() const {
        QVariantMap m;
        if (!accessToken.isEmpty()) m["accessToken"] = QString("<encrypted>");
        if (!refreshToken.isEmpty()) m["refreshToken"] = QString("<encrypted>");
        if (expiresAt.isValid()) m["expiresAt"] = expiresAt.toString(Qt::ISODate);
        if (!tokenType.isEmpty()) m["tokenType"] = tokenType;
        return m;
    }
};

struct TokenAccountCredentials {
    std::optional<APICredentials> api;
    std::optional<WebCredentials> web;
    std::optional<OAuthCredentials> oauth;

    bool isEmpty() const {
        return !api.has_value() && !web.has_value() && !oauth.has_value();
    }

    bool hasCredentialsFor(int kind) const {
        // ProviderFetchKind: CLI=0, Web=1, WebDashboard=2, OAuth=3, APIToken=4
        switch (kind) {
        case 4: // APIToken
            return api.has_value() && api->isValid();
        case 1: // Web
        case 2: // WebDashboard
            return web.has_value() && web->isValid();
        case 3: // OAuth
            return oauth.has_value() && oauth->isValid();
        default:
            return !isEmpty();
        }
    }

    QVariantMap toVariantMap() const {
        QVariantMap m;
        if (api.has_value()) m["api"] = api->toVariantMap();
        if (web.has_value()) m["web"] = web->toVariantMap();
        if (oauth.has_value()) m["oauth"] = oauth->toVariantMap();
        return m;
    }
};
