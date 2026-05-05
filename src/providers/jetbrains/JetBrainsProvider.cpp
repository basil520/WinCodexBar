#include "JetBrainsProvider.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

JetBrainsProvider::JetBrainsProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> JetBrainsProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new JetBrainsLocalStrategy(this) };
}

JetBrainsLocalStrategy::JetBrainsLocalStrategy(QObject* parent) : IFetchStrategy(parent) {}

QString JetBrainsLocalStrategy::findQuotaXml() {
    QString appData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (appData.isEmpty()) appData = QDir::homePath() + "/.local/share";

    // On Windows, GenericDataLocation is usually %LOCALAPPDATA%
    // But JetBrains uses %APPDATA%
    QString jetbrainsDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (jetbrainsDir.isEmpty()) jetbrainsDir = QDir::homePath() + "/.config";

    // Windows: %APPDATA%/JetBrains
    #ifdef Q_OS_WIN
    QString appDataEnv = qEnvironmentVariable("APPDATA");
    if (!appDataEnv.isEmpty()) jetbrainsDir = appDataEnv;
    #endif

    jetbrainsDir += "/JetBrains";

    QStringList idePatterns = {
        "IntelliJIdea*", "PyCharm*", "WebStorm*", "GoLand*", "CLion*",
        "DataGrip*", "RubyMine*", "Rider*", "PhpStorm*", "AndroidStudio*",
        "Fleet*", "RustRover*", "Aqua*", "DataSpell*"
    };

    QDir baseDir(jetbrainsDir);
    if (!baseDir.exists()) return {};

    for (const auto& pattern : idePatterns) {
        QStringList entries = baseDir.entryList({pattern}, QDir::Dirs, QDir::Name);
        for (const auto& entry : entries) {
            QString xmlPath = jetbrainsDir + "/" + entry + "/options/AIAssistantQuotaManager2.xml";
            if (QFile::exists(xmlPath)) return xmlPath;
        }
    }
    return {};
}

bool JetBrainsLocalStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !findQuotaXml().isEmpty();
}

bool JetBrainsLocalStrategy::shouldFallback(const ProviderFetchResult& result,
                                             const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

ProviderFetchResult JetBrainsLocalStrategy::parseXml(const QString& xmlContent) {
    ProviderFetchResult result;
    result.strategyID = "jetbrains.local";
    result.strategyKind = ProviderFetchKind::LocalProbe;
    result.sourceLabel = "local";

    // Extract the value attribute from the AIAssistant option
    QRegularExpression re("name=\"AIAssistant\\.Default\\.AIQuotaManager2\"[^>]*value=\"([^\"]+)\"");
    QRegularExpressionMatch match = re.match(xmlContent);

    if (!match.hasMatch()) {
        // Try alternative format: option with nested content
        QRegularExpression re2("<option\\s+name=\"AIAssistant\\.Default\\.AIQuotaManager2\"[^>]*>");
        match = re2.match(xmlContent);
        if (match.hasMatch()) {
            // Look for JSON-like content in the next characters
            int pos = match.capturedEnd();
            QString remaining = xmlContent.mid(pos, qMin(2000, xmlContent.length() - pos));
            QRegularExpression jsonRe("\\{[^{}]*\"quotaInfo\"[^{}]*\\}");
            match = jsonRe.match(remaining);
            if (!match.hasMatch()) {
                result.success = false;
                result.errorMessage = "No AIAssistant quota data found in XML";
                return result;
            }
        } else {
            result.success = false;
            result.errorMessage = "No AIAssistant quota data found in XML";
            return result;
        }
    }

    QString jsonStr = match.captured(1);
    if (jsonStr.isEmpty()) jsonStr = match.captured(0);

    // Unescape XML entities
    jsonStr.replace("&quot;", "\"");
    jsonStr.replace("&amp;", "&");
    jsonStr.replace("&lt;", "<");
    jsonStr.replace("&gt;", ">");
    jsonStr.replace("&apos;", "'");

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
    QJsonObject root = jsonDoc.object();
    QJsonObject quotaInfo = root["quotaInfo"].toObject();

    if (quotaInfo.isEmpty()) {
        result.success = false;
        result.errorMessage = "No quotaInfo in JetBrains data";
        return result;
    }

    double current = quotaInfo["current"].toString().toDouble();
    double maximum = quotaInfo["maximum"].toString().toDouble();

    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::jetbrains;
    snap.identity = identity;

    RateWindow rw;
    rw.usedPercent = (maximum > 0) ? qBound(0.0, (current / maximum) * 100.0, 100.0) : 0;
    rw.resetDescription = QCoreApplication::translate("ProviderLabels", "%1 / %2").arg(current, 0, 'f', 0).arg(maximum, 0, 'f', 0);

    QJsonObject nextRefill = root["nextRefill"].toObject();
    QString nextTime = nextRefill["next"].toString();
    if (!nextTime.isEmpty()) {
        rw.resetsAt = QDateTime::fromString(nextTime, Qt::ISODate);
    }

    snap.primary = rw;
    result.usage = snap;
    result.success = true;
    return result;
}

ProviderFetchResult JetBrainsLocalStrategy::fetchSync(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)

    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "local";

    QString xmlPath = findQuotaXml();
    if (xmlPath.isEmpty()) {
        result.success = false;
        result.errorMessage = "No JetBrains IDE installation found.";
        return result;
    }

    QFile file(xmlPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.success = false;
        result.errorMessage = "Could not read JetBrains quota file: " + xmlPath;
        return result;
    }

    QString content = QString::fromUtf8(file.readAll());
    return parseXml(content);
}
