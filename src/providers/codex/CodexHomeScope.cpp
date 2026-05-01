#include "CodexHomeScope.h"

QString CodexHomeScope::ambientHomeURL(const QHash<QString, QString>& env)
{
    return resolveHome(env);
}

QString CodexHomeScope::ambientHomeURL(const QHash<QString, QString>& env, const QDir& fileManager)
{
    Q_UNUSED(fileManager);
    return resolveHome(env);
}

QHash<QString, QString> CodexHomeScope::scopedEnvironment(
    const QHash<QString, QString>& base,
    const QString& codexHome)
{
    QHash<QString, QString> env = base;
    env["CODEX_HOME"] = codexHome;
    return env;
}

QString CodexHomeScope::resolveHome(const QHash<QString, QString>& env)
{
    QString home = env.value("USERPROFILE", env.value("HOME", QDir::homePath()));
    QString codexHome = env.value("CODEX_HOME").trimmed();
    return codexHome.isEmpty() ? home + "/.codex" : codexHome;
}
