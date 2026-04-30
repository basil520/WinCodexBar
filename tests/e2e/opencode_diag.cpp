#include <QCoreApplication>
#include <QDebug>
#include "../src/providers/opencode/OpenCodeProvider.h"
#include "../src/providers/ProviderFetchContext.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    OpenCodeProvider provider;
    ProviderFetchContext ctx;
    ctx.providerId = "opencode";
    ctx.networkTimeoutMs = 15000;

    QString cookie = qgetenv("CODEXBAR_E2E_OPENCODE_COOKIE");
    if (cookie.isEmpty()) cookie = qgetenv("OPENCODE_COOKIE");
    if (cookie.isEmpty()) cookie = qgetenv("OPENCODE_AUTH");

    qDebug() << "Cookie:" << cookie.left(50) << "...";
    ctx.manualCookieHeader = cookie;

    auto* strategy = provider.createStrategies(ctx)[0];
    auto result = strategy->fetchSync(ctx);

    qDebug() << "Success:" << result.success;
    qDebug() << "Error:" << result.errorMessage;
    qDebug() << "Primary:" << result.usage.primary.has_value();
    qDebug() << "Secondary:" << result.usage.secondary.has_value();

    return result.success ? 0 : 1;
}
