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

    // On Windows, also check for .cmd, .bat, and .ps1 wrappers (e.g., npm global packages)
    // Prefer .cmd/.bat over .ps1 because ConPTY/cmd can execute them directly
    QString wrapperPath = QStandardPaths::findExecutable(name + ".cmd");
    if (!wrapperPath.isEmpty()) {
        return wrapperPath;
    }
    wrapperPath = QStandardPaths::findExecutable(name + ".bat");
    if (!wrapperPath.isEmpty()) {
        return wrapperPath;
    }
    wrapperPath = QStandardPaths::findExecutable(name + ".ps1");
    if (!wrapperPath.isEmpty()) {
        return wrapperPath;
    }

    return QString();
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
