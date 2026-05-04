#include <QtTest/QtTest>
#include "../src/providers/jetbrains/JetBrainsProvider.h"
#include "../src/models/UsageSnapshot.h"

class tst_JetBrainsProvider : public QObject {
    Q_OBJECT
private slots:
    void parseQuotaXml() {
        QString xml = R"(<option name="AIAssistant.Default.AIQuotaManager2" value="{&quot;quotaInfo&quot;:{&quot;type&quot;:&quot;subscription&quot;,&quot;current&quot;:&quot;25&quot;,&quot;maximum&quot;:&quot;100&quot;},&quot;nextRefill&quot;:{&quot;next&quot;:&quot;2026-05-10T00:00:00Z&quot;}}" />)";
        ProviderFetchResult r = JetBrainsLocalStrategy::parseXml(xml);
        QVERIFY(r.success);
        QVERIFY(r.usage.primary.has_value());
        QCOMPARE(r.usage.primary->usedPercent, 25.0);
        QVERIFY(r.usage.primary->resetsAt.has_value());
    }

    void noQuotaData() {
        QString xml = "<option name=\"other\" value=\"x\" />";
        ProviderFetchResult r = JetBrainsLocalStrategy::parseXml(xml);
        QVERIFY(!r.success);
    }
};

QTEST_MAIN(tst_JetBrainsProvider)
#include "tst_JetBrainsProvider.moc"
