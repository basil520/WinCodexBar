#include "WindsurfProvider.h"
#include "../../network/NetworkManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QLocale>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringConverter>
#include <QUrl>
#include <QUuid>
#include <QVariant>

#include <algorithm>

namespace {

constexpr auto kPlanInfoKey = "windsurf.settings.cachedPlanInfo";
constexpr auto kDefaultEndpoint =
    "https://windsurf.com/_backend/exa.seat_management_pb.SeatManagementService/GetPlanStatus";

double clampPercent(double value)
{
    return std::clamp(value, 0.0, 100.0);
}

std::optional<double> numberValue(const QJsonObject& object, const QString& key)
{
    const QJsonValue value = object.value(key);
    if (value.isDouble()) return value.toDouble();
    if (value.isString()) {
        bool ok = false;
        const double parsed = value.toString().toDouble(&ok);
        if (ok) return parsed;
    }
    return std::nullopt;
}

std::optional<int> intValue(const QJsonObject& object, const QString& key)
{
    auto value = numberValue(object, key);
    if (!value.has_value()) return std::nullopt;
    return static_cast<int>(*value);
}

std::optional<qint64> int64Value(const QJsonObject& object, const QString& key)
{
    auto value = numberValue(object, key);
    if (!value.has_value()) return std::nullopt;
    return static_cast<qint64>(*value);
}

QString trimDecodedJson(QString value)
{
    while (!value.isEmpty() && (value.front().isSpace() || value.front().unicode() == 0)) {
        value.remove(0, 1);
    }
    while (!value.isEmpty() && (value.back().isSpace() || value.back().unicode() == 0)) {
        value.chop(1);
    }
    return value;
}

bool parsesAsJson(const QString& value)
{
    QJsonParseError error;
    QJsonDocument::fromJson(value.toUtf8(), &error);
    return error.error == QJsonParseError::NoError;
}

QString decodeJsonBlob(const QByteArray& data)
{
    const QString utf8 = trimDecodedJson(QString::fromUtf8(data));
    if (parsesAsJson(utf8)) return utf8;

    QStringDecoder decoder(QStringConverter::Utf16LE);
    const QString utf16 = trimDecodedJson(decoder(data));
    if (parsesAsJson(utf16)) return utf16;

    return {};
}

std::optional<QString> resetDescription(const std::optional<QDateTime>& resetAt)
{
    if (!resetAt.has_value() || !resetAt->isValid()) return std::nullopt;

    const qint64 seconds = QDateTime::currentDateTimeUtc().secsTo(resetAt->toUTC());
    if (seconds <= 0) {
        return QCoreApplication::translate("ProviderLabels", "Expired");
    }

    const qint64 hours = seconds / 3600;
    const qint64 minutes = (seconds % 3600) / 60;
    if (hours > 24) {
        const qint64 days = hours / 24;
        const qint64 remainingHours = hours % 24;
        return QCoreApplication::translate("ProviderLabels", "Resets in %1d %2h")
            .arg(days)
            .arg(remainingHours);
    }
    if (hours > 0) {
        return QCoreApplication::translate("ProviderLabels", "Resets in %1h %2m")
            .arg(hours)
            .arg(minutes);
    }
    return QCoreApplication::translate("ProviderLabels", "Resets in %1m").arg(minutes);
}

std::optional<QDateTime> dateFromUnixSeconds(const std::optional<qint64>& seconds)
{
    if (!seconds.has_value() || *seconds <= 0) return std::nullopt;
    return QDateTime::fromSecsSinceEpoch(*seconds, Qt::UTC);
}

std::optional<QDateTime> dateFromUnixMilliseconds(const std::optional<qint64>& milliseconds)
{
    if (!milliseconds.has_value() || *milliseconds <= 0) return std::nullopt;
    return QDateTime::fromMSecsSinceEpoch(*milliseconds, Qt::UTC);
}

std::optional<RateWindow> usageWindow(std::optional<int> rawUsed,
                                      std::optional<int> rawRemaining,
                                      std::optional<int> rawTotal,
                                      const QString& unit)
{
    if (!rawTotal.has_value() || *rawTotal <= 0) return std::nullopt;

    std::optional<int> used = rawUsed;
    if (!used.has_value() && rawRemaining.has_value()) {
        used = std::max(0, *rawTotal - *rawRemaining);
    }
    if (!used.has_value()) return std::nullopt;

    const int clampedUsed = std::clamp(*used, 0, *rawTotal);
    RateWindow window;
    window.usedPercent = clampPercent(static_cast<double>(clampedUsed) / static_cast<double>(*rawTotal) * 100.0);
    window.resetDescription = QStringLiteral("%1 / %2 %3").arg(clampedUsed).arg(*rawTotal).arg(unit);
    return window;
}

ProviderFetchResult makeResultSkeleton(const QString& strategyID, int kind, const QString& sourceLabel)
{
    ProviderFetchResult result;
    result.strategyID = strategyID;
    result.strategyKind = kind;
    result.sourceLabel = sourceLabel;
    return result;
}

QString envValue(const ProviderFetchContext& ctx, const QString& key)
{
    const QString fromContext = ctx.env.value(key).trimmed();
    if (!fromContext.isEmpty()) return fromContext;
    return qEnvironmentVariable(key.toUtf8().constData()).trimmed();
}

QJsonObject jsonObjectFromText(const QString& json, QString* errorMessage)
{
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        if (errorMessage) *errorMessage = error.errorString();
        return {};
    }
    if (!doc.isObject()) {
        if (errorMessage) *errorMessage = QStringLiteral("expected JSON object");
        return {};
    }
    return doc.object();
}

class ProtoReader {
public:
    struct Field {
        int number = 0;
        int wireType = 0;
    };

    explicit ProtoReader(QByteArray data) : m_data(std::move(data)) {}

    bool nextField(std::optional<Field>& field, QString* errorMessage)
    {
        if (m_index >= m_data.size()) {
            field = std::nullopt;
            return true;
        }

        quint64 key = 0;
        if (!readVarint(key, errorMessage)) return false;
        const int number = static_cast<int>(key >> 3);
        const int wireType = static_cast<int>(key & 0x07);
        if (number <= 0) {
            if (errorMessage) *errorMessage = QStringLiteral("malformed protobuf field key");
            return false;
        }
        if (wireType != 0 && wireType != 1 && wireType != 2 && wireType != 5) {
            if (errorMessage) *errorMessage = QStringLiteral("unsupported protobuf wire type %1").arg(wireType);
            return false;
        }
        field = Field{number, wireType};
        return true;
    }

    bool readVarint(quint64& value, QString* errorMessage)
    {
        quint64 result = 0;
        int shift = 0;
        while (m_index < m_data.size()) {
            const quint8 byte = static_cast<quint8>(m_data.at(m_index++));
            result |= static_cast<quint64>(byte & 0x7f) << shift;
            if ((byte & 0x80) == 0) {
                value = result;
                return true;
            }
            shift += 7;
            if (shift >= 64) break;
        }
        if (errorMessage) *errorMessage = QStringLiteral("truncated protobuf varint");
        return false;
    }

    bool readLengthDelimited(QByteArray& value, QString* errorMessage)
    {
        quint64 length = 0;
        if (!readVarint(length, errorMessage)) return false;
        if (length > static_cast<quint64>(m_data.size() - m_index)) {
            if (errorMessage) *errorMessage = QStringLiteral("truncated protobuf payload");
            return false;
        }
        value = m_data.mid(m_index, static_cast<int>(length));
        m_index += static_cast<int>(length);
        return true;
    }

    bool readString(QString& value, QString* errorMessage)
    {
        QByteArray data;
        if (!readLengthDelimited(data, errorMessage)) return false;
        value = QString::fromUtf8(data);
        return true;
    }

    bool skip(int wireType, QString* errorMessage)
    {
        switch (wireType) {
        case 0: {
            quint64 ignored = 0;
            return readVarint(ignored, errorMessage);
        }
        case 1:
            if (m_index + 8 <= m_data.size()) {
                m_index += 8;
                return true;
            }
            break;
        case 2: {
            QByteArray ignored;
            return readLengthDelimited(ignored, errorMessage);
        }
        case 5:
            if (m_index + 4 <= m_data.size()) {
                m_index += 4;
                return true;
            }
            break;
        }
        if (errorMessage) *errorMessage = QStringLiteral("truncated protobuf payload");
        return false;
    }

private:
    QByteArray m_data;
    int m_index = 0;
};

void appendVarint(QByteArray& data, quint64 value)
{
    while (value >= 0x80) {
        data.append(static_cast<char>((value & 0x7f) | 0x80));
        value >>= 7;
    }
    data.append(static_cast<char>(value));
}

void appendFieldKey(QByteArray& data, int fieldNumber, int wireType)
{
    appendVarint(data, static_cast<quint64>((fieldNumber << 3) | wireType));
}

void appendLengthDelimited(QByteArray& data, int fieldNumber, const QByteArray& value)
{
    appendFieldKey(data, fieldNumber, 2);
    appendVarint(data, static_cast<quint64>(value.size()));
    data.append(value);
}

std::optional<QDateTime> decodeTimestamp(const QByteArray& data, QString* errorMessage)
{
    ProtoReader reader(data);
    qint64 seconds = 0;
    qint64 nanos = 0;

    std::optional<ProtoReader::Field> field;
    while (reader.nextField(field, errorMessage) && field.has_value()) {
        switch (field->number) {
        case 1: {
            quint64 value = 0;
            if (!reader.readVarint(value, errorMessage)) return std::nullopt;
            seconds = static_cast<qint64>(value);
            break;
        }
        case 2: {
            quint64 value = 0;
            if (!reader.readVarint(value, errorMessage)) return std::nullopt;
            nanos = static_cast<qint64>(value);
            break;
        }
        default:
            if (!reader.skip(field->wireType, errorMessage)) return std::nullopt;
            break;
        }
    }
    if (errorMessage && !errorMessage->isEmpty()) return std::nullopt;
    return QDateTime::fromMSecsSinceEpoch(seconds * 1000 + nanos / 1000000, Qt::UTC);
}

bool decodePlanInfo(const QByteArray& data, WindsurfPlanStatus& status, QString* errorMessage)
{
    ProtoReader reader(data);
    std::optional<ProtoReader::Field> field;
    while (reader.nextField(field, errorMessage) && field.has_value()) {
        switch (field->number) {
        case 1: {
            quint64 ignored = 0;
            if (!reader.readVarint(ignored, errorMessage)) return false;
            break;
        }
        case 2:
            if (!reader.readString(status.planName, errorMessage)) return false;
            break;
        default:
            if (!reader.skip(field->wireType, errorMessage)) return false;
            break;
        }
    }
    return errorMessage ? errorMessage->isEmpty() : true;
}

std::optional<WindsurfPlanStatus> decodePlanStatus(const QByteArray& data, QString* errorMessage)
{
    ProtoReader reader(data);
    WindsurfPlanStatus status;

    std::optional<ProtoReader::Field> field;
    while (reader.nextField(field, errorMessage) && field.has_value()) {
        switch (field->number) {
        case 1: {
            QByteArray chunk;
            if (!reader.readLengthDelimited(chunk, errorMessage)) return std::nullopt;
            if (!decodePlanInfo(chunk, status, errorMessage)) return std::nullopt;
            break;
        }
        case 2: {
            QByteArray chunk;
            if (!reader.readLengthDelimited(chunk, errorMessage)) return std::nullopt;
            status.planStart = decodeTimestamp(chunk, errorMessage);
            if (!status.planStart.has_value()) return std::nullopt;
            break;
        }
        case 3: {
            QByteArray chunk;
            if (!reader.readLengthDelimited(chunk, errorMessage)) return std::nullopt;
            status.planEnd = decodeTimestamp(chunk, errorMessage);
            if (!status.planEnd.has_value()) return std::nullopt;
            break;
        }
        case 14: {
            quint64 value = 0;
            if (!reader.readVarint(value, errorMessage)) return std::nullopt;
            status.dailyQuotaRemainingPercent = static_cast<int>(value);
            break;
        }
        case 15: {
            quint64 value = 0;
            if (!reader.readVarint(value, errorMessage)) return std::nullopt;
            status.weeklyQuotaRemainingPercent = static_cast<int>(value);
            break;
        }
        case 17: {
            quint64 value = 0;
            if (!reader.readVarint(value, errorMessage)) return std::nullopt;
            status.dailyQuotaResetAtUnix = static_cast<qint64>(value);
            break;
        }
        case 18: {
            quint64 value = 0;
            if (!reader.readVarint(value, errorMessage)) return std::nullopt;
            status.weeklyQuotaResetAtUnix = static_cast<qint64>(value);
            break;
        }
        default:
            if (!reader.skip(field->wireType, errorMessage)) return std::nullopt;
            break;
        }
    }
    if (errorMessage && !errorMessage->isEmpty()) return std::nullopt;
    return status;
}

QString responseSnippet(const QByteArray& body)
{
    const QString text = QString::fromUtf8(body).trimmed();
    if (!text.isEmpty()) return QStringLiteral(": %1").arg(text.left(200));
    return QStringLiteral(": <binary %1 bytes>").arg(body.size());
}

} // namespace

bool WindsurfDevinSessionAuth::isValid() const
{
    return !sessionToken.trimmed().isEmpty()
        && !auth1Token.trimmed().isEmpty()
        && !accountID.trimmed().isEmpty()
        && !primaryOrgID.trimmed().isEmpty();
}

std::optional<WindsurfCachedPlanInfo> WindsurfCachedPlanInfo::fromJson(const QString& json,
                                                                        QString* errorMessage)
{
    if (errorMessage) errorMessage->clear();
    const QJsonObject root = jsonObjectFromText(json, errorMessage);
    if (root.isEmpty() && errorMessage && !errorMessage->isEmpty()) {
        return std::nullopt;
    }

    WindsurfCachedPlanInfo info;
    info.planName = root.value(QStringLiteral("planName")).toString();
    info.startTimestamp = int64Value(root, QStringLiteral("startTimestamp"));
    info.endTimestamp = int64Value(root, QStringLiteral("endTimestamp"));

    const QJsonObject usageObject = root.value(QStringLiteral("usage")).toObject();
    if (!usageObject.isEmpty()) {
        Usage usage;
        usage.messages = intValue(usageObject, QStringLiteral("messages"));
        usage.usedMessages = intValue(usageObject, QStringLiteral("usedMessages"));
        usage.remainingMessages = intValue(usageObject, QStringLiteral("remainingMessages"));
        usage.flowActions = intValue(usageObject, QStringLiteral("flowActions"));
        usage.usedFlowActions = intValue(usageObject, QStringLiteral("usedFlowActions"));
        usage.remainingFlowActions = intValue(usageObject, QStringLiteral("remainingFlowActions"));
        usage.flexCredits = intValue(usageObject, QStringLiteral("flexCredits"));
        usage.usedFlexCredits = intValue(usageObject, QStringLiteral("usedFlexCredits"));
        usage.remainingFlexCredits = intValue(usageObject, QStringLiteral("remainingFlexCredits"));
        info.usage = usage;
    }

    const QJsonObject quotaObject = root.value(QStringLiteral("quotaUsage")).toObject();
    if (!quotaObject.isEmpty()) {
        QuotaUsage quota;
        quota.dailyRemainingPercent = numberValue(quotaObject, QStringLiteral("dailyRemainingPercent"));
        quota.weeklyRemainingPercent = numberValue(quotaObject, QStringLiteral("weeklyRemainingPercent"));
        quota.dailyResetAtUnix = int64Value(quotaObject, QStringLiteral("dailyResetAtUnix"));
        quota.weeklyResetAtUnix = int64Value(quotaObject, QStringLiteral("weeklyResetAtUnix"));
        info.quotaUsage = quota;
    }

    return info;
}

UsageSnapshot WindsurfCachedPlanInfo::toUsageSnapshot() const
{
    UsageSnapshot snapshot;
    snapshot.updatedAt = QDateTime::currentDateTime();

    if (quotaUsage.has_value()) {
        if (quotaUsage->dailyRemainingPercent.has_value()) {
            const auto resetAt = dateFromUnixSeconds(quotaUsage->dailyResetAtUnix);
            RateWindow window;
            window.usedPercent = clampPercent(100.0 - *quotaUsage->dailyRemainingPercent);
            window.resetsAt = resetAt;
            window.resetDescription = resetDescription(resetAt);
            snapshot.primary = window;
        }
        if (quotaUsage->weeklyRemainingPercent.has_value()) {
            const auto resetAt = dateFromUnixSeconds(quotaUsage->weeklyResetAtUnix);
            RateWindow window;
            window.usedPercent = clampPercent(100.0 - *quotaUsage->weeklyRemainingPercent);
            window.resetsAt = resetAt;
            window.resetDescription = resetDescription(resetAt);
            snapshot.secondary = window;
        }
    }

    if (!snapshot.primary.has_value() && usage.has_value()) {
        snapshot.primary = usageWindow(usage->usedMessages,
                                       usage->remainingMessages,
                                       usage->messages,
                                       QStringLiteral("messages"));
    }
    if (!snapshot.secondary.has_value() && usage.has_value()) {
        snapshot.secondary = usageWindow(usage->usedFlowActions,
                                         usage->remainingFlowActions,
                                         usage->flowActions,
                                         QStringLiteral("flow actions"));
    }

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::windsurf;
    if (!planName.trimmed().isEmpty()) {
        identity.loginMethod = planName.trimmed();
    }
    const auto endDate = dateFromUnixMilliseconds(endTimestamp);
    if (endDate.has_value()) {
        identity.accountOrganization = QStringLiteral("Expires %1")
            .arg(QLocale::system().toString(endDate->date(), QLocale::ShortFormat));
    }
    snapshot.identity = identity;

    return snapshot;
}

QByteArray WindsurfPlanStatusProtoCodec::encodeRequest(const QString& authToken, bool includeTopUpStatus)
{
    QByteArray data;
    appendLengthDelimited(data, 1, authToken.toUtf8());
    appendFieldKey(data, 2, 0);
    appendVarint(data, includeTopUpStatus ? 1 : 0);
    return data;
}

std::optional<WindsurfPlanStatusProtoCodec::Request> WindsurfPlanStatusProtoCodec::decodeRequest(
    const QByteArray& data,
    QString* errorMessage)
{
    if (errorMessage) errorMessage->clear();
    ProtoReader reader(data);
    Request request;
    bool hasAuthToken = false;

    std::optional<ProtoReader::Field> field;
    while (reader.nextField(field, errorMessage) && field.has_value()) {
        switch (field->number) {
        case 1:
            if (!reader.readString(request.authToken, errorMessage)) return std::nullopt;
            hasAuthToken = true;
            break;
        case 2: {
            quint64 value = 0;
            if (!reader.readVarint(value, errorMessage)) return std::nullopt;
            request.includeTopUpStatus = value != 0;
            break;
        }
        default:
            if (!reader.skip(field->wireType, errorMessage)) return std::nullopt;
            break;
        }
    }

    if (errorMessage && !errorMessage->isEmpty()) return std::nullopt;
    if (!hasAuthToken) {
        if (errorMessage) *errorMessage = QStringLiteral("missing protobuf field auth_token");
        return std::nullopt;
    }
    return request;
}

std::optional<WindsurfPlanStatus> WindsurfPlanStatusProtoCodec::decodeResponse(const QByteArray& data,
                                                                                QString* errorMessage)
{
    if (errorMessage) errorMessage->clear();
    ProtoReader reader(data);
    std::optional<WindsurfPlanStatus> status;

    std::optional<ProtoReader::Field> field;
    while (reader.nextField(field, errorMessage) && field.has_value()) {
        switch (field->number) {
        case 1: {
            QByteArray chunk;
            if (!reader.readLengthDelimited(chunk, errorMessage)) return std::nullopt;
            status = decodePlanStatus(chunk, errorMessage);
            if (!status.has_value()) return std::nullopt;
            break;
        }
        default:
            if (!reader.skip(field->wireType, errorMessage)) return std::nullopt;
            break;
        }
    }

    if (errorMessage && !errorMessage->isEmpty()) return std::nullopt;
    if (!status.has_value()) {
        if (errorMessage) *errorMessage = QStringLiteral("missing protobuf field plan_status");
        return std::nullopt;
    }
    return status;
}

WindsurfProvider::WindsurfProvider(QObject* parent) : IProvider(parent) {}

QVector<IFetchStrategy*> WindsurfProvider::createStrategies(const ProviderFetchContext& ctx)
{
    Q_UNUSED(ctx)
    return {new WindsurfWebStrategy(this), new WindsurfLocalStrategy(this)};
}

QVector<ProviderSettingsDescriptor> WindsurfProvider::settingsDescriptors() const
{
    return {
        {"sourceMode", "Usage source", "picker", QVariant(QStringLiteral("auto")),
         {{"auto", "Auto"}, {"web", "Web API"}, {"cli", "Local (SQLite cache)"}}},
        {"cookieSource", "Cookie source", "picker", QVariant(QStringLiteral("off")),
         {{"off", "Off"}, {"manual", "Manual"}}},
        {"manualCookieHeader", "Windsurf session JSON bundle", "secret", QVariant(),
         {}, "com.codexbar.session.windsurf", "CODEXBAR_WINDSURF_SESSION",
         "{\"devin_session_token\":\"...\"}", "Stored in Windows Credential Manager", true, true}
    };
}

WindsurfWebStrategy::WindsurfWebStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool WindsurfWebStrategy::isAvailable(const ProviderFetchContext& ctx) const
{
    if (ctx.sourceMode == ProviderSourceMode::Web) return true;
    const QString cookieSource = ctx.settings.get(QStringLiteral("cookieSource"),
                                                  QStringLiteral("off")).toString();
    return cookieSource == QStringLiteral("manual") && !resolveManualSessionInput(ctx).isEmpty();
}

bool WindsurfWebStrategy::shouldFallback(const ProviderFetchResult& result,
                                         const ProviderFetchContext& ctx) const
{
    return !result.success && ctx.sourceMode == ProviderSourceMode::Auto;
}

QString WindsurfWebStrategy::resolveManualSessionInput(const ProviderFetchContext& ctx)
{
    const QString cookieSource = ctx.settings.get(QStringLiteral("cookieSource"),
                                                  QStringLiteral("off")).toString();
    if (cookieSource != QStringLiteral("manual")) return {};

    if (ctx.manualCookieHeader.has_value() && !ctx.manualCookieHeader->trimmed().isEmpty()) {
        return ctx.manualCookieHeader->trimmed();
    }
    return ctx.settings.get(QStringLiteral("manualCookieHeader")).toString().trimmed();
}

QUrl WindsurfWebStrategy::endpointURL(const ProviderFetchContext& ctx)
{
    const QString overrideURL = envValue(ctx, QStringLiteral("CODEXBAR_WINDSURF_PLAN_STATUS_URL"));
    return QUrl(overrideURL.isEmpty() ? QString::fromUtf8(kDefaultEndpoint) : overrideURL);
}

std::optional<WindsurfDevinSessionAuth> WindsurfWebStrategy::parseManualSessionInput(
    const QString& raw,
    QString* errorMessage)
{
    if (errorMessage) errorMessage->clear();
    const QString trimmed = raw.trimmed();
    if (trimmed.isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("empty input");
        return std::nullopt;
    }

    auto valueForKeys = [](const QHash<QString, QString>& values,
                           const QStringList& keys) -> QString {
        for (const auto& key : keys) {
            const QString value = values.value(key).trimmed();
            if (!value.isEmpty()) return value;
        }
        return {};
    };

    auto authFromValues = [&](const QHash<QString, QString>& values) -> std::optional<WindsurfDevinSessionAuth> {
        WindsurfDevinSessionAuth auth;
        auth.sessionToken = valueForKeys(values, {
            QStringLiteral("devin_session_token"),
            QStringLiteral("devinSessionToken"),
            QStringLiteral("sessionToken")
        });
        auth.auth1Token = valueForKeys(values, {
            QStringLiteral("devin_auth1_token"),
            QStringLiteral("devinAuth1Token"),
            QStringLiteral("auth1Token")
        });
        auth.accountID = valueForKeys(values, {
            QStringLiteral("devin_account_id"),
            QStringLiteral("devinAccountId"),
            QStringLiteral("accountID"),
            QStringLiteral("accountId")
        });
        auth.primaryOrgID = valueForKeys(values, {
            QStringLiteral("devin_primary_org_id"),
            QStringLiteral("devinPrimaryOrgId"),
            QStringLiteral("primaryOrgID"),
            QStringLiteral("primaryOrgId")
        });
        if (!auth.isValid()) return std::nullopt;
        return auth;
    };

    QJsonParseError jsonError;
    const QJsonDocument doc = QJsonDocument::fromJson(trimmed.toUtf8(), &jsonError);
    if (jsonError.error == QJsonParseError::NoError && doc.isObject()) {
        QHash<QString, QString> values;
        const QJsonObject object = doc.object();
        for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
            values[it.key()] = it.value().toString().trimmed();
        }
        auto auth = authFromValues(values);
        if (auth.has_value()) return auth;
    }

    QHash<QString, QString> values;
    QString keyValueText = trimmed;
    if (keyValueText.startsWith('{')) keyValueText.remove(0, 1);
    if (keyValueText.endsWith('}')) keyValueText.chop(1);
    const QStringList segments = keyValueText.split(QRegularExpression(QStringLiteral("[\\n,;]")),
                                                    Qt::SkipEmptyParts);
    for (const auto& rawSegment : segments) {
        const QString segment = rawSegment.trimmed();
        const int equalsIndex = segment.indexOf('=');
        const int colonIndex = segment.indexOf(':');
        int delimiter = -1;
        if (equalsIndex >= 0) {
            delimiter = equalsIndex;
        } else if (colonIndex >= 0) {
            delimiter = colonIndex;
        }
        if (delimiter <= 0) continue;

        const QString key = segment.left(delimiter).trimmed().remove('"').remove('\'');
        QString value = segment.mid(delimiter + 1).trimmed();
        if ((value.startsWith('"') && value.endsWith('"')) ||
            (value.startsWith('\'') && value.endsWith('\''))) {
            value = value.mid(1, value.size() - 2);
        }
        values[key] = value.trimmed();
    }

    auto auth = authFromValues(values);
    if (auth.has_value()) return auth;

    if (errorMessage) {
        *errorMessage = QStringLiteral(
            "expected JSON with devin_session_token, devin_auth1_token, "
            "devin_account_id, and devin_primary_org_id");
    }
    return std::nullopt;
}

QHash<QString, QString> WindsurfWebStrategy::buildHeaders(const WindsurfDevinSessionAuth& auth)
{
    QHash<QString, QString> headers;
    headers[QStringLiteral("Accept")] = QStringLiteral("application/proto");
    headers[QStringLiteral("Connect-Protocol-Version")] = QStringLiteral("1");
    headers[QStringLiteral("Origin")] = QStringLiteral("https://windsurf.com");
    headers[QStringLiteral("Referer")] = QStringLiteral("https://windsurf.com/profile");
    headers[QStringLiteral("x-auth-token")] = auth.sessionToken;
    headers[QStringLiteral("x-devin-session-token")] = auth.sessionToken;
    headers[QStringLiteral("x-devin-auth1-token")] = auth.auth1Token;
    headers[QStringLiteral("x-devin-account-id")] = auth.accountID;
    headers[QStringLiteral("x-devin-primary-org-id")] = auth.primaryOrgID;
    return headers;
}

UsageSnapshot WindsurfWebStrategy::planStatusToUsageSnapshot(const WindsurfPlanStatus& status)
{
    UsageSnapshot snapshot;
    snapshot.updatedAt = QDateTime::currentDateTime();

    if (status.dailyQuotaRemainingPercent.has_value()) {
        const auto resetAt = dateFromUnixSeconds(status.dailyQuotaResetAtUnix);
        RateWindow window;
        window.usedPercent = clampPercent(100.0 - static_cast<double>(*status.dailyQuotaRemainingPercent));
        window.resetsAt = resetAt;
        window.resetDescription = resetDescription(resetAt);
        snapshot.primary = window;
    }

    if (status.weeklyQuotaRemainingPercent.has_value()) {
        const auto resetAt = dateFromUnixSeconds(status.weeklyQuotaResetAtUnix);
        RateWindow window;
        window.usedPercent = clampPercent(100.0 - static_cast<double>(*status.weeklyQuotaRemainingPercent));
        window.resetsAt = resetAt;
        window.resetDescription = resetDescription(resetAt);
        snapshot.secondary = window;
    }

    ProviderIdentitySnapshot identity;
    identity.providerID = UsageProvider::windsurf;
    if (!status.planName.trimmed().isEmpty()) {
        identity.loginMethod = status.planName.trimmed();
    }
    if (status.planEnd.has_value()) {
        identity.accountOrganization = QStringLiteral("Expires %1")
            .arg(QLocale::system().toString(status.planEnd->date(), QLocale::ShortFormat));
    }
    snapshot.identity = identity;

    return snapshot;
}

ProviderFetchResult WindsurfWebStrategy::fetchSync(const ProviderFetchContext& ctx)
{
    ProviderFetchResult result = makeResultSkeleton(id(), kind(), QStringLiteral("windsurf-web"));
    const QString cookieSource = ctx.settings.get(QStringLiteral("cookieSource"),
                                                  QStringLiteral("off")).toString();
    if (cookieSource == QStringLiteral("off")) {
        result.errorMessage = QStringLiteral(
            "Windsurf web source is disabled. Set Cookie source to Manual and paste a session bundle.");
        return result;
    }
    if (cookieSource != QStringLiteral("manual")) {
        result.errorMessage = QStringLiteral(
            "Automatic Windsurf web session import is not available on Windows yet. Use Manual session bundle.");
        return result;
    }

    const QString sessionInput = resolveManualSessionInput(ctx);
    if (sessionInput.isEmpty()) {
        result.errorMessage = QStringLiteral("Manual Windsurf session bundle not configured.");
        return result;
    }

    QString parseError;
    auto auth = parseManualSessionInput(sessionInput, &parseError);
    if (!auth.has_value()) {
        result.errorMessage = QStringLiteral("Invalid Windsurf session payload: %1").arg(parseError);
        return result;
    }

    const QByteArray requestBody = WindsurfPlanStatusProtoCodec::encodeRequest(auth->sessionToken, true);
    auto [responseBody, httpStatus] = NetworkManager::instance().postBytesSyncWithStatus(
        endpointURL(ctx),
        requestBody,
        buildHeaders(*auth),
        QByteArrayLiteral("application/proto"),
        ctx.networkTimeoutMs,
        true);

    if (httpStatus != 200) {
        if (httpStatus == 0) {
            result.errorMessage = QStringLiteral("Windsurf API call failed: network timeout or connection error.");
        } else if (httpStatus == 400 || httpStatus == 401 || httpStatus == 403) {
            result.errorMessage = QStringLiteral("Windsurf session expired or unauthorized (HTTP %1)%2")
                .arg(httpStatus)
                .arg(responseSnippet(responseBody));
        } else {
            result.errorMessage = QStringLiteral("Windsurf API call failed: HTTP %1%2")
                .arg(httpStatus)
                .arg(responseSnippet(responseBody));
        }
        return result;
    }

    QString decodeError;
    auto status = WindsurfPlanStatusProtoCodec::decodeResponse(responseBody, &decodeError);
    if (!status.has_value()) {
        result.errorMessage = QStringLiteral("Windsurf API parse error: %1").arg(decodeError);
        return result;
    }

    result.usage = planStatusToUsageSnapshot(*status);
    result.success = result.usage.primary.has_value()
        || result.usage.secondary.has_value()
        || (result.usage.identity.has_value() && result.usage.identity->loginMethod.has_value());
    if (!result.success) {
        result.errorMessage = QStringLiteral("No Windsurf usage data found in API response.");
    }
    return result;
}

WindsurfLocalStrategy::WindsurfLocalStrategy(QObject* parent) : IFetchStrategy(parent) {}

bool WindsurfLocalStrategy::isAvailable(const ProviderFetchContext& ctx) const
{
    for (const auto& path : candidateStateDBPaths(ctx)) {
        if (QFile::exists(path)) return true;
    }
    return ctx.sourceMode == ProviderSourceMode::CLI;
}

bool WindsurfLocalStrategy::shouldFallback(const ProviderFetchResult& result,
                                           const ProviderFetchContext& ctx) const
{
    Q_UNUSED(result)
    Q_UNUSED(ctx)
    return false;
}

QStringList WindsurfLocalStrategy::candidateStateDBPaths(const ProviderFetchContext& ctx)
{
    QStringList paths;
    const QString overridePath = envValue(ctx, QStringLiteral("CODEXBAR_WINDSURF_STATE_DB"));
    if (!overridePath.isEmpty()) paths.append(QDir::fromNativeSeparators(overridePath));

    const QString appData = envValue(ctx, QStringLiteral("APPDATA"));
    if (!appData.isEmpty()) {
        paths.append(QDir::fromNativeSeparators(appData)
                     + QStringLiteral("/Windsurf/User/globalStorage/state.vscdb"));
    }

    const QString localAppData = envValue(ctx, QStringLiteral("LOCALAPPDATA"));
    if (!localAppData.isEmpty()) {
        paths.append(QDir::fromNativeSeparators(localAppData)
                     + QStringLiteral("/Windsurf/User/globalStorage/state.vscdb"));
    }

    const QString genericConfig = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (!genericConfig.isEmpty()) {
        paths.append(genericConfig + QStringLiteral("/Windsurf/User/globalStorage/state.vscdb"));
    }

    paths.removeDuplicates();
    return paths;
}

ProviderFetchResult WindsurfLocalStrategy::parseCachedPlanInfoJson(const QString& json)
{
    ProviderFetchResult result = makeResultSkeleton(QStringLiteral("windsurf.local"),
                                                    ProviderFetchKind::CLI,
                                                    QStringLiteral("local"));
    QString error;
    auto info = WindsurfCachedPlanInfo::fromJson(json, &error);
    if (!info.has_value()) {
        result.errorMessage = QStringLiteral("Could not parse Windsurf plan data: %1").arg(error);
        return result;
    }

    result.usage = info->toUsageSnapshot();
    result.success = result.usage.primary.has_value()
        || result.usage.secondary.has_value()
        || (result.usage.identity.has_value() && result.usage.identity->loginMethod.has_value());
    if (!result.success) {
        result.errorMessage = QStringLiteral("No Windsurf usage data found in cached plan info.");
    }
    return result;
}

QString WindsurfLocalStrategy::decodeSQLiteValue(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) return {};
    if (value.metaType().id() == QMetaType::QByteArray) {
        return decodeJsonBlob(value.toByteArray());
    }
    const QString text = value.toString();
    if (parsesAsJson(text.trimmed())) return text.trimmed();
    const QByteArray bytes = value.toByteArray();
    if (!bytes.isEmpty()) return decodeJsonBlob(bytes);
    return {};
}

ProviderFetchResult WindsurfLocalStrategy::readDatabase(const QString& dbPath)
{
    ProviderFetchResult result = makeResultSkeleton(QStringLiteral("windsurf.local"),
                                                    ProviderFetchKind::CLI,
                                                    QStringLiteral("local"));
    if (!QFile::exists(dbPath)) {
        result.errorMessage = QStringLiteral(
            "Windsurf cache not found. Launch Windsurf and sign in, or use Web source with a manual session bundle.");
        return result;
    }

    const QString connectionName = QStringLiteral("windsurf-%1").arg(QUuid::createUuid().toString(QUuid::Id128));
    QString json;
    QString sqlError;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        db.setDatabaseName(dbPath);
        db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY;QSQLITE_BUSY_TIMEOUT=250"));
        if (!db.open()) {
            sqlError = db.lastError().text();
        } else {
            QSqlQuery query(db);
            const QString sql = QStringLiteral("SELECT value FROM ItemTable WHERE key = '%1' LIMIT 1;")
                .arg(QString::fromUtf8(kPlanInfoKey));
            if (!query.exec(sql)) {
                sqlError = query.lastError().text();
            } else if (!query.next()) {
                result.errorMessage = QStringLiteral("No plan data found in Windsurf database. Sign in to Windsurf first.");
            } else {
                json = decodeSQLiteValue(query.value(0));
                if (json.isEmpty()) {
                    result.errorMessage = QStringLiteral("No readable Windsurf plan data found in database.");
                }
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connectionName);

    if (!sqlError.isEmpty()) {
        result.errorMessage = QStringLiteral("SQLite error reading Windsurf data: %1").arg(sqlError);
        return result;
    }
    if (!result.errorMessage.isEmpty()) {
        return result;
    }
    return parseCachedPlanInfoJson(json);
}

ProviderFetchResult WindsurfLocalStrategy::fetchSync(const ProviderFetchContext& ctx)
{
    for (const auto& path : candidateStateDBPaths(ctx)) {
        if (QFile::exists(path)) {
            return readDatabase(path);
        }
    }

    ProviderFetchResult result = makeResultSkeleton(id(), kind(), QStringLiteral("local"));
    result.errorMessage = QStringLiteral(
        "Windsurf cache not found. Launch Windsurf and sign in, or use Web source with a manual session bundle.");
    return result;
}
