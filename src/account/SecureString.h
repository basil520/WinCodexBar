#pragma once

#include <QByteArray>
#include <QString>

/**
 * @brief A QByteArray wrapper that securely wipes memory on destruction.
 *
 * Use this for storing sensitive data (API keys, tokens, cookies) in memory.
 * The underlying bytes are overwritten with zeros when the SecureString
 * is destroyed or reassigned.
 */
class SecureString {
public:
    SecureString() = default;
    explicit SecureString(const QByteArray& data) : m_data(data) {}
    explicit SecureString(const QString& str) : m_data(str.toUtf8()) {}
    explicit SecureString(const char* str) : m_data(str) {}

    ~SecureString() { clear(); }

    SecureString(const SecureString& other) : m_data(other.m_data) {}
    SecureString(SecureString&& other) noexcept : m_data(std::move(other.m_data)) {
        other.m_data.clear();
    }

    SecureString& operator=(const SecureString& other) {
        if (this != &other) {
            clear();
            m_data = other.m_data;
        }
        return *this;
    }

    SecureString& operator=(SecureString&& other) noexcept {
        if (this != &other) {
            clear();
            m_data = std::move(other.m_data);
            other.m_data.clear();
        }
        return *this;
    }

    bool isEmpty() const { return m_data.isEmpty(); }
    int length() const { return m_data.length(); }

    const QByteArray& data() const { return m_data; }
    QByteArray takeData() { QByteArray tmp = std::move(m_data); m_data.clear(); return tmp; }

    QString toString() const { return QString::fromUtf8(m_data); }

    void clear() {
        if (!m_data.isEmpty()) {
            // Overwrite with zeros
            std::fill(m_data.begin(), m_data.end(), 0);
            m_data.clear();
        }
    }

    bool operator==(const SecureString& other) const { return m_data == other.m_data; }
    bool operator!=(const SecureString& other) const { return m_data != other.m_data; }

private:
    QByteArray m_data;
};
