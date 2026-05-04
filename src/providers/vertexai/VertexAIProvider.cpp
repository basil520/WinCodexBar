#include "VertexAIProvider.h"
#include "../../network/NetworkManager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QProcessEnvironment>

VertexAIProvider::VertexAIProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> VertexAIProvider::createStrategies(const ProviderFetchContext& ctx) {
    Q_UNUSED(ctx)
    return { new VertexAIOAuthStrategy(this) };
}

VertexAIOAuthStrategy::VertexAIOAuthStrategy(QObject* parent) : IFetchStrategy(parent) {}

QString VertexAIOAuthStrategy::loadADC() {
    QString path = QDir::homePath() + "/.config/gcloud/application_default_credentials.json";
    #ifdef Q_OS_WIN
    QString appData = qEnvironmentVariable("APPDATA");
    if (!appData.isEmpty()) path = appData + "/gcloud/application_default_credentials.json";
    #endif

    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) return {};
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.object()["access_token"].toString();
}

QString VertexAIOAuthStrategy::resolveProjectId() {
    // Check env vars first
    QStringList envVars = {"GOOGLE_CLOUD_PROJECT", "GCLOUD_PROJECT", "CLOUDSDK_CORE_PROJECT"};
    for (const auto& var : envVars) {
        QString val = qEnvironmentVariable(var.toUtf8().constData());
        if (!val.isEmpty()) return val;
    }

    // Try gcloud config
    QString configPath = QDir::homePath() + "/.config/gcloud/configurations/config_default";
    #ifdef Q_OS_WIN
    QString appData = qEnvironmentVariable("APPDATA");
    if (!appData.isEmpty()) configPath = appData + "/gcloud/configurations/config_default";
    #endif

    QFile file(configPath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};

    QString content = QString::fromUtf8(file.readAll());
    // Simple INI-style parsing for [core] section -> project
    bool inCore = false;
    for (const auto& line : content.split('\n')) {
        QString trimmed = line.trimmed();
        if (trimmed == "[core]") { inCore = true; continue; }
        if (trimmed.startsWith('[')) { inCore = false; continue; }
        if (inCore && trimmed.startsWith("project")) {
            QStringList parts = trimmed.split('=');
            if (parts.size() >= 2) return parts[1].trimmed();
        }
    }
    return {};
}

bool VertexAIOAuthStrategy::isAvailable(const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !loadADC().isEmpty() && !resolveProjectId().isEmpty();
}

bool VertexAIOAuthStrategy::shouldFallback(const ProviderFetchResult& result,
                                            const ProviderFetchContext& ctx) const {
    Q_UNUSED(ctx)
    return !result.success;
}

ProviderFetchResult VertexAIOAuthStrategy::fetchSync(const ProviderFetchContext& ctx) {
    ProviderFetchResult result;
    result.strategyID = id();
    result.strategyKind = kind();
    result.sourceLabel = "oauth";

    QString accessToken = loadADC();
    if (accessToken.isEmpty()) {
        result.success = false;
        result.errorMessage = "Google ADC not found. Run 'gcloud auth application-default login'.";
        return result;
    }

    QString projectId = resolveProjectId();
    if (projectId.isEmpty()) {
        result.success = false;
        result.errorMessage = "GCP project not found. Set GOOGLE_CLOUD_PROJECT or run 'gcloud config set project'.";
        return result;
    }

    // Vertex AI usage is complex (Cloud Monitoring time series).
    // Simplified: just report that Vertex AI is configured but needs Cloud Monitoring API.
    result.success = false;
    result.errorMessage = QString(
        "Vertex AI quota monitoring requires the Cloud Monitoring API (monitoring.read scope). "
        "To enable:\n"
        "1. Enable the Monitoring API: gcloud services enable monitoring.googleapis.com\n"
        "2. Grant read access: gcloud projects add-iam-policy-binding %1 "
        "--member=user:$(gcloud auth list --filter=status:ACTIVE --format='value(account)') "
        "--role=roles/monitoring.viewer\n"
        "3. For AI Studio usage, use the Gemini provider instead."
    ).arg(projectId);
    return result;
}
