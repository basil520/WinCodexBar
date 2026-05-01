#pragma once

#include <QString>
#include <QHash>
#include <QDir>

class CodexHomeScope {
public:
    static QString ambientHomeURL(const QHash<QString, QString>& env);
    static QString ambientHomeURL(const QHash<QString, QString>& env, const QDir& fileManager);

    static QHash<QString, QString> scopedEnvironment(
        const QHash<QString, QString>& base,
        const QString& codexHome);

private:
    static QString resolveHome(const QHash<QString, QString>& env);
};
