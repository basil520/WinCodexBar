#pragma once

#include <QString>

class CodexUIErrorMapper {
public:
    static QString userFacingMessage(const QString& raw);

private:
    static bool isAlreadyUserFacing(const QString& lower);
    static bool looksExpired(const QString& lower);
    static bool looksInternalTransport(const QString& lower);
    static QString cachedMessage(const QString& raw, const QString& lower);
};
