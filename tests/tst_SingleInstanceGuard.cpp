#include "../src/util/SingleInstanceGuard.h"

#include <QtTest/QtTest>
#include <QUuid>
#include <memory>

class tst_SingleInstanceGuard : public QObject {
    Q_OBJECT

private slots:
    void keepsLockUntilOwnerIsDestroyed();
};

void tst_SingleInstanceGuard::keepsLockUntilOwnerIsDestroyed()
{
    const QString key = QStringLiteral("WinCodexBar_Test_%1")
        .arg(QUuid::createUuid().toString(QUuid::Id128));

    auto first = std::make_unique<SingleInstanceGuard>(key);
    QVERIFY(first->acquired());
    QVERIFY(!first->alreadyRunning());

    {
        SingleInstanceGuard second(key);
        QVERIFY(!second.acquired());
        QVERIFY(second.alreadyRunning());
    }

    {
        SingleInstanceGuard secondWhileFirstStillAlive(key);
        QVERIFY(!secondWhileFirstStillAlive.acquired());
        QVERIFY(secondWhileFirstStillAlive.alreadyRunning());
    }

    first.reset();

    SingleInstanceGuard afterOwnerDestroyed(key);
    QVERIFY(afterOwnerDestroyed.acquired());
    QVERIFY(!afterOwnerDestroyed.alreadyRunning());
}

QTEST_MAIN(tst_SingleInstanceGuard)
#include "tst_SingleInstanceGuard.moc"
