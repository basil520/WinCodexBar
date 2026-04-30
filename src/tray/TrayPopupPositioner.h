#pragma once

#include <QRect>
#include <windows.h>
#include <shellapi.h>

class TrayPopupPositioner {
public:
    static QRect getTrayIconRect(HWND hwnd, UINT iconID);
    static QRect estimateTrayIconRect();

private:
    static QPoint getTaskbarEdge();
    static DWORD getTaskbarPosition();
};
