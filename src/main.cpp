#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QDesktopServices>
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QQuickItem>
#include <QQuickView>
#include <QScreen>
#include <QSettings>
#include <QSharedMemory>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>
#include <QtQml>
#include <algorithm>
#include <functional>

#ifdef Q_OS_WIN
#include <cmath>
#include <windows.h>
#endif

static QString g_logPath;
static QtMessageHandler g_previousMessageHandler = nullptr;

static void fileMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    if (!g_logPath.isEmpty()) {
        QFile file(g_logPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            file.write(qFormatLogMessage(type, context, message).toUtf8());
            file.write("\n");
        }
    }
    if (g_previousMessageHandler) {
        g_previousMessageHandler(type, context, message);
    }
}

#include "app/SettingsStore.h"
#include "app/UsageStore.h"
#include "app/LanguageManager.h"
#include "app/SessionQuotaNotifications.h"
#include "tray/StatusItemController.h"
#include "providers/ProviderRegistry.h"
#include "providers/zai/ZaiProvider.h"
#include "providers/openrouter/OpenRouterProvider.h"
#include "providers/copilot/CopilotProvider.h"
#include "providers/kimik2/KimiK2Provider.h"
#include "providers/kilo/KiloProvider.h"
#include "providers/kiro/KiroProvider.h"
#include "providers/mistral/MistralProvider.h"
#include "providers/ollama/OllamaProvider.h"
#include "providers/codex/CodexProvider.h"
#include "providers/claude/ClaudeProvider.h"
#include "providers/cursor/CursorProvider.h"
#include "providers/kimi/KimiProvider.h"
#include "providers/opencode/OpenCodeProvider.h"
#include "providers/opencode/OpenCodeGoProvider.h"
#include "providers/deepseek/DeepSeekProvider.h"

#ifdef Q_OS_WIN
static void applyRoundedWindowRegion(QWindow* window, int radius) {
    if (!window) return;

    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd) return;

    const qreal scale = window->devicePixelRatio();
    const int width = std::max(1, static_cast<int>(std::lround(window->width() * scale)));
    const int height = std::max(1, static_cast<int>(std::lround(window->height() * scale)));
    const int diameter = std::max(1, static_cast<int>(std::lround(radius * 2 * scale)));

    HRGN region = CreateRoundRectRgn(0, 0, width + 1, height + 1, diameter, diameter);
    if (!region) return;

    if (SetWindowRgn(hwnd, region, TRUE) == FALSE) {
        DeleteObject(region);
    }
}
#else
static void applyRoundedWindowRegion(QWindow* window, int radius) {
    Q_UNUSED(window);
    Q_UNUSED(radius);
}
#endif

class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool settingsVisible READ isSettingsVisible NOTIFY settingsVisibleChanged)
    Q_PROPERTY(bool settingsMaximized READ isSettingsMaximized NOTIFY settingsMaximizedChanged)
public:
    explicit AppController(QObject* parent = nullptr) : QObject(parent) {}

    QQuickView* settingsView = nullptr;
    QQuickView* trayView = nullptr;

    Q_INVOKABLE void openSettings() {
        if (!settingsView) return;
        settingsView->show();
        settingsView->raise();
        settingsView->requestActivate();
        emit settingsVisibleChanged();
        emit settingsMaximizedChanged();
    }

    Q_INVOKABLE void startSettingsMove() {
        if (!settingsView) return;
        settingsView->startSystemMove();
    }

    Q_INVOKABLE void startSettingsResize(int edges) {
        if (!settingsView) return;
        settingsView->startSystemResize(Qt::Edges(edges));
    }

    Q_INVOKABLE void minimizeSettings() {
        if (!settingsView) return;
        settingsView->showMinimized();
        emit settingsVisibleChanged();
    }

    Q_INVOKABLE void toggleSettingsMaximized() {
        if (!settingsView) return;
        if (isSettingsMaximized()) {
            settingsView->showNormal();
        } else {
            settingsView->showMaximized();
        }
        emit settingsMaximizedChanged();
        emit settingsVisibleChanged();
    }

    Q_INVOKABLE void closeSettings() {
        if (!settingsView) return;
        settingsView->hide();
        emit settingsVisibleChanged();
    }

    Q_INVOKABLE void moveTrayPanel(int deltaX, int deltaY) {
        if (!trayView) return;
        QPoint pos = trayView->position();
        trayView->setPosition(pos.x() + deltaX, pos.y() + deltaY);
    }

    Q_INVOKABLE void toggleSettings() {
        if (!settingsView) return;
        if (settingsView->isVisible()) {
            settingsView->hide();
        } else {
            openSettings();
        }
        emit settingsVisibleChanged();
    }

    Q_INVOKABLE void quitApp() {
        QCoreApplication::quit();
    }

    Q_INVOKABLE void openExternalUrl(const QString& url) {
        QDesktopServices::openUrl(QUrl(url));
    }

    Q_INVOKABLE void copyText(const QString& text) {
        if (auto* clipboard = QGuiApplication::clipboard()) {
            clipboard->setText(text);
        }
    }

    bool isSettingsVisible() const {
        return settingsView && settingsView->isVisible();
    }

    bool isSettingsMaximized() const {
        return settingsView && (settingsView->windowState() & Qt::WindowMaximized);
    }

signals:
    void settingsVisibleChanged();
    void settingsMaximizedChanged();
};

static QVariantMap makeAppTheme() {
    QVariantMap theme;
    theme["bgPrimary"] = QColor("#1a1a2e");
    theme["bgSecondary"] = QColor("#15152a");
    theme["bgCard"] = QColor("#1f1f38");
    theme["bgHover"] = QColor("#2a2a4a");
    theme["bgSelected"] = QColor("#3a3a5c");
    theme["bgPressed"] = QColor("#4a4a7c");
    theme["borderColor"] = QColor("#2a2a4a");
    theme["borderAccent"] = QColor("#4a4a7c");
    theme["textPrimary"] = QColor("#ffffff");
    theme["textSecondary"] = QColor("#aaaaaa");
    theme["textTertiary"] = QColor("#888888");
    theme["textDisabled"] = QColor("#555555");
    theme["statusOk"] = QColor("#4CAF50");
    theme["statusDegraded"] = QColor("#FFC107");
    theme["statusOutage"] = QColor("#F44336");
    theme["statusUnknown"] = QColor("#888888");
    theme["accentColor"] = QColor("#6b6bff");
    theme["accentHover"] = QColor("#8a8aff");
    theme["spacingXs"] = 4;
    theme["spacingSm"] = 8;
    theme["spacingMd"] = 12;
    theme["spacingLg"] = 16;
    theme["spacingXl"] = 24;
    theme["radiusSm"] = 4;
    theme["radiusMd"] = 8;
    theme["radiusLg"] = 12;
    theme["fontSizeSm"] = 11;
    theme["fontSizeMd"] = 13;
    theme["fontSizeLg"] = 16;
    theme["fontSizeXl"] = 20;
    theme["sidebarWidth"] = 240;
    theme["listItemHeight"] = 48;
    theme["iconSizeSm"] = 18;
    theme["iconSizeMd"] = 24;
    theme["iconSizeLg"] = 28;
    theme["statusDotSize"] = 6;
    theme["progressBarHeight"] = 6;
    return theme;
}

static void dumpQuickItemTree(QQuickItem* item, int depth = 0) {
    if (!item) {
        qWarning() << "Settings item tree: <null>";
        return;
    }
    const QString indent(depth * 2, QLatin1Char(' '));
    qWarning().noquote() << indent
                         << item->metaObject()->className()
                         << "objectName=" << item->objectName()
                         << "visible=" << item->isVisible()
                         << "opacity=" << item->opacity()
                         << "x=" << item->x()
                         << "y=" << item->y()
                         << "w=" << item->width()
                         << "h=" << item->height()
                         << "children=" << item->childItems().size();
    if (depth >= 4) return;
    for (auto* child : item->childItems()) {
        dumpQuickItemTree(child, depth + 1);
    }
}

int main(int argc, char* argv[]) {
    qputenv("QT_QUICK_CONTROLS_STYLE", QByteArray("Basic"));

    QSharedMemory singleInstance("WinCodexBar_SingleInstance");
    if (!singleInstance.create(1)) {
        return 0; // Another instance is already running
    }

    QApplication app(argc, argv);
    app.setApplicationName("WinCodexBar");
    app.setOrganizationName("CodexBar");
    app.setQuitOnLastWindowClosed(false);
    const QStringList appArgs = QCoreApplication::arguments();
    for (const auto& arg : appArgs) {
        if (arg.startsWith("--qml-log=")) {
            g_logPath = arg.mid(QStringLiteral("--qml-log=").size());
            g_previousMessageHandler = qInstallMessageHandler(fileMessageHandler);
        }
    }
    const bool showSettingsOnStartup = appArgs.contains("--show-settings");

    ProviderRegistry::instance().registerProvider(new ZaiProvider());
    ProviderRegistry::instance().registerProvider(new OpenRouterProvider());
    ProviderRegistry::instance().registerProvider(new CopilotProvider());
    ProviderRegistry::instance().registerProvider(new KimiK2Provider());
    ProviderRegistry::instance().registerProvider(new KiloProvider());
    ProviderRegistry::instance().registerProvider(new KiroProvider());
    ProviderRegistry::instance().registerProvider(new MistralProvider());
    ProviderRegistry::instance().registerProvider(new OllamaProvider());
    ProviderRegistry::instance().registerProvider(new CodexProvider());
    ProviderRegistry::instance().registerProvider(new ClaudeProvider());
    ProviderRegistry::instance().registerProvider(new CursorProvider());
    ProviderRegistry::instance().registerProvider(new KimiProvider());
    ProviderRegistry::instance().registerProvider(new OpenCodeProvider());
    ProviderRegistry::instance().registerProvider(new OpenCodeGoProvider());
    ProviderRegistry::instance().registerProvider(new DeepSeekProvider());

    SettingsStore* settings = new SettingsStore();
    UsageStore* usageStore = new UsageStore();
    usageStore->setSettingsStore(settings);

    qmlRegisterSingletonInstance("CodexBar", 1, 0, "SettingsStore", settings);
    qmlRegisterSingletonInstance("CodexBar", 1, 0, "UsageStore", usageStore);

    AppController* appController = new AppController(&app);
    qmlRegisterSingletonInstance("CodexBar", 1, 0, "AppController", appController);

    LanguageManager& langMgr = LanguageManager::instance();
    qmlRegisterSingletonInstance("CodexBar", 1, 0, "LanguageManager", &langMgr);
    langMgr.setLanguage(settings->language());
    QObject::connect(settings, &SettingsStore::languageChanged, [&]() {
        langMgr.setLanguage(settings->language());
    });

    auto allIds = usageStore->allProviderIDs();
    QSettings reg("HKEY_CURRENT_USER\\Software\\CodexBar", QSettings::NativeFormat);
    for (const auto& id : allIds) {
        QString key = "providers/" + id + "/enabled";
        bool enabled;
        if (reg.contains(key)) {
            enabled = reg.value(key).toBool();
        } else {
            auto* prov = ProviderRegistry::instance().provider(id);
            enabled = prov ? prov->defaultEnabled() : false;
        }
        usageStore->setProviderEnabled(id, enabled);
        settings->setProviderEnabled(id, enabled);
    }

    StatusItemController trayCtrl(usageStore, settings);
    if (!trayCtrl.initialize()) return 1;

    usageStore->setCostUsageEnabled(true);

    if (settings->refreshFrequency() > 0) {
        usageStore->startAutoRefresh(settings->refreshFrequency());
    }
    QTimer::singleShot(0, usageStore, &UsageStore::refreshProviderStatuses);
    QObject::connect(settings, &SettingsStore::refreshFrequencyChanged, usageStore, [settings, usageStore]() {
        if (settings->refreshFrequency() > 0) {
            usageStore->startAutoRefresh(settings->refreshFrequency());
        } else {
            usageStore->stopAutoRefresh();
        }
    });

    QQmlEngine qmlEngine;
    langMgr.install(&qmlEngine);
    qmlEngine.rootContext()->setContextProperty("AppTheme", makeAppTheme());

    QQuickView trayView(&qmlEngine, nullptr);
    trayView.setTitle("CodexBar");
    trayView.resize(300, 520);
    trayView.setFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint);
    trayView.setColor(QColor("#1a1a2e"));
    applyRoundedWindowRegion(&trayView, 12);

    appController->trayView = &trayView;

    trayView.setSource(QUrl("qrc:/qml/TrayPanel.qml"));

    auto positionPanel = [&]() {
        QRect rect = trayCtrl.trayIconRect();
        QPoint iconCenter(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
        QScreen* screen = QGuiApplication::screenAt(iconCenter);
        if (!screen) screen = QGuiApplication::primaryScreen();
        if (!screen) return;
        QRect avail = screen->availableGeometry();
        int pw = trayView.width();
        int ph = trayView.height();
        if (pw <= 0) pw = 300;
        if (ph <= 0) ph = 420;
        int x = rect.x() + rect.width() / 2 - pw / 2;
        int y = avail.bottom() - ph - 4;
        x = qBound(avail.left(), x, avail.right() - pw);
        y = qMax(avail.top(), y);
        trayView.setPosition(x, y);
    };

    auto showPanel = [&]() {
        if (trayView.status() != QQuickView::Ready) return;
        positionPanel();
        trayView.show();
        applyRoundedWindowRegion(&trayView, 12);
        trayView.raise();
        trayView.requestActivate();
    };

    bool startupPanelShown = false;
    auto showStartupPanel = [&]() {
        if (startupPanelShown || trayView.status() != QQuickView::Ready) return;
        startupPanelShown = true;
        showPanel();
    };

    QObject::connect(&trayView, &QQuickView::statusChanged, &trayView, [&](QQuickView::Status status) {
        if (status == QQuickView::Error) {
            QStringList messages;
            for (const QQmlError& error : trayView.errors()) {
                messages.append(error.toString());
            }
            const QString detail = messages.isEmpty()
                ? QCoreApplication::translate("App", "Unknown QML loading error.")
                : messages.join(QLatin1Char('\n'));
            qWarning() << "Failed to load tray panel:" << detail;
            QMessageBox::critical(nullptr, QStringLiteral("WinCodexBar"), detail);
        } else if (status == QQuickView::Ready) {
            showStartupPanel();
        }
    });

    QObject::connect(&trayCtrl, &StatusItemController::trayPanelRequested, [&]() {
        if (trayView.isVisible()) {
            trayView.hide();
            return;
        }
        showPanel();
    });

    QObject::connect(&trayView, &QQuickView::activeChanged, [&]() {
        if (!trayView.isActive() && trayView.isVisible()) {
            trayView.hide();
        }
    });
    QObject::connect(&trayView, &QWindow::visibleChanged, &trayView, [&trayView](bool visible) {
        if (visible) applyRoundedWindowRegion(&trayView, 12);
    });

    QTimer::singleShot(0, &trayView, showStartupPanel);

    QQuickView settingsView(&qmlEngine, nullptr);
    auto updateSettingsTitle = [&settingsView]() {
        settingsView.setTitle(QCoreApplication::translate("App", "CodexBar Settings"));
    };
    updateSettingsTitle();
    QObject::connect(&langMgr, &LanguageManager::retranslate, &settingsView, updateSettingsTitle);
    settingsView.resize(900, 640);
    settingsView.setMinimumSize(QSize(820, 560));
    settingsView.setResizeMode(QQuickView::SizeRootObjectToView);
    settingsView.setFlags(Qt::Window | Qt::FramelessWindowHint);
    settingsView.setColor(QColor("#1a1a2e"));

    QObject::connect(&settingsView, &QQuickView::statusChanged, &settingsView, [&](QQuickView::Status status) {
        if (status != QQuickView::Error) return;
        QStringList messages;
        for (const QQmlError& error : settingsView.errors()) {
            messages.append(error.toString());
        }
        const QString detail = messages.isEmpty()
            ? QCoreApplication::translate("App", "Unknown QML loading error.")
            : messages.join(QLatin1Char('\n'));
        qWarning() << "Failed to load settings window:" << detail;
    });
    settingsView.setSource(QUrl("qrc:/qml/SettingsWindow.qml"));

    appController->settingsView = &settingsView;
    QObject::connect(&settingsView, &QWindow::windowStateChanged, appController,
                     [appController]() {
                         emit appController->settingsMaximizedChanged();
                     });
    if (showSettingsOnStartup) {
        QTimer::singleShot(0, appController, &AppController::openSettings);
        if (!g_logPath.isEmpty()) {
            QTimer::singleShot(1000, &settingsView, [&settingsView]() {
                qWarning() << "Settings view status=" << settingsView.status()
                           << "size=" << settingsView.size()
                           << "root=" << settingsView.rootObject();
                dumpQuickItemTree(settingsView.rootObject());
            });
        }
    }

    QObject::connect(&trayCtrl, &StatusItemController::settingsRequested, appController,
                     &AppController::toggleSettings);

    QObject::connect(&trayCtrl, &StatusItemController::aboutRequested, &app, [&app]() {
        const QString body = QString("%1\n\n%2\n\n%3\n%4")
            .arg(QCoreApplication::translate("App", "WinCodexBar v0.1.0"),
                 QCoreApplication::translate(
                     "App",
                     "Windows system tray app for tracking AI provider usage limits."),
                 QCoreApplication::translate("App", "Built with Qt 6.5 + QML"),
                 QStringLiteral("github.com/anomalyco/CodexBar"));
        QMessageBox::about(nullptr,
            QCoreApplication::translate("App", "About WinCodexBar"),
            body);
    });

    QObject::connect(&SessionQuotaNotifier::instance(), &SessionQuotaNotifier::notificationRequested,
                     &trayCtrl, [&trayCtrl](const QString& title, const QString& body) {
        trayCtrl.showBalloon(title, body);
    });

    

    return app.exec();
}

#include "main.moc"
