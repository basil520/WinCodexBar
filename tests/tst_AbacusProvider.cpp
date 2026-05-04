#include <QtTest/QtTest>
#include "../src/providers/abacus/AbacusProvider.h"

class tst_AbacusProvider : public QObject {
    Q_OBJECT
private slots:
    void providerMetadata() {
        AbacusProvider p;
        QCOMPARE(p.id(), QString("abacus"));
        QCOMPARE(p.displayName(), QString("Abacus AI"));
        QCOMPARE(p.brandColor(), QString("#6366F1"));
        ProviderFetchContext ctx;
        auto s = p.createStrategies(ctx);
        QCOMPARE(s.size(), 1);
        QCOMPARE(s[0]->id(), QString("abacus.web"));
        qDeleteAll(s);
    }
};

QTEST_MAIN(tst_AbacusProvider)
#include "tst_AbacusProvider.moc"
