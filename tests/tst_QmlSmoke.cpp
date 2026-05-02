#include <QGuiApplication>
#include <QtTest/QtTest>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <QVariantMap>

class MockSettingsStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool debugMenuEnabled READ debugMenuEnabled CONSTANT)
    Q_PROPERTY(bool mergeIcons READ mergeIcons CONSTANT)
    Q_PROPERTY(bool usageBarsShowUsed READ usageBarsShowUsed CONSTANT)
    Q_PROPERTY(bool resetTimesShowAbsolute READ resetTimesShowAbsolute CONSTANT)
    Q_PROPERTY(bool showOptionalCreditsAndExtraUsage READ showOptionalCreditsAndExtraUsage CONSTANT)
    Q_PROPERTY(bool statusChecksEnabled READ statusChecksEnabled CONSTANT)
    Q_PROPERTY(bool sessionQuotaNotificationsEnabled READ sessionQuotaNotificationsEnabled CONSTANT)
    Q_PROPERTY(int refreshFrequency READ refreshFrequency CONSTANT)
    Q_PROPERTY(QString language READ language CONSTANT)
public:
    bool debugMenuEnabled() const { return true; }
    bool mergeIcons() const { return true; }
    bool usageBarsShowUsed() const { return false; }
    bool resetTimesShowAbsolute() const { return false; }
    bool showOptionalCreditsAndExtraUsage() const { return true; }
    bool statusChecksEnabled() const { return true; }
    bool sessionQuotaNotificationsEnabled() const { return false; }
    int refreshFrequency() const { return 15; }
    QString language() const { return "en"; }
    bool launchAtLogin() const { return false; }
    int settingsRevision() const { return 0; }

    Q_INVOKABLE void setProviderEnabled(const QString&, bool) {}
    Q_INVOKABLE bool isProviderEnabled(const QString&) const { return false; }
    Q_INVOKABLE QVariant providerSetting(const QString&, const QString&, const QVariant& def = {}) const { return def; }
    Q_INVOKABLE void setProviderSetting(const QString&, const QString&, const QVariant&) {}
    Q_INVOKABLE QVariantList providerOrder() const { return {}; }
    Q_INVOKABLE void setProviderOrder(const QVariantList&) {}
    Q_INVOKABLE void setMergeIcons(bool) {}
    Q_INVOKABLE void setUsageBarsShowUsed(bool) {}
    Q_INVOKABLE void setResetTimesShowAbsolute(bool) {}
    Q_INVOKABLE void setShowOptionalCreditsAndExtraUsage(bool) {}
    Q_INVOKABLE void setStatusChecksEnabled(bool) {}
    Q_INVOKABLE void setSessionQuotaNotificationsEnabled(bool) {}
    Q_INVOKABLE void setRefreshFrequency(int) {}
    Q_INVOKABLE void setLanguage(const QString&) {}
    Q_INVOKABLE void setDebugMenuEnabled(bool) {}

signals:
    void debugMenuEnabledChanged();
    void mergeIconsChanged();
    void usageBarsShowUsedChanged();
    void resetTimesShowAbsoluteChanged();
    void showOptionalCreditsAndExtraUsageChanged();
    void statusChecksEnabledChanged();
    void sessionQuotaNotificationsEnabledChanged();
};

class MockUsageStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList providerIDs READ providerIDs CONSTANT)
    Q_PROPERTY(bool isRefreshing READ isRefreshing CONSTANT)
    Q_PROPERTY(bool costUsageEnabled READ costUsageEnabled CONSTANT)
    Q_PROPERTY(bool costUsageRefreshing READ costUsageRefreshing CONSTANT)
    Q_PROPERTY(int snapshotRevision READ snapshotRevision CONSTANT)
    Q_PROPERTY(int statusRevision READ statusRevision CONSTANT)
    Q_PROPERTY(QVariantMap codexAccountState READ codexAccountState CONSTANT)
    Q_PROPERTY(QVariantList codexFetchAttempts READ codexFetchAttempts CONSTANT)
public:
    QStringList providerIDs() const { return {"codex", "claude", "cursor"}; }
    QStringList allProviderIDs() const { return {"codex", "claude", "cursor", "zai"}; }
    bool isRefreshing() const { return false; }
    bool costUsageEnabled() const { return false; }
    bool costUsageRefreshing() const { return false; }
    int snapshotRevision() const { return 0; }
    int statusRevision() const { return 0; }
    bool isProviderEnabled(const QString&) const { return true; }
    QVariantMap snapshotData(const QString&) const {
        QVariantMap m;
        m["primaryUsed"] = 25.0; m["primaryRemaining"] = 75.0;
        m["primaryDisplayPercent"] = 75.0; m["primaryDisplayIsUsed"] = false;
        m["secondaryUsed"] = 10.0; m["secondaryRemaining"] = 90.0;
        m["secondaryDisplayPercent"] = 90.0; m["secondaryDisplayIsUsed"] = false;
        m["hasSecondary"] = true; m["hasTertiary"] = false;
        m["sessionLabel"] = "Session"; m["weeklyLabel"] = "Weekly";
        m["displayName"] = "Codex"; m["supportsCredits"] = false;
        m["hasUsage"] = true; m["updatedAt"] = 0;
        return m;
    }
    QVariantList providerList() const {
        auto makeEntry = [](const QString& id, const QString& name, bool enabled) {
            QVariantMap e; e["id"] = id; e["name"] = name; e["enabled"] = enabled;
            e["sessionLabel"] = "Session"; e["weeklyLabel"] = "Weekly";
            e["supportsCredits"] = false;
            return e;
        };
        return {makeEntry("codex", "Codex", true), makeEntry("claude", "Claude", true)};
    }
    Q_INVOKABLE QString providerDisplayName(const QString& id) const {
        if (id == "codex") return "Codex";
        if (id == "claude") return "Claude";
        return id;
    }
    Q_INVOKABLE QString providerError(const QString&) const { return {}; }
    Q_INVOKABLE QVariantMap providerStatus(const QString&) const { return {{"state", "ok"}}; }
    Q_INVOKABLE QVariantMap providerConnectionTest(const QString&) const { return {{"state", "idle"}}; }
    Q_INVOKABLE QVariantMap providerSecretStatus(const QString&, const QString&) const { return {{"configured", false}}; }
    Q_INVOKABLE QVariantList providerSettingsFields(const QString&) const { return {}; }
    Q_INVOKABLE QVariantMap providerDescriptorData(const QString&) const { return {}; }
    Q_INVOKABLE QString codexActiveAccountID() const { return "live-system"; }
    Q_INVOKABLE QVariantList codexAccounts() const { return {}; }
    Q_INVOKABLE QVariantMap codexAccountState() const { return {}; }
    Q_INVOKABLE QVariantList codexFetchAttempts() const { return {}; }
    Q_INVOKABLE QVariantList utilizationChartData(const QString&, const QString&) const { return {}; }
    Q_INVOKABLE QVariantMap costUsageData() const { return {}; }
    Q_INVOKABLE QVariantList providerCostUsageList() const { return {}; }
    Q_INVOKABLE QVariantMap providerCostUsageData(const QString&) const { return {}; }
    Q_INVOKABLE QVariantMap providerDashboardData(const QString&) const { return {}; }
    Q_INVOKABLE QVariantMap codexConsumerProjectionData() const { return {}; }
    Q_INVOKABLE void refresh() {}
    Q_INVOKABLE void refreshAll() {}
    Q_INVOKABLE void refreshCostUsage() {}
    Q_INVOKABLE void refreshProvider(const QString&) {}
    Q_INVOKABLE void setProviderEnabled(const QString&, bool) {}
    Q_INVOKABLE void setProviderSetting(const QString&, const QString&, const QVariant&) {}
    Q_INVOKABLE bool setProviderSecret(const QString&, const QString&, const QString&) { return false; }
    Q_INVOKABLE bool clearProviderSecret(const QString&, const QString&) { return false; }
    Q_INVOKABLE void testProviderConnection(const QString&) {}
    Q_INVOKABLE void startProviderLogin(const QString&) {}
    Q_INVOKABLE void cancelProviderLogin(const QString&) {}
    Q_INVOKABLE void refreshProviderStatuses() {}
    Q_INVOKABLE void moveProvider(int, int) {}
    Q_INVOKABLE void updateProviderIDs() {}

signals:
    void snapshotChanged(const QString&);
    void providerIDsChanged();
    void refreshingChanged();
    void costUsageEnabledChanged();
    void costUsageRefreshingChanged();
    void costUsageChanged();
    void snapshotRevisionChanged();
    void providerConnectionTestChanged(const QString&);
    void providerLoginStateChanged(const QString&);
    void providerStatusChanged(const QString&);
    void statusRevisionChanged();
    void codexAccountsChanged();
    void codexActiveAccountChanged(const QString&);
    void codexAccountStateChanged();
    void codexFetchAttemptsChanged();
};

class MockLanguageManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString language READ language CONSTANT)
public:
    QString language() const { return "en"; }
    Q_INVOKABLE QString translate(const QString& key) const { return key; }

signals:
    void retranslate();
};

class MockAppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool settingsVisible READ isSettingsVisible CONSTANT)
    Q_PROPERTY(bool settingsMaximized READ isSettingsMaximized CONSTANT)
    Q_PROPERTY(bool usageVisible READ isUsageVisible CONSTANT)
public:
    bool isSettingsVisible() const { return false; }
    bool isSettingsMaximized() const { return false; }
    bool isUsageVisible() const { return false; }
    Q_INVOKABLE void openSettings() {}
    Q_INVOKABLE void closeSettings() {}
    Q_INVOKABLE void toggleSettings() {}
    Q_INVOKABLE void startSettingsMove() {}
    Q_INVOKABLE void startSettingsResize(int) {}
    Q_INVOKABLE void minimizeSettings() {}
    Q_INVOKABLE void toggleSettingsMaximized() {}
    Q_INVOKABLE void openUsage() {}
    Q_INVOKABLE void closeUsage() {}
    Q_INVOKABLE void startUsageMove() {}
    Q_INVOKABLE void startUsageResize(int) {}
    Q_INVOKABLE void minimizeUsage() {}
    Q_INVOKABLE void moveTrayPanel(int, int) {}
    Q_INVOKABLE void quitApp() {}
    Q_INVOKABLE void openExternalUrl(const QString&) {}
    Q_INVOKABLE void copyText(const QString&) {}

signals:
    void settingsVisibleChanged();
    void settingsMaximizedChanged();
    void usageVisibleChanged();
};

class tst_QmlSmoke : public QObject {
    Q_OBJECT
private:
    MockSettingsStore mockSettings;
    MockUsageStore mockUsage;
    MockLanguageManager mockLang;
    MockAppController mockAppCtrl;

    void setupEngine(QQmlEngine& engine) {
        engine.addImportPath("qrc:/qml");
        qmlRegisterSingletonInstance("CodexBar", 1, 0, "SettingsStore", &mockSettings);
        qmlRegisterSingletonInstance("CodexBar", 1, 0, "UsageStore", &mockUsage);
        qmlRegisterSingletonInstance("CodexBar", 1, 0, "AppController", &mockAppCtrl);
        qmlRegisterSingletonInstance("CodexBar", 1, 0, "LanguageManager", &mockLang);
    }

private slots:
    void basicQmlEngineWorks() {
        QQmlEngine engine;
        setupEngine(engine);

        QByteArray qml = "import QtQuick 2.15; Rectangle { width: 100; height: 100; color: 'red' }";
        QQmlComponent component(&engine);
        component.setData(qml, QUrl());
        QVERIFY2(component.status() == QQmlComponent::Ready,
                 qPrintable(component.errorString()));

        QObject* root = component.create();
        QVERIFY(root != nullptr);
        QCOMPARE(root->property("width").toInt(), 100);
        delete root;
    }

    void settingsWindowLoads() {
        QQmlEngine engine;
        setupEngine(engine);

        QQmlComponent component(&engine, QUrl("qrc:/qml/SettingsWindow.qml"));
        if (component.status() == QQmlComponent::Error) {
            qWarning() << "SettingsWindow load errors:" << component.errorString();
        }
        QVERIFY2(component.status() == QQmlComponent::Ready,
                 qPrintable(component.errorString()));

        QQuickItem* root = qobject_cast<QQuickItem*>(component.create());
        QVERIFY(root != nullptr);
        delete root;
    }

    void trayPanelLoads() {
        QQmlEngine engine;
        setupEngine(engine);

        QQmlComponent component(&engine, QUrl("qrc:/qml/TrayPanel.qml"));
        if (component.status() == QQmlComponent::Error) {
            qWarning() << "TrayPanel load errors:" << component.errorString();
        }
        QVERIFY2(component.status() == QQmlComponent::Ready,
                 qPrintable(component.errorString()));

        QQuickItem* root = qobject_cast<QQuickItem*>(component.create());
        QVERIFY(root != nullptr);
        delete root;
    }

    void settingsWindowRenders() {
        QQuickView view;
        setupEngine(*view.engine());
        view.setSource(QUrl("qrc:/qml/SettingsWindow.qml"));
        view.show();
        QTest::qWait(400);

        QImage screenshot = view.grabWindow();
        QVERIFY(!screenshot.isNull());
        QVERIFY(screenshot.width() > 100);
        QVERIFY(screenshot.height() > 100);

        QColor pixel = screenshot.pixelColor(10, 10);
        bool isDark = pixel.red() < 60 && pixel.green() < 60 && pixel.blue() < 60;
        QVERIFY2(isDark, qPrintable(
            QString("Expected dark pixel at (10,10), got RGB(%1,%2,%3)")
                .arg(pixel.red()).arg(pixel.green()).arg(pixel.blue())));

        view.hide();
    }

    void trayPanelRenders() {
        QQuickView view;
        setupEngine(*view.engine());
        view.setSource(QUrl("qrc:/qml/TrayPanel.qml"));
        view.show();
        QTest::qWait(400);

        QImage screenshot = view.grabWindow();
        QVERIFY(!screenshot.isNull());
        QVERIFY(screenshot.width() > 50);
        QVERIFY(screenshot.height() > 50);

        view.hide();
    }

    void settingsWindowTabInteraction() {
        QQuickView view;
        setupEngine(*view.engine());
        view.setSource(QUrl("qrc:/qml/SettingsWindow.qml"));
        view.show();
        QTest::qWait(400);

        QQuickItem* root = view.rootObject();
        QVERIFY(root != nullptr);

        // Verify the window loaded with content by checking it has child items
        QVERIFY(root->childItems().size() > 0);

        view.hide();
    }
};

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    tst_QmlSmoke tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_QmlSmoke.moc"
