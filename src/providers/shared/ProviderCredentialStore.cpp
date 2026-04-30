#include "ProviderCredentialStore.h"

#include "WindowsCredentialStore.h"

#include <mutex>

namespace {

class WindowsCredentialBackend : public ProviderCredentialBackend {
public:
    bool write(const QString& target, const QString& username, const QByteArray& secret) override {
        return WindowsCredentialStore::write(target, username, secret);
    }

    std::optional<QByteArray> read(const QString& target) override {
        return WindowsCredentialStore::read(target);
    }

    bool remove(const QString& target) override {
        return WindowsCredentialStore::remove(target);
    }

    bool exists(const QString& target) override {
        return WindowsCredentialStore::exists(target);
    }
};

std::shared_ptr<ProviderCredentialBackend>& backend()
{
    static std::shared_ptr<ProviderCredentialBackend> instance =
        std::make_shared<WindowsCredentialBackend>();
    return instance;
}

std::mutex& backendMutex()
{
    static std::mutex mutex;
    return mutex;
}

} // namespace

bool ProviderCredentialStore::write(const QString& target,
                                    const QString& username,
                                    const QByteArray& secret)
{
    std::lock_guard<std::mutex> lock(backendMutex());
    return backend()->write(target, username, secret);
}

std::optional<QByteArray> ProviderCredentialStore::read(const QString& target)
{
    std::lock_guard<std::mutex> lock(backendMutex());
    return backend()->read(target);
}

bool ProviderCredentialStore::remove(const QString& target)
{
    std::lock_guard<std::mutex> lock(backendMutex());
    return backend()->remove(target);
}

bool ProviderCredentialStore::exists(const QString& target)
{
    std::lock_guard<std::mutex> lock(backendMutex());
    return backend()->exists(target);
}

void ProviderCredentialStore::setBackendForTesting(std::shared_ptr<ProviderCredentialBackend> testBackend)
{
    std::lock_guard<std::mutex> lock(backendMutex());
    backend() = testBackend ? std::move(testBackend) : std::make_shared<WindowsCredentialBackend>();
}

void ProviderCredentialStore::resetBackendForTesting()
{
    std::lock_guard<std::mutex> lock(backendMutex());
    backend() = std::make_shared<WindowsCredentialBackend>();
}

bool InMemoryCredentialBackend::write(const QString& target,
                                      const QString& username,
                                      const QByteArray& secret)
{
    Q_UNUSED(username)
    m_values[target] = secret;
    return true;
}

std::optional<QByteArray> InMemoryCredentialBackend::read(const QString& target)
{
    auto it = m_values.constFind(target);
    if (it == m_values.constEnd()) return std::nullopt;
    return it.value();
}

bool InMemoryCredentialBackend::remove(const QString& target)
{
    const bool existed = m_values.contains(target);
    m_values.remove(target);
    return existed;
}

bool InMemoryCredentialBackend::exists(const QString& target)
{
    return m_values.contains(target);
}
