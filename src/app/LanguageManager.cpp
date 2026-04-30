#include "LanguageManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QQmlEngine>
#include <QJSEngine>

LanguageManager& LanguageManager::instance() {
    static LanguageManager mgr;
    return mgr;
}

LanguageManager::LanguageManager() {
}

QString LanguageManager::displayName(const QString& code) const {
    if (code == "zh_CN") return QStringLiteral("中文");
    if (code == "en") return QStringLiteral("English");
    return code;
}

void LanguageManager::setLanguage(const QString& code) {
    if (m_language == code) return;
    m_language = code;

    qApp->removeTranslator(&m_translator);
    qApp->removeTranslator(&m_qtTranslator);

    QStringList translationDirs;
    const QString envDir = qEnvironmentVariable("WINCODEXBAR_TRANSLATION_DIR");
    if (!envDir.isEmpty()) {
        translationDirs.append(envDir);
    }
    const QDir appDir(QCoreApplication::applicationDirPath());
    translationDirs.append(appDir.filePath("translations"));
    translationDirs.append(appDir.filePath("../translations"));
    translationDirs.append(appDir.filePath("../../translations"));

    translationDirs.removeDuplicates();

    QString loadedFrom;
    for (const QString& baseDir : translationDirs) {
        if (m_translator.load("WinCodexBar_" + code, baseDir)) {
            loadedFrom = QDir::cleanPath(baseDir);
            break;
        }
    }
    if (!loadedFrom.isEmpty()) {
        qApp->installTranslator(&m_translator);
    } else if (code != "en") {
        qWarning() << "Could not load WinCodexBar translation for" << code
                   << "from" << translationDirs;
    }

    for (const QString& baseDir : translationDirs) {
        if (m_qtTranslator.load("qt_" + code, baseDir)) {
            qApp->installTranslator(&m_qtTranslator);
            break;
        }
    }

    ++m_revision;
    emit languageChanged();
    emit translationRevisionChanged();
    emit retranslate();
}

QString LanguageManager::tr(const QString& context, const QString& source) const {
    const QByteArray contextBytes = context.toUtf8();
    const QByteArray sourceBytes = source.toUtf8();
    return QCoreApplication::translate(contextBytes.constData(), sourceBytes.constData());
}

void LanguageManager::install(QQmlEngine* engine) {
    auto* jsEngine = engine->handle();
    Q_UNUSED(jsEngine)

    QObject::connect(this, &LanguageManager::retranslate, engine, [engine]() {
        engine->retranslate();
    });
}
