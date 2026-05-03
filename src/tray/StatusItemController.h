#pragma once

#include <QObject>
#include <QAction>
#include <QMenu>
#include <QQuickWindow>
#include <QIcon>
#include <windows.h>

class UsageStore;
class SettingsStore;
class TrayIconRenderer;

class StatusItemController : public QObject {
    Q_OBJECT
public:
    explicit StatusItemController(UsageStore* store,
                                   SettingsStore* settings,
                                   QObject* parent = nullptr);
    ~StatusItemController() override;

    bool initialize();
    void destroyTrayIcon();
    void applyIcon(const QString& providerId);
    void applyMergedIcon();
    QRect trayIconRect() const;

signals:
    void trayPanelRequested();
    void settingsRequested();
    void aboutRequested();
    void quitRequested();

public slots:
    void showTrayPanel();
    void hideTrayPanel();
    void toggleTrayPanel();
    void showBalloon(const QString& title, const QString& message);

private slots:
    void onSnapshotChanged(const QString& providerId);
    void rebuildProviderMenu();
    void onProviderSelected();

private:
    bool createTrayIcon();
    bool createMessageWindow();
    void retranslateMenu();
    static LRESULT CALLBACK messageWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    NOTIFYICONDATAW m_nid = {};
    HWND m_hwndMsgWindow = nullptr;
    QMenu* m_contextMenu = nullptr;
    QMenu* m_providerMenu = nullptr;
    QAction* m_providerSeparator = nullptr;
    QAction* m_refreshAction = nullptr;
    QAction* m_settingsAction = nullptr;
    QAction* m_aboutAction = nullptr;
    QAction* m_quitAction = nullptr;
    QQuickWindow* m_trayPanel = nullptr;
    TrayIconRenderer* m_renderer = nullptr;
    UsageStore* m_store = nullptr;
    SettingsStore* m_settings = nullptr;
    QString m_currentProviderId;

    static constexpr UINT WM_TRAYICON = WM_APP + 1;
    static constexpr UINT TASKBAR_ICON_ID = 1;
};
