#include <QtTest/QtTest>
#include "cli/CLIEntry.h"
#include "cli/CLIRenderer.h"

class tst_CLI : public QObject {
    Q_OBJECT

private slots:
    void isCliCommandDetection();
    void rendererTextMode();
    void rendererJsonMode();
};

void tst_CLI::isCliCommandDetection()
{
    QVERIFY(CLIEntry::isCliCommand("usage"));
    QVERIFY(CLIEntry::isCliCommand("cost"));
    QVERIFY(CLIEntry::isCliCommand("config"));
    QVERIFY(!CLIEntry::isCliCommand("help"));
    QVERIFY(!CLIEntry::isCliCommand(""));
    QVERIFY(!CLIEntry::isCliCommand("gui"));
}

void tst_CLI::rendererTextMode()
{
    CLIRenderer renderer(false, false, true); // text, no pretty, no color
    renderer.heading("Test Heading");
    renderer.line("Test line");
    renderer.bullet("Bullet point");
    renderer.error("Error message");
    renderer.blank();
    // Just verify no crash; output goes to stdout
    QVERIFY(!renderer.isJsonMode());
}

void tst_CLI::rendererJsonMode()
{
    CLIRenderer renderer(true, false, true); // json, no pretty, no color
    QJsonObject obj;
    obj["test"] = "value";
    renderer.setRootObject(obj);
    renderer.flush();
    QVERIFY(renderer.isJsonMode());
}

QTEST_MAIN(tst_CLI)
#include "tst_CLI.moc"
