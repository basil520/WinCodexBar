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
    Q_PROPERTY(bool costUsageEnabled READ costUsageEnabled NOTIFY costUsageEnabledChanged)
    Q_PROPERTY(bool costUsageRefreshing READ costUsageRefreshing CONSTANT)
    Q_PROPERTY(int snapshotRevision READ snapshotRevision CONSTANT)
    Q_PROPERTY(int statusRevision READ statusRevision CONSTANT)
    Q_PROPERTY(QVariantMap codexAccountState READ codexAccountState CONSTANT)
    Q_PROPERTY(QVariantList codexFetchAttempts READ codexFetchAttempts CONSTANT)
public:
    QStringList providerIDs() const {
        return providerIDsForTest.isEmpty()
            ? QStringList({"codex", "claude", "cursor"})
            : providerIDsForTest;
    }
    Q_INVOKABLE QStringList allProviderIDs() const { return {"codex", "claude", "cursor", "zai"}; }
    bool isRefreshing() const { return false; }
    bool costUsageEnabled() const { return costUsageEnabledValue; }
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
    Q_INVOKABLE QVariantList providerList() const {
        ++providerListCalls;
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
    Q_INVOKABLE QVariantMap providerUsageSnapshot(const QString&) const { return {}; }
    Q_INVOKABLE QVariantMap providerConnectionTest(const QString&) const { return {{"state", "idle"}}; }
    Q_INVOKABLE QVariantMap providerSecretStatus(const QString&, const QString&) const { return {{"configured", false}}; }
    Q_INVOKABLE QVariantList providerSettingsFields(const QString&) const { return {}; }
    Q_INVOKABLE QVariantMap providerDescriptorData(const QString&) const {
        ++providerDescriptorCalls;
        return {};
    }
    Q_INVOKABLE QVariantList tokenAccountsForProvider(const QString& providerId) const {
        ++tokenAccountsForProviderCalls;
        if (providerId == tokenAccountProviderForTest) {
            return tokenAccountsForProviderValue;
        }
        return {};
    }
    Q_INVOKABLE QString addTokenAccount(const QString& providerId, const QString& displayName, int sourceMode) {
        ++addTokenAccountCalls;
        lastTokenProvider = providerId;
        lastTokenDisplayName = displayName;
        lastTokenSourceMode = sourceMode;
        emit tokenAccountsChanged(providerId);
        return "token-account-1";
    }
    Q_INVOKABLE QString addTokenAccountWithApiKey(const QString& providerId, const QString& displayName, int sourceMode, const QString& apiKey) {
        ++addTokenAccountWithApiKeyCalls;
        lastTokenProvider = providerId;
        lastTokenDisplayName = displayName;
        lastTokenSourceMode = sourceMode;
        lastTokenApiKey = apiKey;
        emit tokenAccountsChanged(providerId);
        return "token-account-1";
    }
    Q_INVOKABLE bool removeTokenAccount(const QString& accountId) {
        ++removeTokenAccountCalls;
        lastTokenAccountId = accountId;
        emit tokenAccountsChanged(lastTokenProvider);
        return true;
    }
    Q_INVOKABLE bool setTokenAccountVisibility(const QString& accountId, int visibility) {
        ++setTokenAccountVisibilityCalls;
        lastTokenAccountId = accountId;
        lastTokenVisibility = visibility;
        return true;
    }
    Q_INVOKABLE bool setTokenAccountSourceMode(const QString& accountId, int sourceMode) {
        ++setTokenAccountSourceModeCalls;
        lastTokenAccountId = accountId;
        lastTokenSourceMode = sourceMode;
        return true;
    }
    Q_INVOKABLE bool setDefaultTokenAccount(const QString& providerId, const QString& accountId) {
        ++setDefaultTokenAccountCalls;
        lastTokenProvider = providerId;
        lastTokenAccountId = accountId;
        if (providerId == tokenAccountProviderForTest) {
            defaultTokenAccountValue = accountId;
        }
        emit tokenAccountsChanged(providerId);
        return true;
    }
    Q_INVOKABLE QString defaultTokenAccount(const QString& providerId) const {
        return providerId == tokenAccountProviderForTest ? defaultTokenAccountValue : QString();
    }
    Q_INVOKABLE QString codexActiveAccountID() const { return "live-system"; }
    Q_INVOKABLE QVariantList codexAccounts() const { return {}; }
    Q_INVOKABLE QVariantMap codexAccountState() const { return {}; }
    Q_INVOKABLE QVariantList codexFetchAttempts() const { return {}; }
    Q_INVOKABLE QVariantList utilizationChartData(const QString&, const QString&) const { return {}; }
    Q_INVOKABLE QVariantMap costUsageData() const {
        ++costUsageDataCalls;
        return {};
    }
    Q_INVOKABLE QVariantList providerCostUsageList() const {
        ++providerCostUsageListCalls;
        return {};
    }
    Q_INVOKABLE QVariantMap providerCostUsageData(const QString&) const { return {}; }
    Q_INVOKABLE QVariantMap providerDashboardData(const QString&) const { return {}; }
    Q_INVOKABLE QVariantMap codexConsumerProjectionData() const { return {}; }
    Q_INVOKABLE void refresh() {}
    Q_INVOKABLE void refreshAll() {}
    Q_INVOKABLE void refreshCostUsage() { ++refreshCostUsageCalls; }
    Q_INVOKABLE void releaseCostUsageViewCaches() { ++releaseCostUsageViewCachesCalls; }
    Q_INVOKABLE void ensureCostUsageEnabled() {
        ++ensureCostUsageEnabledCalls;
        if (!costUsageEnabledValue) {
            costUsageEnabledValue = true;
            emit costUsageEnabledChanged();
        }
    }
    Q_INVOKABLE void refreshProvider(const QString& providerId) {
        ++refreshProviderCalls;
        lastRefreshProvider = providerId;
    }
    Q_INVOKABLE void setProviderEnabled(const QString&, bool) {}
    Q_INVOKABLE void setProviderSetting(const QString&, const QString& key, const QVariant& value) {
        ++setProviderSettingCalls;
        lastSettingKey = key;
        lastSettingValue = value;
    }
    Q_INVOKABLE bool setProviderSecret(const QString&, const QString& key, const QString& value) {
        ++setProviderSecretCalls;
        lastSecretKey = key;
        lastSecretValue = value;
        return true;
    }
    Q_INVOKABLE bool clearProviderSecret(const QString&, const QString&) { return false; }
    Q_INVOKABLE void testProviderConnection(const QString&) {}
    Q_INVOKABLE void startProviderLogin(const QString&) {}
    Q_INVOKABLE void cancelProviderLogin(const QString&) {}
    Q_INVOKABLE void refreshProviderStatuses() {}
    Q_INVOKABLE void moveProvider(int, int) {}
    Q_INVOKABLE void updateProviderIDs() {}

    void resetCounters() {
        providerListCalls = 0;
        providerDescriptorCalls = 0;
        costUsageDataCalls = 0;
        providerCostUsageListCalls = 0;
        refreshCostUsageCalls = 0;
        releaseCostUsageViewCachesCalls = 0;
        ensureCostUsageEnabledCalls = 0;
        setProviderSettingCalls = 0;
        setProviderSecretCalls = 0;
        tokenAccountsForProviderCalls = 0;
        addTokenAccountCalls = 0;
        addTokenAccountWithApiKeyCalls = 0;
        removeTokenAccountCalls = 0;
        setTokenAccountVisibilityCalls = 0;
        setTokenAccountSourceModeCalls = 0;
        setDefaultTokenAccountCalls = 0;
        refreshProviderCalls = 0;
        providerIDsForTest.clear();
        tokenAccountProviderForTest.clear();
        tokenAccountsForProviderValue.clear();
        defaultTokenAccountValue.clear();
        lastSettingKey.clear();
        lastSettingValue.clear();
        lastSecretKey.clear();
        lastSecretValue.clear();
        lastTokenProvider.clear();
        lastTokenDisplayName.clear();
        lastTokenAccountId.clear();
        lastTokenApiKey.clear();
        lastRefreshProvider.clear();
        lastTokenSourceMode = -1;
        lastTokenVisibility = -1;
    }

    void emitProviderStatusChangedForTest(const QString& providerId) {
        emit providerStatusChanged(providerId);
    }

    mutable int providerListCalls = 0;
    mutable int providerDescriptorCalls = 0;
    mutable int costUsageDataCalls = 0;
    mutable int providerCostUsageListCalls = 0;
    int refreshCostUsageCalls = 0;
    int releaseCostUsageViewCachesCalls = 0;
    int ensureCostUsageEnabledCalls = 0;
    int setProviderSettingCalls = 0;
    int setProviderSecretCalls = 0;
    mutable int tokenAccountsForProviderCalls = 0;
    int addTokenAccountCalls = 0;
    int addTokenAccountWithApiKeyCalls = 0;
    int removeTokenAccountCalls = 0;
    int setTokenAccountVisibilityCalls = 0;
    int setTokenAccountSourceModeCalls = 0;
    int setDefaultTokenAccountCalls = 0;
    int refreshProviderCalls = 0;
    bool costUsageEnabledValue = false;
    QStringList providerIDsForTest;
    QString tokenAccountProviderForTest;
    QVariantList tokenAccountsForProviderValue;
    QString defaultTokenAccountValue;
    QString lastSettingKey;
    QVariant lastSettingValue;
    QString lastSecretKey;
    QString lastSecretValue;
    QString lastTokenProvider;
    QString lastTokenDisplayName;
    QString lastTokenAccountId;
    QString lastTokenApiKey;
    QString lastRefreshProvider;
    int lastTokenSourceMode = -1;
    int lastTokenVisibility = -1;

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
    void providerSecretChanged(const QString&, const QString&);
    void tokenAccountsChanged(const QString&);
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

    QObject* findObjectByStringProperty(QObject* root, const char* propertyName, const QString& value) const {
        if (!root) return nullptr;
        QVariant propertyValue = root->property(propertyName);
        if (propertyValue.isValid() && propertyValue.toString() == value) {
            return root;
        }
        const auto children = root->children();
        for (QObject* child : children) {
            if (QObject* match = findObjectByStringProperty(child, propertyName, value)) {
                return match;
            }
        }
        if (auto* item = qobject_cast<QQuickItem*>(root)) {
            const auto childItems = item->childItems();
            for (QQuickItem* childItem : childItems) {
                if (QObject* match = findObjectByStringProperty(childItem, propertyName, value)) {
                    return match;
                }
            }
        }
        return nullptr;
    }

    QQuickItem* textInputByPlaceholder(QQuickItem* root, const QString& placeholder) const {
        return qobject_cast<QQuickItem*>(
            findObjectByStringProperty(root, "placeholderText", placeholder));
    }

    QQuickItem* createInlineRoot(QQuickView& view, const QByteArray& qml, const QUrl& url) {
        QQmlComponent component(view.engine());
        component.setData(qml, url);
        if (component.status() == QQmlComponent::Error) {
            qWarning() << component.errorString();
        }
        if (component.status() != QQmlComponent::Ready) {
            return nullptr;
        }
        QObject* object = component.create();
        auto* root = qobject_cast<QQuickItem*>(object);
        if (!root) {
            delete object;
            return nullptr;
        }
        view.setContent(url, &component, root);
        return root;
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

    void settingsWindowDefersProviderWorkUntilProvidersTab() {
        QQmlEngine engine;
        setupEngine(engine);
        mockUsage.resetCounters();

        QQmlComponent component(&engine, QUrl("qrc:/qml/SettingsWindow.qml"));
        QVERIFY2(component.status() == QQmlComponent::Ready,
                 qPrintable(component.errorString()));

        QQuickItem* root = qobject_cast<QQuickItem*>(component.create());
        QVERIFY(root != nullptr);
        QCoreApplication::processEvents();

        QCOMPARE(mockUsage.providerListCalls, 0);
        QCOMPARE(mockUsage.providerDescriptorCalls, 0);

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

    void trayPanelSwitchesTokenAccount() {
        QQuickView view;
        setupEngine(*view.engine());
        mockUsage.resetCounters();
        mockUsage.providerIDsForTest = {QStringLiteral("claude")};
        mockUsage.tokenAccountProviderForTest = QStringLiteral("claude");
        mockUsage.defaultTokenAccountValue = QStringLiteral("account-a");

        QVariantMap accountA;
        accountA["accountId"] = QStringLiteral("account-a");
        accountA["providerId"] = QStringLiteral("claude");
        accountA["displayName"] = QStringLiteral("Work");
        accountA["sourceMode"] = QStringLiteral("api");
        accountA["visibility"] = QStringLiteral("visible");
        QVariantMap accountB;
        accountB["accountId"] = QStringLiteral("account-b");
        accountB["providerId"] = QStringLiteral("claude");
        accountB["displayName"] = QStringLiteral("Personal");
        accountB["sourceMode"] = QStringLiteral("api");
        accountB["visibility"] = QStringLiteral("visible");
        mockUsage.tokenAccountsForProviderValue = {accountA, accountB};

        view.resize(300, 600);
        view.setSource(QUrl("qrc:/qml/TrayPanel.qml"));
        QVERIFY2(view.status() == QQuickView::Ready,
                 qPrintable(view.errors().isEmpty() ? QString() : view.errors().first().toString()));
        view.show();
        QTest::qWait(250);

        QObject* switcher = nullptr;
        QTRY_VERIFY((switcher = findObjectByStringProperty(view.rootObject(),
                                                           "objectName",
                                                           "accountSwitcher_claude")) != nullptr);
        QVERIFY(QMetaObject::invokeMethod(switcher, "valueActivated",
                                          Q_ARG(QVariant, QVariant(QStringLiteral("account-b")))));

        QTRY_COMPARE(mockUsage.setDefaultTokenAccountCalls, 1);
        QCOMPARE(mockUsage.lastTokenProvider, QStringLiteral("claude"));
        QCOMPARE(mockUsage.lastTokenAccountId, QStringLiteral("account-b"));
        QCOMPARE(mockUsage.refreshProviderCalls, 1);
        QCOMPARE(mockUsage.lastRefreshProvider, QStringLiteral("claude"));

        view.hide();
    }

    void usageWindowDefersTokenUsagePaneUntilShown() {
        QQuickView view;
        setupEngine(*view.engine());
        mockUsage.resetCounters();

        view.setSource(QUrl("qrc:/qml/UsageWindow.qml"));
        QVERIFY2(view.status() == QQuickView::Ready,
                 qPrintable(view.errors().isEmpty() ? QString() : view.errors().first().toString()));
        QCoreApplication::processEvents();

        QCOMPARE(mockUsage.costUsageDataCalls, 0);
        QCOMPARE(mockUsage.providerCostUsageListCalls, 0);
        QCOMPARE(mockUsage.providerListCalls, 0);
        QCOMPARE(mockUsage.ensureCostUsageEnabledCalls, 0);

        view.show();
        QTRY_VERIFY(mockUsage.costUsageDataCalls > 0);
        QTRY_VERIFY(mockUsage.providerCostUsageListCalls > 0);
        QTRY_VERIFY(mockUsage.providerListCalls > 0);
        QCOMPARE(mockUsage.ensureCostUsageEnabledCalls, 1);

        view.hide();
    }

    void tokenUsagePaneRequestsCostScanOnLoad() {
        QQuickView view;
        setupEngine(*view.engine());
        mockUsage.resetCounters();
        mockUsage.costUsageEnabledValue = false;
        view.resize(760, 560);

        QQuickItem* root = createInlineRoot(view, R"(
            import QtQuick 2.15
            import QtQuick.Controls 2.15
            import "qrc:/qml/panes" as Panes

            Panes.TokenUsagePane {
                width: 740
                height: 540
            }
        )", QUrl("qrc:/tests/TokenUsagePaneHarness.qml"));

        QVERIFY(root != nullptr);
        view.show();
        QTRY_COMPARE(mockUsage.ensureCostUsageEnabledCalls, 1);

        view.hide();
    }

    void usageWindowReleasesTokenUsageCachesWhenHidden() {
        QQuickView view;
        setupEngine(*view.engine());
        mockUsage.resetCounters();

        view.setSource(QUrl("qrc:/qml/UsageWindow.qml"));
        QVERIFY2(view.status() == QQuickView::Ready,
                 qPrintable(view.errors().isEmpty() ? QString() : view.errors().first().toString()));

        view.show();
        QTRY_VERIFY(mockUsage.costUsageDataCalls > 0);
        view.hide();

        QTRY_COMPARE(mockUsage.releaseCostUsageViewCachesCalls, 1);
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

    void secretInputCommitsOnlyOnExplicitAction() {
        QQuickView view;
        setupEngine(*view.engine());
        view.resize(420, 80);
        QQuickItem* root = createInlineRoot(view, R"(
            import QtQuick 2.15
            import QtQuick.Controls 2.15
            import "qrc:/qml/components" as Components

            Components.SecretInput {
                width: 400
                height: 44
                placeholder: "secret placeholder"
                property int saveCount: 0
                property string lastSecret: ""
                onSaveRequested: function(value) {
                    saveCount += 1
                    lastSecret = value
                }
            }
        )", QUrl("qrc:/tests/SecretInputHarness.qml"));

        QVERIFY(root != nullptr);
        view.show();
        QTest::qWait(150);

        QQuickItem* input = textInputByPlaceholder(root, "secret placeholder");
        QVERIFY(input != nullptr);
        input->forceActiveFocus();
        QTest::keyClick(&view, Qt::Key_A);
        QTest::keyClick(&view, Qt::Key_B);
        QTest::keyClick(&view, Qt::Key_C);
        QCoreApplication::processEvents();

        QCOMPARE(root->property("saveCount").toInt(), 0);

        QTest::keyClick(&view, Qt::Key_Return);
        QTRY_COMPARE(root->property("saveCount").toInt(), 1);
        QCOMPARE(root->property("lastSecret").toString(), QString("abc"));

        view.hide();
    }

    void providerTextSettingCommitsOnlyOnExplicitAction() {
        QQuickView view;
        setupEngine(*view.engine());
        view.resize(760, 560);
        QQuickItem* root = createInlineRoot(view, R"(
            import QtQuick 2.15
            import QtQuick.Controls 2.15
            import "qrc:/qml/components" as Components

            Components.ProviderDetailView {
                width: 740
                height: 540
                providerId: "zai"
                descriptor: ({
                    displayName: "z.ai",
                    enabled: true,
                    sessionLabel: "Session",
                    weeklyLabel: "Weekly",
                    sourceModes: ["api"],
                    settingsFields: [
                        {
                            key: "apiBaseUrl",
                            label: "API Base URL",
                            type: "text",
                            value: "",
                            placeholder: "base url"
                        }
                    ]
                })
                connectionTest: ({state: "idle"})
                providerStatus: ({state: "ok"})
                usageSnapshot: null
                property int settingCount: 0
                property string lastKey: ""
                property string lastValue: ""
                onSettingChanged: function(key, value) {
                    settingCount += 1
                    lastKey = key
                    lastValue = value
                }
            }
        )", QUrl("qrc:/tests/ProviderDetailHarness.qml"));

        QVERIFY(root != nullptr);
        view.show();
        QTest::qWait(250);

        QQuickItem* input = textInputByPlaceholder(root, "base url");
        QVERIFY(input != nullptr);
        input->forceActiveFocus();
        QTest::keyClick(&view, Qt::Key_A);
        QTest::keyClick(&view, Qt::Key_B);
        QTest::keyClick(&view, Qt::Key_C);
        QCoreApplication::processEvents();

        QCOMPARE(root->property("settingCount").toInt(), 0);

        QTest::keyClick(&view, Qt::Key_Return);
        QTRY_COMPARE(root->property("settingCount").toInt(), 1);
        QCOMPARE(root->property("lastKey").toString(), QString("apiBaseUrl"));
        QCOMPARE(root->property("lastValue").toString(), QString("abc"));

        view.hide();
    }

    void tokenAccountsPaneAddsApiAccount() {
        QQuickView view;
        setupEngine(*view.engine());
        mockUsage.resetCounters();
        view.resize(760, 360);

        QQuickItem* root = createInlineRoot(view, R"(
            import QtQuick 2.15
            import QtQuick.Controls 2.15
            import "qrc:/qml/components" as Components
            import CodexBar 1.0

            Components.TokenAccountsPane {
                width: 740
                height: 320
                providerId: "codebuff"
                descriptor: ({
                    sourceModes: ["api"],
                    tokenAccount: {
                        supportsMultipleAccounts: true,
                        requiredCredentialTypes: ["apiKey"]
                    }
                })
                accounts: []
                defaultAccountId: ""
                onAddAccount: function(displayName, sourceMode, apiKey) {
                    UsageStore.addTokenAccountWithApiKey(providerId, displayName, sourceMode, apiKey)
                }
            }
        )", QUrl("qrc:/tests/TokenAccountsPaneHarness.qml"));

        QVERIFY(root != nullptr);
        view.show();
        QTest::qWait(200);

        QQuickItem* nameInput = qobject_cast<QQuickItem*>(
            findObjectByStringProperty(root, "objectName", "accountNameField"));
        QVERIFY(nameInput != nullptr);
        QVERIFY(nameInput->setProperty("text", QStringLiteral("Production")));

        QQuickItem* keyInput = qobject_cast<QQuickItem*>(
            findObjectByStringProperty(root, "objectName", "accountApiKeyField"));
        QVERIFY(keyInput != nullptr);
        QVERIFY(keyInput->setProperty("text", QStringLiteral("cb-test-token")));
        QCoreApplication::processEvents();

        QQuickItem* addButton = qobject_cast<QQuickItem*>(
            findObjectByStringProperty(root, "objectName", "addAccountButton"));
        QVERIFY(addButton != nullptr);
        const QPoint clickPoint = addButton->mapToScene(
            QPointF(addButton->width() / 2.0, addButton->height() / 2.0)).toPoint();
        QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, clickPoint);

        QTRY_COMPARE(mockUsage.addTokenAccountWithApiKeyCalls, 1);
        QCOMPARE(mockUsage.lastTokenProvider, QString("codebuff"));
        QCOMPARE(mockUsage.lastTokenDisplayName, QString("Production"));
        QCOMPARE(mockUsage.lastTokenSourceMode, 4);
        QCOMPARE(mockUsage.lastTokenApiKey, QString("cb-test-token"));

        view.hide();
    }

    void settingsWindowDebouncesStatusProviderListRefresh() {
        QQuickView view;
        setupEngine(*view.engine());
        view.setSource(QUrl("qrc:/qml/SettingsWindow.qml"));
        view.show();
        QTest::qWait(400);

        mockUsage.resetCounters();
        mockUsage.emitProviderStatusChangedForTest("codex");
        mockUsage.emitProviderStatusChangedForTest("claude");
        mockUsage.emitProviderStatusChangedForTest("cursor");
        QTest::qWait(250);

        QVERIFY2(mockUsage.providerListCalls <= 1,
                 qPrintable(QString("providerList called %1 times").arg(mockUsage.providerListCalls)));

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
