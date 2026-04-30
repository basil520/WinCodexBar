#pragma once

#include <QString>
#include <QHash>
#include <QJsonValue>

struct ProviderFetchContext;

namespace OpenCodeUtils {

QString extractAuthCookie(const QString& raw);
QString resolveCookieHeader(const ProviderFetchContext& ctx);
QString fetchWorkspaceID(const QString& cookieHeader, int timeoutMs);
bool looksSignedOut(const QString& text);
QString serverRequestURL(const QString& serverID, const QStringList& args, const QString& method);
QString normalizeWorkspaceID(const QString& raw);

} // namespace OpenCodeUtils
