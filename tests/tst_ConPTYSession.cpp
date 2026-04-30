#include <QtTest/QtTest>

#include "../src/providers/shared/ConPTYSession.h"

#include <QProcessEnvironment>
#include <QRegularExpression>

class tst_ConPTYSession : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        qDebug() << "ConPTY available:" << ConPTYSession::isConPtyAvailable();
    }

    void capturesInteractiveOutput() {
        QString cmd = qEnvironmentVariable("ComSpec", "C:/Windows/System32/cmd.exe");

        ConPTYSession session;
        bool started = session.start(cmd, {"/C", "echo", "READY"}, QProcessEnvironment());
        QVERIFY2(started, "Could not start session");

        bool found = session.waitForPattern(QRegularExpression("READY"), 5000);
        if (!found) {
            QByteArray allOutput = session.readOutput(0);
            qDebug() << "Buffer content:" << allOutput;
        }
        QVERIFY2(found, "Did not find READY in output");

        session.terminate();
    }

    void usesNativePseudoConsoleWhenAvailable() {
        if (!ConPTYSession::isConPtyAvailable()) {
            QSKIP("ConPTY is not available on this Windows version");
        }

        QString powershell = "C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe";
        ConPTYSession session;
        bool started = session.start(
            powershell,
            {"-NoLogo", "-NoProfile", "-NonInteractive", "-Command",
             "[Console]::IsInputRedirected; [Console]::IsOutputRedirected"},
            QProcessEnvironment());
        QVERIFY2(started, "Could not start PowerShell in ConPTY session");

        bool found = session.waitForPattern(QRegularExpression("False[\\s\\S]*False"), 8000);
        if (!found) {
            QByteArray allOutput = session.readOutput(0);
            qDebug() << "Buffer content:" << allOutput;
        }
        QVERIFY2(found, "Process did not observe native console handles");

        session.terminate();
    }

    void canWriteAndRead() {
        QString cmd = qEnvironmentVariable("ComSpec", "C:/Windows/System32/cmd.exe");

        ConPTYSession session;
        bool started = session.start(cmd, {}, QProcessEnvironment());
        if (!started) {
            QSKIP("Could not start session");
        }

        QTest::qWait(200);

        bool wrote = session.write("echo PONG\r\n");
        QVERIFY2(wrote, "Failed to write to session");

        bool found = session.waitForPattern(QRegularExpression("PONG"), 8000);
        if (!found) {
            QByteArray allOutput = session.readOutput(0);
            qDebug() << "Buffer content:" << allOutput;
        }
        QVERIFY2(found, "Did not find PONG in output");

        session.write("exit\r\n");
        QTest::qWait(200);

        session.terminate();
    }
};

QTEST_MAIN(tst_ConPTYSession)
#include "tst_ConPTYSession.moc"
