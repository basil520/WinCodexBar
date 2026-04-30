#include <QtTest>

#include <QFileInfo>

#include "app/LanguageManager.h"
#include "app/Localization.h"
#include "models/UsagePace.h"
#include "util/UsagePaceText.h"

class LocalizationTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        qputenv("WINCODEXBAR_TRANSLATION_DIR", TEST_TRANSLATION_DIR);
        const QString zhPath = QStringLiteral(TEST_TRANSLATION_DIR) + "/WinCodexBar_zh_CN.qm";
        if (!QFileInfo::exists(zhPath)) {
            QSKIP("Compiled zh_CN translation is not available.");
        }
    }

    void testLanguageRevisionChanges() {
        auto& manager = LanguageManager::instance();
        manager.setLanguage("en");

        const int before = manager.translationRevision();
        QSignalSpy revisionSpy(&manager, &LanguageManager::translationRevisionChanged);

        manager.setLanguage("zh_CN");

        QVERIFY(manager.translationRevision() > before);
        QCOMPARE(revisionSpy.count(), 1);
    }

    void testProviderTextTranslatesAndReturnsToEnglish() {
        auto& manager = LanguageManager::instance();
        manager.setLanguage("zh_CN");

        QCOMPARE(Localization::providerLabel("Session"), QString::fromUtf8("会话"));
        QCOMPARE(Localization::providerSettingLabel("Data source"), QString::fromUtf8("数据来源"));
        QCOMPARE(Localization::providerError("no available fetch strategy"),
                 QString::fromUtf8("没有可用的获取策略"));

        manager.setLanguage("en");

        QCOMPARE(Localization::providerLabel("Session"), QString("Session"));
        QCOMPARE(Localization::providerSettingLabel("Data source"), QString("Data source"));
        QCOMPARE(Localization::providerError("no available fetch strategy"),
                 QString("no available fetch strategy"));
    }

    void testUsagePaceTextTranslates() {
        auto& manager = LanguageManager::instance();
        manager.setLanguage("zh_CN");

        UsagePace pace;
        pace.stage = UsagePace::Stage::onTrack;
        pace.willLastToReset = true;

        auto detail = UsagePaceText::weeklyDetail(pace);
        QCOMPARE(detail.leftLabel, QString::fromUtf8("节奏正常"));
        QCOMPARE(detail.rightLabel, QString::fromUtf8("可持续到重置"));

        manager.setLanguage("en");
    }
};

QTEST_MAIN(LocalizationTest)
#include "tst_Localization.moc"
