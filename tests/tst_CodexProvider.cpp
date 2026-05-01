#include <QtTest/QtTest>

#include "../src/providers/ProviderSettingsSnapshot.h"
#include "../src/providers/codex/CodexProvider.h"

class tst_CodexProvider : public QObject {
    Q_OBJECT
private slots:
    void autoModeUsesOAuthThenRpcThenPtyThenWeb() {
        CodexProvider provider;
        ProviderFetchContext ctx;
        ctx.settings.set("sourceMode", "auto");

        QVector<IFetchStrategy*> strategies = provider.createStrategies(ctx);

        QCOMPARE(strategies.size(), 4);
        QCOMPARE(strategies.at(0)->id(), QString("codex.oauth"));
        QCOMPARE(strategies.at(1)->id(), QString("codex.cli.rpc"));
        QCOMPARE(strategies.at(2)->id(), QString("codex.cli.pty"));
        QCOMPARE(strategies.at(3)->id(), QString("codex.web"));
        qDeleteAll(strategies);
    }

    void cliModeUsesRpcThenPtyOnly() {
        CodexProvider provider;
        ProviderFetchContext ctx;
        ctx.settings.set("sourceMode", "cli");

        QVector<IFetchStrategy*> strategies = provider.createStrategies(ctx);

        QCOMPARE(strategies.size(), 2);
        QCOMPARE(strategies.at(0)->id(), QString("codex.cli.rpc"));
        QCOMPARE(strategies.at(1)->id(), QString("codex.cli.pty"));
        qDeleteAll(strategies);
    }

    void mapsRpcRateLimitsAndAccountToUsageSnapshot() {
        QJsonObject rateLimits{
            {"rateLimits", QJsonObject{
                {"primary", QJsonObject{
                    {"usedPercent", 42},
                    {"windowDurationMins", 300},
                    {"resetsAt", 1893456000}
                }},
                {"secondary", QJsonObject{
                    {"usedPercent", 17},
                    {"windowDurationMins", 10080},
                    {"resetsAt", 1894060800}
                }},
                {"credits", QJsonObject{
                    {"hasCredits", true},
                    {"unlimited", false},
                    {"balance", "12.50"}
                }}
            }}
        };
        QJsonObject account{
            {"account", QJsonObject{
                {"type", "chatgpt"},
                {"email", "dev@example.com"},
                {"planType", "pro"}
            }},
            {"requiresOpenaiAuth", false}
        };

        ProviderFetchResult result = CodexAppServerStrategy::mapRpcResult(rateLimits, account);

        QVERIFY(result.success);
        QCOMPARE(result.strategyID, QString("codex.cli.rpc"));
        QVERIFY(result.usage.primary.has_value());
        QCOMPARE(result.usage.primary->usedPercent, 42.0);
        QCOMPARE(result.usage.primary->windowMinutes.value(), 300);
        QCOMPARE(result.usage.primary->resetsAt->toSecsSinceEpoch(), qint64(1893456000));
        QVERIFY(result.usage.secondary.has_value());
        QCOMPARE(result.usage.secondary->usedPercent, 17.0);
        QCOMPARE(result.usage.secondary->windowMinutes.value(), 10080);
        QVERIFY(result.credits.has_value());
        QCOMPARE(result.credits->remaining, 12.5);
        QVERIFY(result.usage.providerCost.has_value());
        QCOMPARE(result.usage.providerCost->used, 12.5);
        QVERIFY(result.usage.identity.has_value());
        QCOMPARE(result.usage.identity->accountEmail.value(), QString("dev@example.com"));
        QCOMPARE(result.usage.identity->loginMethod.value(), QString("pro"));
    }

    void prefersCodexBucketFromRpcRateLimitsByLimitId() {
        QJsonObject result{
            {"rateLimits", QJsonObject{
                {"primary", QJsonObject{{"usedPercent", 90}, {"windowDurationMins", 300}}}
            }},
            {"rateLimitsByLimitId", QJsonObject{
                {"other", QJsonObject{
                    {"primary", QJsonObject{{"usedPercent", 80}, {"windowDurationMins", 300}}}
                }},
                {"codex", QJsonObject{
                    {"limitId", "codex"},
                    {"primary", QJsonObject{{"usedPercent", 11}, {"windowDurationMins", 300}}},
                    {"secondary", QJsonObject{{"usedPercent", 22}, {"windowDurationMins", 10080}}}
                }}
            }}
        };

        ProviderFetchResult mapped = CodexAppServerStrategy::mapRpcResult(result);

        QVERIFY(mapped.success);
        QVERIFY(mapped.usage.primary.has_value());
        QCOMPARE(mapped.usage.primary->usedPercent, 11.0);
        QVERIFY(mapped.usage.secondary.has_value());
        QCOMPARE(mapped.usage.secondary->usedPercent, 22.0);
    }

    void mapsRpcErrorsToFallbackFriendlyFailure() {
        QJsonObject error{
            {"code", -32603},
            {"message", "body={\"rate_limit\":{\"primary_window\":{\"used_percent\":9,\"limit_window_seconds\":300,\"reset_at\":1893456000}},\"plan_type\":\"plus\",\"email\":\"dev@example.com\"}"}
        };

        ProviderFetchResult result = CodexAppServerStrategy::mapRpcError(error);

        QVERIFY(result.success);
        QVERIFY(result.usage.primary.has_value());
        QCOMPARE(result.usage.primary->usedPercent, 9.0);
        QVERIFY(result.usage.identity.has_value());
        QCOMPARE(result.usage.identity->accountEmail.value(), QString("dev@example.com"));
        QCOMPARE(result.usage.identity->loginMethod.value(), QString("plus"));
    }
};

QTEST_MAIN(tst_CodexProvider)
#include "tst_CodexProvider.moc"
