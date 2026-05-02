#pragma once

#include <QString>
#include <QDateTime>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <QHash>

struct DashboardCreditEvent {
    QDateTime date;
    QString service;
    double amount;
    QString currency;

    QJsonObject toJson() const {
        QJsonObject o;
        o["date"] = date.toString(Qt::ISODate);
        o["service"] = service;
        o["amount"] = amount;
        o["currency"] = currency;
        return o;
    }
};

struct DashboardServiceUsage {
    QString service;
    double tokens;
    double costUSD;

    QJsonObject toJson() const {
        QJsonObject o;
        o["service"] = service;
        o["tokens"] = tokens;
        o["costUSD"] = costUSD;
        return o;
    }
};

struct DashboardData {
    QDateTime fetchedAt;
    QVector<DashboardCreditEvent> creditEvents;
    QVector<DashboardServiceUsage> usageByService;
    QString purchaseURL;

    QJsonObject toJson() const {
        QJsonObject o;
        o["fetchedAt"] = fetchedAt.toString(Qt::ISODate);
        QJsonArray events;
        for (const auto& e : creditEvents) events.append(e.toJson());
        o["creditEvents"] = events;
        QJsonArray usage;
        for (const auto& u : usageByService) usage.append(u.toJson());
        o["usageByService"] = usage;
        o["purchaseURL"] = purchaseURL;
        return o;
    }
};

class OpenAIDashboardFetcher {
public:
    struct Result {
        bool success = false;
        QString errorMessage;
        DashboardData data;
    };

    static Result fetch(const QString& accessToken, const QString& accountId, int timeoutMs = 15000);
    static Result fetchViaWeb(const QString& cookieHeader, const QString& accountId, int timeoutMs = 15000);

private:
    static Result parseDashboardHTML(const QString& html);
    static QString buildDashboardURL(const QString& accountId);
};
