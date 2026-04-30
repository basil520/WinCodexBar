#include "WindowsCredentialStore.h"

#include <windows.h>
#include <wincred.h>

bool WindowsCredentialStore::write(const QString& target,
                                    const QString& username,
                                    const QByteArray& secret)
{
    CREDENTIALW cred = {};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = const_cast<LPWSTR>(reinterpret_cast<const WCHAR*>(target.utf16()));
    cred.CredentialBlobSize = static_cast<DWORD>(secret.size());
    cred.CredentialBlob = const_cast<LPBYTE>(reinterpret_cast<const BYTE*>(secret.data()));
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    if (!username.isEmpty()) {
        cred.UserName = const_cast<LPWSTR>(reinterpret_cast<const WCHAR*>(username.utf16()));
    }

    BOOL ok = CredWriteW(&cred, 0);
    return ok != FALSE;
}

std::optional<QByteArray> WindowsCredentialStore::read(const QString& target) {
    PCREDENTIALW pcred = nullptr;
    BOOL ok = CredReadW(reinterpret_cast<LPCWSTR>(target.utf16()), CRED_TYPE_GENERIC, 0, &pcred);
    if (!ok || !pcred) {
        return std::nullopt;
    }
    QByteArray data(reinterpret_cast<const char*>(pcred->CredentialBlob),
                    static_cast<int>(pcred->CredentialBlobSize));
    CredFree(pcred);
    return data;
}

bool WindowsCredentialStore::remove(const QString& target) {
    BOOL ok = CredDeleteW(reinterpret_cast<LPCWSTR>(target.utf16()), CRED_TYPE_GENERIC, 0);
    return ok != FALSE;
}

bool WindowsCredentialStore::exists(const QString& target) {
    PCREDENTIALW pcred = nullptr;
    BOOL ok = CredReadW(reinterpret_cast<LPCWSTR>(target.utf16()), CRED_TYPE_GENERIC, 0, &pcred);
    if (pcred) CredFree(pcred);
    return ok != FALSE;
}
