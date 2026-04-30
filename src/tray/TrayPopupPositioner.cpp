#include "TrayPopupPositioner.h"

#include <QScreen>
#include <QGuiApplication>

QRect TrayPopupPositioner::getTrayIconRect(HWND hwnd, UINT iconID) {
    NOTIFYICONIDENTIFIER nii = {};
    nii.cbSize = sizeof(nii);
    nii.hWnd = hwnd;
    nii.uID = iconID;
    nii.guidItem = GUID_NULL;

    RECT rect;
    HRESULT hr = Shell_NotifyIconGetRect(&nii, &rect);
    if (SUCCEEDED(hr)) {
        return QRect(rect.left, rect.top,
                     rect.right - rect.left,
                     rect.bottom - rect.top);
    }
    return estimateTrayIconRect();
}

static QScreen* screenForTaskbar() {
    APPBARDATA abd = {};
    abd.cbSize = sizeof(abd);
    abd.hWnd = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (abd.hWnd) {
        SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
        QPoint center((abd.rc.left + abd.rc.right) / 2, (abd.rc.top + abd.rc.bottom) / 2);
        QScreen* s = QGuiApplication::screenAt(center);
        if (s) return s;
    }
    return QGuiApplication::primaryScreen();
}

QRect TrayPopupPositioner::estimateTrayIconRect() {
    QScreen* screen = screenForTaskbar();
    if (!screen) return QRect(0, 0, 24, 24);

    QRect avail = screen->availableGeometry();
    QRect geo = screen->geometry();

    DWORD pos = ABE_BOTTOM;
    APPBARDATA abd = {};
    abd.cbSize = sizeof(abd);
    abd.hWnd = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (abd.hWnd) {
        SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
        pos = abd.uEdge;
    }

    int iconSize = 24;
    int margin = 4;

    switch (pos) {
    case ABE_TOP:
        return QRect(avail.right() - 40, geo.top() + margin, iconSize, iconSize);
    case ABE_BOTTOM:
        return QRect(avail.right() - 40, avail.bottom() + margin, iconSize, iconSize);
    case ABE_LEFT:
        return QRect(geo.left() + margin, avail.bottom() - 40, iconSize, iconSize);
    case ABE_RIGHT:
        return QRect(avail.right() + margin, avail.bottom() - 40, iconSize, iconSize);
    }
    return QRect(avail.right() - 40, avail.bottom() + margin, iconSize, iconSize);
}

QPoint TrayPopupPositioner::getTaskbarEdge() {
    QScreen* screen = screenForTaskbar();
    if (!screen) return QPoint(0, 0);
    return screen->geometry().topLeft();
}

DWORD TrayPopupPositioner::getTaskbarPosition() {
    APPBARDATA abd = {};
    abd.cbSize = sizeof(abd);
    abd.hWnd = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (abd.hWnd) {
        UINT state = (UINT)SHAppBarMessage(ABM_GETSTATE, &abd);
        if (state & ABS_AUTOHIDE) {
            SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
            return abd.uEdge;
        }
    }
    return ABE_BOTTOM;
}
