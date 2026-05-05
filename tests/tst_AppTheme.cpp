#include "app/AppTheme.h"

#include <QColor>
#include <QtTest/QtTest>
#include <QVariant>
#include <QVariantMap>

class tst_AppTheme : public QObject {
    Q_OBJECT

private slots:
    void runtimeColorsAreOpaque();
};

void tst_AppTheme::runtimeColorsAreOpaque() {
    const QVariantMap theme = makeAppTheme();
    const QStringList colorKeys = {
        "bgPrimary",
        "bgSecondary",
        "bgCard",
        "bgHover",
        "bgSelected",
        "bgPressed",
        "borderColor",
        "borderAccent",
        "textPrimary",
        "textSecondary",
        "textTertiary",
        "textDisabled",
        "statusOk",
        "statusDegraded",
        "statusOutage",
        "statusUnknown",
        "accentColor",
        "accentHover",
    };

    for (const QString& key : colorKeys) {
        QVERIFY2(theme.contains(key), qPrintable(QString("missing theme key %1").arg(key)));

        const QColor color = theme.value(key).value<QColor>();
        QVERIFY2(color.isValid(), qPrintable(QString("invalid theme color %1").arg(key)));
        QCOMPARE(color.alpha(), 255);
    }
}

QTEST_MAIN(tst_AppTheme)
#include "tst_AppTheme.moc"
