#include "CodexUsageDataSource.h"

QString CodexUsageDataSource::displayName(CodexUsageDataSourceEnum source)
{
    switch (source) {
    case CodexUsageDataSourceEnum::Auto: return "Auto";
    case CodexUsageDataSourceEnum::OAuth: return "OAuth API";
    case CodexUsageDataSourceEnum::CLI: return "CLI (RPC/PTY)";
    }
    return "Auto";
}

QString CodexUsageDataSource::sourceLabel(CodexUsageDataSourceEnum source)
{
    switch (source) {
    case CodexUsageDataSourceEnum::Auto: return "auto";
    case CodexUsageDataSourceEnum::OAuth: return "oauth";
    case CodexUsageDataSourceEnum::CLI: return "cli";
    }
    return "auto";
}

CodexUsageDataSourceEnum CodexUsageDataSource::fromString(const QString& value)
{
    QString lower = value.trimmed().toLower();
    if (lower == "oauth") return CodexUsageDataSourceEnum::OAuth;
    if (lower == "cli") return CodexUsageDataSourceEnum::CLI;
    return CodexUsageDataSourceEnum::Auto;
}
