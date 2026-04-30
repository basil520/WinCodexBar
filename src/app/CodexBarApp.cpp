#include "CodexBarApp.h"

CodexBarApp::CodexBarApp(QObject* parent)
    : QObject(parent) {}

bool CodexBarApp::initialize() {
    if (!setupTrayIcon()) return false;
    setupAutoRefresh();
    return true;
}

void CodexBarApp::quit() {
    // Cleanup
}

bool CodexBarApp::setupTrayIcon() {
    // Will be implemented in StatusItemController
    return true;
}

bool CodexBarApp::setupAutoRefresh() {
    return true;
}
