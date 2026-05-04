#include <QtTest/QtTest>
#include "../src/providers/perplexity/PerplexityProvider.h"
#include "../src/models/UsageSnapshot.h"

class tst_PerplexityProvider : public QObject {
    Q_OBJECT
private slots:
    void parseCredits() {
        QJsonObject json;
        json["balance_cents"] = 2000;
        json["current_period_purchased_cents"] = 500;
        json["total_usage_cents"] = 1500;
        json["renewal_date_ts"] = 1715212800.0;
        QJsonArray grants;
        QJsonObject g1, g2;
        g1["type"] = "recurring"; g1["amount_cents"] = 500;
        g2["type"] = "promotional"; g2["amount_cents"] = 1000;
        grants.append(g1); grants.append(g2);
        json["credit_grants"] = grants;

        ProviderFetchResult r = PerplexityWebStrategy::parseResponse(json);
        QVERIFY(r.success);
        QVERIFY(r.usage.primary.has_value());
        QVERIFY(r.usage.secondary.has_value());
        QVERIFY(r.usage.tertiary.has_value());
    }

    void emptyResponse() {
        QJsonObject json;
        ProviderFetchResult r = PerplexityWebStrategy::parseResponse(json);
        QVERIFY(!r.success);
    }
};

QTEST_MAIN(tst_PerplexityProvider)
#include "tst_PerplexityProvider.moc"
