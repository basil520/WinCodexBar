#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QTextStream>

class OpenCodeGoDebugTest : public QObject {
    Q_OBJECT

private slots:
    void testSQLiteJsonSupport() {
        QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        QString dbPath = home + "/.local/share/opencode/opencode.db";
        if (!QFile::exists(dbPath)) {
            QString localAppData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
            dbPath = localAppData + "/opencode/opencode.db";
        }

        QFile logFile(QDir::tempPath() + "/opencode_debug.log");
        logFile.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream log(&logFile);

        log << "Database path: " << dbPath << "\n";
        log << "Database exists: " << QFile::exists(dbPath) << "\n";

        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "debug_test");
        db.setDatabaseName(dbPath);
        if (!db.open()) {
            log << "Failed to open DB: " << db.lastError().text() << "\n";
            logFile.close();
            return;
        }

        // Check SQLite version
        QSqlQuery versionQuery(db);
        if (versionQuery.exec("SELECT sqlite_version()")) {
            if (versionQuery.next()) {
                log << "SQLite version: " << versionQuery.value(0).toString() << "\n";
            }
        }

        // Check JSON support
        QSqlQuery jsonQuery(db);
        if (jsonQuery.exec("SELECT json('{""test"": 1}')")) {
            if (jsonQuery.next()) {
                log << "JSON support: YES - " << jsonQuery.value(0).toString() << "\n";
            }
        } else {
            log << "JSON support: NO - " << jsonQuery.lastError().text() << "\n";
        }

        // Count messages
        QSqlQuery countQuery(db);
        if (countQuery.exec("SELECT COUNT(*) FROM message")) {
            if (countQuery.next()) {
                log << "Total messages: " << countQuery.value(0).toInt() << "\n";
            }
        }

        // Count messages with tokens
        QSqlQuery tokenCountQuery(db);
        if (tokenCountQuery.exec("SELECT COUNT(*) FROM message WHERE json_extract(data, '$.tokens') IS NOT NULL")) {
            if (tokenCountQuery.next()) {
                log << "Messages with tokens: " << tokenCountQuery.value(0).toInt() << "\n";
            }
        } else {
            log << "Token count query failed: " << tokenCountQuery.lastError().text() << "\n";
        }

        // Count user messages with model
        QSqlQuery modelCountQuery(db);
        if (modelCountQuery.exec("SELECT COUNT(*) FROM message WHERE json_extract(data, '$.role') = 'user' AND json_extract(data, '$.model') IS NOT NULL")) {
            if (modelCountQuery.next()) {
                log << "User messages with model: " << modelCountQuery.value(0).toInt() << "\n";
            }
        } else {
            log << "Model count query failed: " << modelCountQuery.lastError().text() << "\n";
        }

        // Try the actual aggregation query
        QSqlQuery aggQuery(db);
        QString sql = R"(
            WITH user_models AS (
                SELECT
                    session_id,
                    time_created,
                    json_extract(data, '$.model.providerID') AS provider,
                    json_extract(data, '$.model.modelID') AS model
                FROM message
                WHERE json_extract(data, '$.role') = 'user'
                  AND json_extract(data, '$.model') IS NOT NULL
            ),
            assistant_tokens AS (
                SELECT
                    session_id,
                    time_created,
                    json_extract(data, '$.tokens.input') AS input_tokens,
                    json_extract(data, '$.tokens.output') AS output_tokens,
                    json_extract(data, '$.tokens.cache.read') AS cache_read
                FROM message
                WHERE json_extract(data, '$.role') = 'assistant'
                  AND json_extract(data, '$.tokens') IS NOT NULL
            )
            SELECT
                um.provider || '/' || um.model AS model_name,
                COUNT(*) AS message_count,
                SUM(a.input_tokens) AS input_tokens,
                SUM(a.output_tokens) AS output_tokens
            FROM assistant_tokens a
            LEFT JOIN user_models um
                ON a.session_id = um.session_id
                AND um.time_created = (
                    SELECT MAX(time_created)
                    FROM user_models
                    WHERE session_id = a.session_id
                      AND time_created <= a.time_created
                )
            WHERE um.provider IS NOT NULL
            GROUP BY um.provider, um.model
            ORDER BY input_tokens DESC
        )";

        if (aggQuery.exec(sql)) {
            log << "Aggregation results:\n";
            int rowCount = 0;
            while (aggQuery.next()) {
                log << "  " << aggQuery.value("model_name").toString()
                    << " | count=" << aggQuery.value("message_count").toInt()
                    << " | input=" << aggQuery.value("input_tokens").toInt()
                    << " | output=" << aggQuery.value("output_tokens").toInt() << "\n";
                rowCount++;
            }
            log << "Total rows: " << rowCount << "\n";
        } else {
            log << "Aggregation query failed: " << aggQuery.lastError().text() << "\n";
        }

        db.close();
        QSqlDatabase::removeDatabase("debug_test");
        logFile.close();
    }
};

QTEST_MAIN(OpenCodeGoDebugTest)
#include "opencode_debug_test.moc"
