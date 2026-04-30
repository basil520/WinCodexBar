#pragma once

#include <QObject>
#include <QTranslator>
#include <QString>

class QQmlEngine;

class LanguageManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(int translationRevision READ translationRevision NOTIFY translationRevisionChanged)

public:
    static LanguageManager& instance();

    QString language() const { return m_language; }
    int translationRevision() const { return m_revision; }
    QStringList availableLanguages() const { return {"en", "zh_CN"}; }
    QString displayName(const QString& code) const;

    QString tr(const QString& context, const QString& source) const;
    void install(QQmlEngine* engine);

public slots:
    void setLanguage(const QString& code);

signals:
    void languageChanged();
    void translationRevisionChanged();
    void retranslate();

private:
    LanguageManager();

    QString m_language = "en";
    int m_revision = 0;
    QTranslator m_translator;
    QTranslator m_qtTranslator;
};
