#pragma once

#include <QString>
#include <optional>

class BinaryLocator {
public:
    static QString resolve(const QString& name);
    static std::optional<QString> version(const QString& binaryPath);
    static bool isAvailable(const QString& name);
};
