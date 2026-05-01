#pragma once

#include <QString>

class SingleInstanceGuard {
public:
    explicit SingleInstanceGuard(const QString& key);
    ~SingleInstanceGuard();

    SingleInstanceGuard(const SingleInstanceGuard&) = delete;
    SingleInstanceGuard& operator=(const SingleInstanceGuard&) = delete;

    bool acquired() const { return m_acquired; }
    bool alreadyRunning() const { return m_alreadyRunning; }

private:
    void release();

    void* m_handle = nullptr;
    bool m_acquired = false;
    bool m_alreadyRunning = false;
};
