#include "SingleInstanceGuard.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

SingleInstanceGuard::SingleInstanceGuard(const QString& key)
{
#ifdef Q_OS_WIN
    if (key.isEmpty()) return;

    const std::wstring name = key.toStdWString();
    HANDLE handle = CreateMutexW(nullptr, FALSE, name.c_str());
    if (!handle) {
        m_alreadyRunning = GetLastError() == ERROR_ACCESS_DENIED;
        return;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(handle);
        m_alreadyRunning = true;
        return;
    }

    m_handle = handle;
    m_acquired = true;
#else
    Q_UNUSED(key)
    m_acquired = true;
#endif
}

SingleInstanceGuard::~SingleInstanceGuard()
{
    release();
}

void SingleInstanceGuard::release()
{
#ifdef Q_OS_WIN
    if (m_handle) {
        CloseHandle(static_cast<HANDLE>(m_handle));
        m_handle = nullptr;
    }
#endif
    m_acquired = false;
}
