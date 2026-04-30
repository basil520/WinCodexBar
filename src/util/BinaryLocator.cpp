#include "BinaryLocator.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

QString BinaryLocator::resolve(const QString& name) {
    QString envKey = "CODEXBAR_" + name.toUpper() + "_PATH";
    QString envPath = qEnvironmentVariable(envKey.toUtf8());
    if (!envPath.isEmpty() && QFileInfo::exists(envPath)) {
        return envPath;
    }

    QStringList searchPaths = {
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.local/bin/" + name + ".exe",
        "C:/Program Files/" + name + "/" + name + ".exe",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/." + name + "/bin/" + name + ".exe",
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.homebrew/bin/" + name + ".exe",
    };

    for (const auto& path : searchPaths) {
        if (QFileInfo::exists(path)) {
            return QDir::toNativeSeparators(path);
        }
    }

    QString pathInPath = QStandardPaths::findExecutable(name + ".exe");
    if (!pathInPath.isEmpty()) {
        return pathInPath;
    }

    return name + ".exe";
}

std::optional<QString> BinaryLocator::version(const QString& binaryPath) {
    QProcess proc;
    proc.start(binaryPath, {"--version"});
    if (proc.waitForFinished(3000)) {
        QString output = proc.readAllStandardOutput();
        if (!output.isEmpty()) {
            return output.trimmed();
        }
    }
    return std::nullopt;
}

bool BinaryLocator::isAvailable(const QString& name) {
    QString path = resolve(name);
    return QFileInfo::exists(path);
}
