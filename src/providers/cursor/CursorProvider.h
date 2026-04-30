#pragma once

#include "../IProvider.h"
#include "../IFetchStrategy.h"
#include "../ProviderFetchContext.h"
#include "../ProviderFetchResult.h"
#include "../../models/UsageSnapshot.h"
#include "../../models/CursorUsageSummary.h"

#include <QObject>
#include <QString>
#include <QFuture>
#include <QNetworkCookie>

class CursorProvider : public IProvider {
    Q_OBJECT
public:
    explicit CursorProvider(QObject* parent = nullptr);

    QString id() const override { return "cursor"; }
    QString displayName() const override { return "Cursor"; }
    QString sessionLabel() const override { return "Total"; }
    QString weeklyLabel() const override { return "Auto"; }
    QString opusLabel() const override { return "API"; }
    bool supportsCredits() const override { return true; }
    QString cliName() const override { return "cursor"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext& ctx) override;

    QString dashboardURL() const override { return "https://cursor.com/dashboard?tab=usage"; }
    QString statusPageURL() const override { return "https://status.cursor.com"; }
    QString brandColor() const override { return "#00BFA5"; }

    QVector<ProviderSettingsDescriptor> settingsDescriptors() const override {
        return {
            {"sourceMode", "Data source", "picker", QVariant(QStringLiteral("auto")),
             { {"auto", "Auto"}, {"web", "Web"} }},
            {"cookieSource", "Cookie source", "picker", QVariant(QStringLiteral("auto")),
             { {"auto", "Auto"}, {"browser", "Browser cookies"}, {"manual", "Manual cookie"} }},
            {"manualCookieHeader", "Manual cookie header", "secret", QVariant(),
             {}, "com.codexbar.cookie.cursor", {}, "WorkosCursorSessionToken=...", "Stored in Windows Credential Manager", true, true}
        };
    }
    QVector<QString> supportedSourceModes() const override { return {"auto", "web"}; }
};

class CursorWebStrategy : public IFetchStrategy {
    Q_OBJECT
public:
    explicit CursorWebStrategy(QObject* parent = nullptr);

    QString id() const override { return "cursor.web"; }
    int kind() const override { return ProviderFetchKind::Web; }
    bool isAvailable(const ProviderFetchContext& ctx) const override;
    ProviderFetchResult fetchSync(const ProviderFetchContext& ctx) override;
    bool shouldFallback(const ProviderFetchResult& result, const ProviderFetchContext& ctx) const override;

private:
    static QVector<QNetworkCookie> importSessionCookies();
    static std::optional<QString> findSessionCookie(const QVector<QNetworkCookie>& cookies);
    static QString buildCookieHeader(const QVector<QNetworkCookie>& cookies);

    static const QStringList s_knownCookieNames;
};
