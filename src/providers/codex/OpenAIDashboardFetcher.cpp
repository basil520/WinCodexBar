#include "OpenAIDashboardFetcher.h"
#include "../../network/NetworkManager.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QDebug>
#include <QUrl>

QString OpenAIDashboardFetcher::buildDashboardURL(const QString& accountId)
{
    QString base = "https://chatgpt.com/backend-api/dashboard";
    if (!accountId.isEmpty()) {
        base += "?account_id=" + accountId;
    }
    return base;
}

OpenAIDashboardFetcher::Result OpenAIDashboardFetcher::fetch(
    const QString& accessToken, const QString& accountId, int timeoutMs)
{
    Result result;
    QString url = buildDashboardURL(accountId);
    QHash<QString, QString> headers;
    headers["Authorization"] = "Bearer " + accessToken;
    headers["Accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8";
    headers["Accept-Language"] = "en-US,en;q=0.9";

    QString html = NetworkManager::instance().getStringSync(QUrl(url), headers, timeoutMs);
    if (html.isEmpty()) {
        result.errorMessage = "Dashboard request returned empty response";
        return result;
    }

    return parseDashboardHTML(html);
}

OpenAIDashboardFetcher::Result OpenAIDashboardFetcher::fetchViaWeb(
    const QString& cookieHeader, const QString& accountId, int timeoutMs)
{
    Result result;
    QString url = buildDashboardURL(accountId);
    QHash<QString, QString> headers;
    headers["Cookie"] = cookieHeader;
    headers["Accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8";
    headers["Accept-Language"] = "en-US,en;q=0.9";

    QString html = NetworkManager::instance().getStringSync(QUrl(url), headers, timeoutMs);
    if (html.isEmpty()) {
        result.errorMessage = "Dashboard request returned empty response";
        return result;
    }

    return parseDashboardHTML(html);
}

OpenAIDashboardFetcher::Result OpenAIDashboardFetcher::parseDashboardHTML(const QString& html)
{
    Result result;
    result.data.fetchedAt = QDateTime::currentDateTime();

    // Strip HTML tags to get text content
    QString text = html;
    static QRegularExpression tagRe("<[^>]+>");
    text = text.replace(tagRe, " ");
    static QRegularExpression spaceRe("\\s+");
    text = text.replace(spaceRe, " ").trimmed();

    // Try to find JSON embedded in script tags (common pattern for SPA data hydration)
    static QRegularExpression scriptRe("<script[^>]*>(.*?)</script>", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator it = scriptRe.globalMatch(html);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString script = match.captured(1);
        if (script.contains("credit_events") || script.contains("usage_breakdown")) {
            // Try to extract JSON from script
            static QRegularExpression jsonRe("(\\{[^}]*credit_events[^}]*\\})");
            QRegularExpressionMatch jsonMatch = jsonRe.match(script);
            if (jsonMatch.hasMatch()) {
                QString jsonStr = jsonMatch.captured(1);
                QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
                if (!doc.isNull() && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    QJsonArray events = obj.value("credit_events").toArray();
                    for (const auto& v : events) {
                        QJsonObject e = v.toObject();
                        DashboardCreditEvent ce;
                        ce.date = QDateTime::fromString(e.value("date").toString(), Qt::ISODate);
                        ce.service = e.value("service").toString();
                        ce.amount = e.value("amount").toDouble();
                        ce.currency = e.value("currency").toString("USD");
                        result.data.creditEvents.append(ce);
                    }
                    QJsonArray usage = obj.value("usage_breakdown").toArray();
                    for (const auto& v : usage) {
                        QJsonObject u = v.toObject();
                        DashboardServiceUsage su;
                        su.service = u.value("service").toString();
                        su.tokens = u.value("tokens").toDouble();
                        su.costUSD = u.value("cost_usd").toDouble();
                        result.data.usageByService.append(su);
                    }
                }
            }
        }
    }

    // Fallback: regex-based parsing from text content
    if (result.data.creditEvents.isEmpty()) {
        static QRegularExpression creditRe("Credit\\s*[:\\-]?\\s*([0-9.,]+)");
        QRegularExpressionMatch creditMatch = creditRe.match(text);
        if (creditMatch.hasMatch()) {
            DashboardCreditEvent ce;
            ce.date = QDateTime::currentDateTime();
            ce.service = "dashboard";
            QString amt = creditMatch.captured(1);
            amt.remove(',');
            ce.amount = amt.toDouble();
            ce.currency = "USD";
            result.data.creditEvents.append(ce);
        }
    }

    // Extract purchase URL if present
    static QRegularExpression purchaseRe("href=\"(https://chat\\.openai\\.com/[^\"]*purchase[^\"]*)\"");
    QRegularExpressionMatch purchaseMatch = purchaseRe.match(html);
    if (purchaseMatch.hasMatch()) {
        result.data.purchaseURL = purchaseMatch.captured(1);
    } else {
        result.data.purchaseURL = "https://chat.openai.com/#purchase";
    }

    result.success = true;
    return result;
}
