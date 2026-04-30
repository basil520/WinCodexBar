#include "SessionQuotaNotifications.h"

#include <QCoreApplication>

void SessionQuotaNotifier::post(SessionQuotaTransition transition, const QString& providerName) {
    QString title, body;

    switch (transition) {
    case SessionQuotaTransition::Depleted:
        title = QCoreApplication::translate("SessionQuotaNotifications", "%1 session depleted").arg(providerName);
        body = QCoreApplication::translate(
            "SessionQuotaNotifications",
            "0% left. Will notify when it's available again.");
        break;
    case SessionQuotaTransition::Restored:
        title = QCoreApplication::translate("SessionQuotaNotifications", "%1 session restored").arg(providerName);
        body = QCoreApplication::translate("SessionQuotaNotifications", "Session quota is available again.");
        break;
    default:
        return;
    }

    auto& notifier = instance();
    QDateTime now = QDateTime::currentDateTime();
    auto last = notifier.m_lastNotify.value(providerName);
    if (last.isValid() && last.secsTo(now) < MIN_NOTIFY_INTERVAL_SEC) return;

    notifier.m_lastNotify[providerName] = now;
    emit notifier.notificationRequested(title, body);
}

SessionQuotaNotifier& SessionQuotaNotifier::instance() {
    static SessionQuotaNotifier notifier;
    return notifier;
}
