#include "KiroProvider.h"
#include "../../util/BinaryLocator.h"
#include "../../util/TextParser.h"
#include <QProcess>
#include <QRegularExpression>
#include <algorithm>

KiroProvider::KiroProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> KiroProvider::createStrategies(const ProviderFetchContext& ctx) {
    return { new KiroCLIStrategy(const_cast<KiroProvider*>(this)) };
}

KiroCLIStrategy::KiroCLIStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool KiroCLIStrategy::isAvailable(const ProviderFetchContext&) const {
    return BinaryLocator::isAvailable("kiro-cli");
}

static UsageSnapshot parseKiroOutput(const QString& raw) {
    QString text = TextParser::stripAnsiEscapes(raw);
    UsageSnapshot snap;
    snap.updatedAt = QDateTime::currentDateTime();

    static QRegularExpression creditsRe("█+\\s*(\\d+)%");
    static QRegularExpression coveredRe("\\(([\\d.]+)\\s*of\\s*([\\d.]+)\\s*covered");
    static QRegularExpression bonusRe("Bonus credits:\\s*([\\d.]+)/([\\d.]+)\\s*credits used");
    static QRegularExpression expiresRe("expires in\\s*(\\d+)\\s*days");
    static QRegularExpression planRe("\\|\\s*([^|]+?)\\s*\\|");
    static QRegularExpression newPlanRe("Plan:\\s*(.+)");

    auto creditsMatch = creditsRe.match(text);
    auto coveredMatch = coveredRe.match(text);
    auto bonusMatch = bonusRe.match(text);
    auto expiresMatch = expiresRe.match(text);
    auto planMatch = planRe.match(text);
    auto newPlanMatch = newPlanRe.match(text);

    double creditsUsed = 0, creditsTotal = 0;

    if (creditsMatch.hasMatch()) {
        double pct = creditsMatch.captured(1).toDouble();
        snap.primary = RateWindow{std::clamp(pct, 0.0, 100.0), std::nullopt, std::nullopt, QString(), std::nullopt};
    }
    if (coveredMatch.hasMatch()) {
        creditsUsed = coveredMatch.captured(1).toDouble();
        creditsTotal = coveredMatch.captured(2).toDouble();
    }
    if (!snap.primary.has_value() && creditsTotal > 0) {
        double pct = std::clamp(creditsUsed / creditsTotal * 100.0, 0.0, 100.0);
        snap.primary = RateWindow{pct, std::nullopt, std::nullopt, QString(), std::nullopt};
    }
    if (bonusMatch.hasMatch()) {
        double bUsed = bonusMatch.captured(1).toDouble();
        double bTotal = bonusMatch.captured(2).toDouble();
        double bPct = bTotal > 0 ? std::clamp(bUsed / bTotal * 100.0, 0.0, 100.0) : 0;
        QString desc;
        if (expiresMatch.hasMatch()) desc = QString("expires in %1 days").arg(expiresMatch.captured(1));
        snap.secondary = RateWindow{bPct, std::nullopt, std::nullopt, desc, std::nullopt};
    }

    QString plan;
    if (newPlanMatch.hasMatch()) plan = newPlanMatch.captured(1).trimmed();
    else if (planMatch.hasMatch()) plan = planMatch.captured(1).trimmed();

    if (!plan.isEmpty()) {
        ProviderIdentitySnapshot id;
        id.providerID = UsageProvider::kiro;
        id.loginMethod = plan;
        snap.identity = id;
    }
    return snap;
}

ProviderFetchResult KiroCLIStrategy::fetchSync(const ProviderFetchContext&) {
    ProviderFetchResult result;
    result.strategyID = "kiro.cli";
    result.strategyKind = 0;
    result.sourceLabel = "cli";

    QString binary = BinaryLocator::resolve("kiro-cli");
    QProcess proc;
    proc.start(binary, {"chat", "--no-interactive", "/usage"});
    if (!proc.waitForFinished(20000)) {
        proc.kill();
        result.errorMessage = "kiro-cli timed out";
        return result;
    }

    QString output = proc.readAllStandardOutput() + proc.readAllStandardError();
    if (output.isEmpty()) {
        result.errorMessage = "empty CLI output";
        return result;
    }

    result.usage = parseKiroOutput(output);
    result.success = true;
    return result;
}
