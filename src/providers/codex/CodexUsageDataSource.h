#pragma once

#include <QString>

enum class CodexUsageDataSourceEnum {
    Auto,
    OAuth,
    CLI
};

class CodexUsageDataSource {
public:
    static QString displayName(CodexUsageDataSourceEnum source);
    static QString sourceLabel(CodexUsageDataSourceEnum source);
    static CodexUsageDataSourceEnum fromString(const QString& value);
};
