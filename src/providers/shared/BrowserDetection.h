#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

#include "CookieImporter.h"

class BrowserDetection {
public:
    static QVector<CookieImporter::Browser> installedBrowsers();
    static QString cookiePath(CookieImporter::Browser browser);
    static QString localStatePath(CookieImporter::Browser browser);
    static QStringList profilePaths(CookieImporter::Browser browser);
};
