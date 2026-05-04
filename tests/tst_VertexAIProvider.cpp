#include <QtTest/QtTest>
#include "../src/providers/vertexai/VertexAIProvider.h"

class tst_VertexAIProvider : public QObject {
    Q_OBJECT
private slots:
    void providerRegistration() {
        VertexAIProvider p;
        QCOMPARE(p.id(), QString("vertexai"));
        QCOMPARE(p.displayName(), QString("Vertex AI"));
        QCOMPARE(p.brandColor(), QString("#4285F4"));
        ProviderFetchContext ctx;
        auto s = p.createStrategies(ctx);
        QCOMPARE(s.size(), 1);
        QCOMPARE(s[0]->id(), QString("vertexai.oauth"));
        qDeleteAll(s);
    }
};

QTEST_MAIN(tst_VertexAIProvider)
#include "tst_VertexAIProvider.moc"
