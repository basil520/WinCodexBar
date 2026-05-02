#include "CodexUIErrorMapper.h"

QString CodexUIErrorMapper::userFacingMessage(const QString& raw) {
    if (raw.isEmpty()) return {};

    QString trimmed = raw.trimmed();
    if (trimmed.isEmpty()) return {};

    QString lower = trimmed.toLower();

    if (isAlreadyUserFacing(lower)) {
        return trimmed;
    }

    QString cached = cachedMessage(trimmed, lower);
    if (!cached.isEmpty()) {
        return cached;
    }

    if (looksExpired(lower)) {
        return QStringLiteral("Codex session expired. Sign in again.");
    }

    if (lower.contains("frame load interrupted")) {
        return QStringLiteral("OpenAI web refresh was interrupted. Refresh OpenAI cookies and try again.");
    }

    if (looksInternalTransport(lower)) {
        return QStringLiteral("Codex usage is temporarily unavailable. Try refreshing.");
    }

    return trimmed;
}

bool CodexUIErrorMapper::isAlreadyUserFacing(const QString& lower) {
    return lower.contains("openai cookies are for")
        || lower.contains("sign in to chatgpt.com")
        || lower.contains("requires a signed-in chatgpt.com session")
        || lower.contains("managed codex account data is unavailable")
        || lower.contains("selected managed codex account is unavailable")
        || lower.contains("codex credits are still loading")
        || lower.contains("codex account changed; importing browser cookies")
        || lower.contains("codex session expired. sign in again.")
        || lower.contains("codex usage is temporarily unavailable. try refreshing.");
}

bool CodexUIErrorMapper::looksExpired(const QString& lower) {
    return lower.contains("token_expired")
        || lower.contains("authentication token is expired")
        || lower.contains("oauth token has expired")
        || lower.contains("provided authentication token is expired")
        || lower.contains("please try signing in again")
        || lower.contains("please sign in again")
        || (lower.contains("401") && lower.contains("unauthorized"));
}

bool CodexUIErrorMapper::looksInternalTransport(const QString& lower) {
    return lower.contains("codex connection failed")
        || lower.contains("failed to fetch codex rate limits")
        || lower.contains("/backend-api/")
        || lower.contains("content-type=")
        || lower.contains("body={")
        || lower.contains("body=")
        || lower.contains("get https://")
        || lower.contains("get http://")
        || lower.contains("returned invalid data");
}

QString CodexUIErrorMapper::cachedMessage(const QString& raw, const QString& lower) {
    const QString cachedMarker = QStringLiteral(" Cached values from ");
    int suffixIndex = raw.indexOf(cachedMarker, Qt::CaseInsensitive);
    if (suffixIndex < 0) return {};

    QString suffix = raw.mid(suffixIndex).trimmed();

    if (lower.startsWith("last codex credits refresh failed:")) {
        QString basePart = raw.left(suffixIndex);
        QString base = userFacingMessage(basePart);
        if (!base.isEmpty()) {
            return base + QStringLiteral(" ") + suffix;
        }
    }

    if (lower.startsWith("last openai dashboard refresh failed:")) {
        QString basePart = raw.left(suffixIndex);
        QString base = userFacingMessage(basePart);
        if (!base.isEmpty()) {
            return base + QStringLiteral(" ") + suffix;
        }
    }

    return {};
}
