#include "KimiTokenResolver.h"
#include <QRegularExpression>
#include <QJsonDocument>

std::optional<QString> KimiTokenResolver::extractKimiAuthToken(const QString& raw) {
    QString trimmed = raw.trimmed();
    if (trimmed.isEmpty()) return std::nullopt;

    // Direct JWT token: eyJ... with 3 parts
    if (trimmed.startsWith("eyJ") && trimmed.split('.').count() == 3) {
        return trimmed;
    }

    // Cookie header patterns
    QRegularExpression re1(R"re((?i)kimi-auth=([^\s;'"]+))re");
    QRegularExpressionMatch m1 = re1.match(trimmed);
    if (m1.hasMatch() && !m1.captured(1).isEmpty()) return m1.captured(1);

    QRegularExpression re2(R"re((?i)kimi-auth:\s*([^\s;'"]+))re");
    QRegularExpressionMatch m2 = re2.match(trimmed);
    if (m2.hasMatch() && !m2.captured(1).isEmpty()) return m2.captured(1);

    // curl -H 'Cookie: kimi-auth=...' pattern (single-quoted or double-quoted)
    QRegularExpression re3(R"re((?i)-H\s+['"]?Cookie:\s*[^'"]*kimi-auth=([^\s;'"]+)[^'"]*['"]?))re");
    QRegularExpressionMatch m3 = re3.match(trimmed);
    if (m3.hasMatch() && !m3.captured(1).isEmpty()) return m3.captured(1);

    // curl -H 'Authorization: Bearer ...' pattern (JWT token after Bearer)
    QRegularExpression re4(R"re((?i)Authorization:\s*Bearer\s+(\S+?)['"]?\s*$)re");
    QRegularExpressionMatch m4 = re4.match(trimmed);
    if (m4.hasMatch() && !m4.captured(1).isEmpty()) return m4.captured(1);

    return std::nullopt;
}

QJsonObject KimiTokenResolver::decodeJWTPayload(const QString& jwt) {
    QStringList parts = jwt.split('.');
    if (parts.size() != 3) return {};

    QString payload = parts[1];
    payload.replace('-', '+').replace('_', '/');
    while (payload.size() % 4 != 0) payload.append('=');

    QByteArray data = QByteArray::fromBase64(payload.toUtf8());
    if (data.isEmpty()) return {};

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) return {};
    return doc.object();
}

bool KimiTokenResolver::isValidJWT(const QString& token) {
    return !token.isEmpty() && token.startsWith("eyJ") && token.split('.').count() == 3;
}
