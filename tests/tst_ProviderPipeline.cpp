#include <QtTest/QtTest>

#include "../src/providers/IProvider.h"
#include "../src/providers/ProviderPipeline.h"

#include <utility>

class FakeStrategy : public IFetchStrategy {
public:
    FakeStrategy(QString id, bool available, bool success, bool fallback,
                 int kind = ProviderFetchKind::APIToken)
        : m_id(std::move(id))
        , m_available(available)
        , m_success(success)
        , m_fallback(fallback)
        , m_kind(kind)
    {}

    QString id() const override { return m_id; }
    int kind() const override { return m_kind; }
    bool isAvailable(const ProviderFetchContext&) const override { return m_available; }

    ProviderFetchResult fetchSync(const ProviderFetchContext&) override {
        ++calls;
        ProviderFetchResult result;
        result.strategyID = m_id;
        result.strategyKind = kind();
        result.sourceLabel = m_id;
        result.success = m_success;
        if (!m_success) result.errorMessage = m_id + " failed";
        return result;
    }

    bool shouldFallback(const ProviderFetchResult&, const ProviderFetchContext&) const override {
        return m_fallback;
    }

    int calls = 0;

private:
    QString m_id;
    bool m_available;
    bool m_success;
    bool m_fallback;
    int m_kind;
};

class SourceProvider : public IProvider {
public:
    QString id() const override { return "source"; }
    QString displayName() const override { return "Source"; }
    QString sessionLabel() const override { return "Session"; }
    QString weeklyLabel() const override { return "Weekly"; }
    bool defaultEnabled() const override { return false; }

    QVector<IFetchStrategy*> createStrategies(const ProviderFetchContext&) override {
        return {
            new FakeStrategy("oauth", true, true, false, ProviderFetchKind::OAuth),
            new FakeStrategy("web", true, true, false, ProviderFetchKind::Web),
            new FakeStrategy("api", true, true, false, ProviderFetchKind::APIToken)
        };
    }
};

class tst_ProviderPipeline : public QObject {
    Q_OBJECT
private slots:
    void skipsUnavailableAndReturnsSuccess() {
        ProviderPipeline pipeline;
        ProviderFetchContext ctx;
        FakeStrategy unavailable("unavailable", false, true, false);
        FakeStrategy success("success", true, true, false);
        QVector<IFetchStrategy*> strategies = { &unavailable, &success };

        ProviderFetchResult result = pipeline.execute(strategies, ctx);

        QVERIFY(result.success);
        QCOMPARE(result.strategyID, QString("success"));
        QCOMPARE(unavailable.calls, 0);
        QCOMPARE(success.calls, 1);
    }

    void fallsBackAfterFailure() {
        ProviderPipeline pipeline;
        ProviderFetchContext ctx;
        FakeStrategy first("first", true, false, true);
        FakeStrategy second("second", true, true, false);
        QVector<IFetchStrategy*> strategies = { &first, &second };

        ProviderFetchResult result = pipeline.execute(strategies, ctx);

        QVERIFY(result.success);
        QCOMPARE(result.strategyID, QString("second"));
        QCOMPARE(first.calls, 1);
        QCOMPARE(second.calls, 1);
    }

    void stopsWhenFallbackIsDisabled() {
        ProviderPipeline pipeline;
        ProviderFetchContext ctx;
        FakeStrategy first("first", true, false, false);
        FakeStrategy second("second", true, true, false);
        QVector<IFetchStrategy*> strategies = { &first, &second };

        ProviderFetchResult result = pipeline.execute(strategies, ctx);

        QVERIFY(!result.success);
        QCOMPARE(result.errorMessage, QString("first failed"));
        QCOMPARE(first.calls, 1);
        QCOMPARE(second.calls, 0);
    }

    void filtersStrategiesBySourceMode() {
        ProviderPipeline pipeline;
        ProviderFetchContext ctx;
        ctx.sourceMode = ProviderSourceMode::Web;
        SourceProvider provider;

        QVector<IFetchStrategy*> strategies = pipeline.resolveStrategies(&provider, ctx);

        QCOMPARE(strategies.size(), 1);
        QCOMPARE(strategies.first()->id(), QString("web"));
        qDeleteAll(strategies);
    }
};

QTEST_MAIN(tst_ProviderPipeline)
#include "tst_ProviderPipeline.moc"
