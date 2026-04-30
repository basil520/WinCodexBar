#pragma once

#include <QObject>

class CodexBarApp : public QObject {
    Q_OBJECT
public:
    explicit CodexBarApp(QObject* parent = nullptr);

    bool initialize();
    void quit();

private:
    bool setupTrayIcon();
    bool setupAutoRefresh();
};
