#pragma once

#include <QString>
#include <optional>
#include <QObject>
#include <QDateTime>
#include <QHash>

enum class SessionQuotaTransition {
    None,
    Depleted,
    Restored,
};

struct SessionQuotaNotificationLogic {
    static constexpr double depletedThreshold = 0.0001;

    static bool isDepleted(double remainingPercent) {
        return remainingPercent <= depletedThreshold;
    }

    static SessionQuotaTransition transition(std::optional<double> previousRemaining, double currentRemaining) {
        bool wasDepleted = previousRemaining.has_value() ? isDepleted(*previousRemaining) : false;
        bool isNowDepleted = isDepleted(currentRemaining);

        if (!wasDepleted && isNowDepleted) return SessionQuotaTransition::Depleted;
        if (wasDepleted && !isNowDepleted) return SessionQuotaTransition::Restored;
        return SessionQuotaTransition::None;
    }
};

class SessionQuotaNotifier : public QObject {
    Q_OBJECT
public:
    static SessionQuotaNotifier& instance();
    static void post(SessionQuotaTransition transition, const QString& providerName);

signals:
    void notificationRequested(const QString& title, const QString& body);

private:
    SessionQuotaNotifier() = default;
    QHash<QString, QDateTime> m_lastNotify;
    static constexpr int MIN_NOTIFY_INTERVAL_SEC = 300;
};
