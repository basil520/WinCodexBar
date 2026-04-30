#include "StatusItemController.h"
#include "TrayIconRenderer.h"
#include "TrayPopupPositioner.h"
#include "../app/LanguageManager.h"
#include "../app/UsageStore.h"
#include "../app/SettingsStore.h"

#include <QCoreApplication>
#include <QMenu>
#include <QMessageBox>
#include <QScreen>
#include <shellapi.h>

StatusItemController::StatusItemController(UsageStore* store,
                                             SettingsStore* settings,
                                             QObject* parent)
    : QObject(parent)
    , m_store(store)
    , m_settings(settings)
    , m_renderer(new TrayIconRenderer)
{
    connect(m_store, &UsageStore::snapshotChanged,
            this, &StatusItemController::onSnapshotChanged);
    connect(m_store, &UsageStore::providerIDsChanged,
            this, &StatusItemController::rebuildProviderMenu);
    connect(m_store, &UsageStore::providerStatusChanged,
            this, [this](const QString&) {
                if (m_settings->mergeIcons()) applyMergedIcon();
            });
    connect(m_settings, &SettingsStore::debugMenuEnabledChanged,
            this, [this]() { rebuildProviderMenu(); });
    connect(m_settings, &SettingsStore::mergeIconsChanged,
            this, [this]() { applyMergedIcon(); });
    connect(&LanguageManager::instance(), &LanguageManager::retranslate,
            this, [this]() {
                retranslateMenu();
                rebuildProviderMenu();
            });
}

StatusItemController::~StatusItemController() {
    destroyTrayIcon();
}

bool StatusItemController::createMessageWindow() {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &StatusItemController::messageWindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"CodexBarTrayMessageWindow";
    RegisterClassExW(&wc);

    m_hwndMsgWindow = CreateWindowExW(
        0, L"CodexBarTrayMessageWindow", L"", 0,
        0, 0, 0, 0, nullptr, nullptr, GetModuleHandleW(nullptr), this);
    return m_hwndMsgWindow != nullptr;
}

bool StatusItemController::createTrayIcon() {
    if (!m_hwndMsgWindow && !createMessageWindow()) return false;

    memset(&m_nid, 0, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hwndMsgWindow;
    m_nid.uID = TASKBAR_ICON_ID;
    m_nid.uFlags = NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    wcscpy_s(m_nid.szTip, L"CodexBar");

    if (!Shell_NotifyIconW(NIM_ADD, &m_nid)) {
        return false;
    }

    m_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    m_nid.uFlags = NIF_ICON;
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);

    m_contextMenu = new QMenu();
    m_refreshAction = m_contextMenu->addAction(QString());
    QObject::connect(m_refreshAction, &QAction::triggered, m_store, &UsageStore::refresh);

    m_providerMenu = m_contextMenu->addMenu(QString());
    m_providerSeparator = m_contextMenu->addSeparator();

    m_settingsAction = m_contextMenu->addAction(QString());
    QObject::connect(m_settingsAction, &QAction::triggered, this, &StatusItemController::settingsRequested);

    m_aboutAction = m_contextMenu->addAction(QString());
    QObject::connect(m_aboutAction, &QAction::triggered, this, &StatusItemController::aboutRequested);

    m_contextMenu->addSeparator();
    m_quitAction = m_contextMenu->addAction(QString());
    QObject::connect(m_quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    retranslateMenu();
    rebuildProviderMenu();

    return true;
}

void StatusItemController::destroyTrayIcon() {
    if (m_nid.hWnd) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        memset(&m_nid, 0, sizeof(m_nid));
    }
    if (m_hwndMsgWindow) {
        DestroyWindow(m_hwndMsgWindow);
        m_hwndMsgWindow = nullptr;
    }
    if (m_contextMenu) {
        delete m_contextMenu;
        m_contextMenu = nullptr;
    }
    m_providerMenu = nullptr;
    m_providerSeparator = nullptr;
    m_refreshAction = nullptr;
    m_settingsAction = nullptr;
    m_aboutAction = nullptr;
    m_quitAction = nullptr;
}

void StatusItemController::showBalloon(const QString& title, const QString& message) {
    if (!m_nid.hWnd) return;
    NOTIFYICONDATAW nid = m_nid;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    nid.uTimeout = 5000;
    auto tW = title.toStdWString();
    auto mW = message.toStdWString();
    wcscpy_s(nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle), tW.c_str());
    wcscpy_s(nid.szInfo, ARRAYSIZE(nid.szInfo), mW.c_str());
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

bool StatusItemController::initialize() {
    return createTrayIcon();
}

void StatusItemController::rebuildProviderMenu() {
    if (!m_providerMenu) return;
    m_providerMenu->clear();

    auto ids = m_store->allProviderIDs();
    if (ids.isEmpty()) {
        QAction* noneAction = m_providerMenu->addAction(tr("(no providers)"));
        noneAction->setEnabled(false);
        return;
    }

    for (const auto& id : ids) {
        bool enabled = m_store->isProviderEnabled(id);
        QString name = m_store->providerDisplayName(id);

        QAction* action = m_providerMenu->addAction(name);
        action->setCheckable(true);
        action->setChecked(id == m_currentProviderId);
        action->setEnabled(enabled);
        action->setData(id);

        QObject::connect(action, &QAction::triggered, this, [this, action]() {
            QString providerId = action->data().toString();
            m_currentProviderId = providerId;
            applyIcon(providerId);
            rebuildProviderMenu();
        });
    }
}

void StatusItemController::retranslateMenu() {
    if (m_refreshAction) m_refreshAction->setText(tr("Refresh"));
    if (m_providerMenu) m_providerMenu->menuAction()->setText(tr("Provider"));
    if (m_settingsAction) m_settingsAction->setText(tr("Settings"));
    if (m_aboutAction) m_aboutAction->setText(tr("About"));
    if (m_quitAction) m_quitAction->setText(tr("Quit"));
}

void StatusItemController::onProviderSelected() {
    auto* action = qobject_cast<QAction*>(sender());
    if (!action) return;
    QString providerId = action->data().toString();
    m_currentProviderId = providerId;
    applyIcon(providerId);
    rebuildProviderMenu();
}

void StatusItemController::applyIcon(const QString& providerId) {
    m_currentProviderId = providerId;
    auto snap = m_store->snapshot(providerId);
    double primary = snap.primary.has_value() ? snap.primary->remainingPercent() : 100.0;
    double weekly = snap.secondary.has_value() ? snap.secondary->remainingPercent() : 100.0;

    QIcon icon = m_renderer->makeIcon(primary, weekly, std::nullopt, false,
                                       TrayIconRenderer::IconStyle::Default);
    int iconSize = GetSystemMetrics(SM_CXSMICON);
    m_nid.hIcon = icon.pixmap(iconSize, iconSize).toImage().toHICON();
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

    QString tip = m_store->providerDisplayName(providerId);
    if (snap.primary.has_value()) {
        tip += QString(" %1%").arg(static_cast<int>(primary));
    }
    QVariantMap status = m_store->providerStatus(providerId);
    QString statusState = status.value("state", "unknown").toString();
    if (!statusState.isEmpty() && statusState != "unknown" && statusState != "ok") {
        tip += QString(" · %1").arg(statusState);
    }
    wcsncpy_s(m_nid.szTip, tip.toStdWString().c_str(), _TRUNCATE);

    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void StatusItemController::applyMergedIcon() {
    auto ids = m_store->providerIDs();
    if (!m_settings->mergeIcons()) {
        if (m_currentProviderId.isEmpty() && !ids.isEmpty()) {
            m_currentProviderId = ids.first();
        }
        applyIcon(m_currentProviderId);
        return;
    }

    std::optional<double> lowestPrimary;
    std::optional<double> lowestWeekly;
    QString tightestProvider;
    QString notableStatus;

    for (const auto& id : ids) {
        auto snap = m_store->snapshot(id);
        double primary = snap.primary.has_value() ? snap.primary->remainingPercent() : 100.0;
        double weekly = snap.secondary.has_value() ? snap.secondary->remainingPercent() : 100.0;
        if (!lowestPrimary.has_value() || primary < *lowestPrimary) {
            lowestPrimary = primary;
            tightestProvider = id;
        }
        if (!lowestWeekly.has_value() || weekly < *lowestWeekly) {
            lowestWeekly = weekly;
        }

        QString state = m_store->providerStatus(id).value("state", "unknown").toString();
        if (state == "outage") {
            notableStatus = state;
        } else if (state == "degraded" && notableStatus != "outage") {
            notableStatus = state;
        }
    }

    QIcon icon = m_renderer->makeIcon(lowestPrimary, lowestWeekly, std::nullopt, false,
                                      TrayIconRenderer::IconStyle::Default);
    int iconSize = GetSystemMetrics(SM_CXSMICON);
    m_nid.hIcon = icon.pixmap(iconSize, iconSize).toImage().toHICON();
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

    QString tip = tr("CodexBar");
    if (!tightestProvider.isEmpty() && lowestPrimary.has_value()) {
        tip += QString(" · %1 %2%")
            .arg(m_store->providerDisplayName(tightestProvider))
            .arg(static_cast<int>(*lowestPrimary));
    }
    if (!notableStatus.isEmpty()) {
        tip += QString(" · %1").arg(notableStatus);
    }
    wcsncpy_s(m_nid.szTip, tip.toStdWString().c_str(), _TRUNCATE);

    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

QRect StatusItemController::trayIconRect() const {
    return TrayPopupPositioner::getTrayIconRect(m_hwndMsgWindow, TASKBAR_ICON_ID);
}

void StatusItemController::showTrayPanel() {
    if (m_trayPanel) {
        m_trayPanel->show();
        m_trayPanel->raise();
    }
}

void StatusItemController::hideTrayPanel() {
    if (m_trayPanel) {
        m_trayPanel->hide();
    }
}

void StatusItemController::toggleTrayPanel() {
    if (m_trayPanel && m_trayPanel->isVisible()) {
        hideTrayPanel();
    } else {
        showTrayPanel();
    }
}

void StatusItemController::onSnapshotChanged(const QString& providerId) {
    if (providerId == m_currentProviderId || m_settings->mergeIcons()) {
        applyMergedIcon();
    }
}

LRESULT CALLBACK StatusItemController::messageWindowProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        StatusItemController* self = static_cast<StatusItemController*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        return 0;
    }

    if (msg == WM_TRAYICON) {
        StatusItemController* self =
            reinterpret_cast<StatusItemController*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (self) {
            switch (LOWORD(lParam)) {
            case WM_LBUTTONUP:
            case NIN_SELECT:
                emit self->trayPanelRequested();
                break;
            case NIN_BALLOONUSERCLICK:
                emit self->trayPanelRequested();
                break;
            case WM_RBUTTONUP:
            case WM_CONTEXTMENU:
                if (self->m_contextMenu) {
                    self->m_contextMenu->popup(QCursor::pos());
                }
                break;
            }
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
