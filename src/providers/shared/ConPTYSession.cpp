#include "ConPTYSession.h"

#include <QDebug>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QMutexLocker>
#include <QStringList>
#include <QThread>

namespace {

using CreatePseudoConsoleFn = HRESULT (WINAPI *)(COORD, HANDLE, HANDLE, DWORD, HPCON*);
using ClosePseudoConsoleFn = void (WINAPI *)(HPCON);

CreatePseudoConsoleFn createPseudoConsoleProc() {
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) return nullptr;
    return reinterpret_cast<CreatePseudoConsoleFn>(GetProcAddress(hKernel32, "CreatePseudoConsole"));
}

ClosePseudoConsoleFn closePseudoConsoleProc() {
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) return nullptr;
    return reinterpret_cast<ClosePseudoConsoleFn>(GetProcAddress(hKernel32, "ClosePseudoConsole"));
}

void closeHandle(HANDLE& handle) {
    if (handle && handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
    }
    handle = nullptr;
}

QByteArray environmentBlock(const QProcessEnvironment& env) {
    if (env.isEmpty()) return {};

    QStringList entries;
    const auto keys = env.keys();
    for (const auto& key : keys) {
        entries.append(key + "=" + env.value(key));
    }
    entries.sort(Qt::CaseInsensitive);

    QByteArray block;
    for (const auto& entry : entries) {
        const std::wstring wide = entry.toStdWString();
        block.append(reinterpret_cast<const char*>(wide.c_str()),
                     static_cast<int>((wide.size() + 1) * sizeof(wchar_t)));
    }
    const wchar_t terminator = L'\0';
    block.append(reinterpret_cast<const char*>(&terminator), sizeof(wchar_t));
    return block;
}

} // namespace

ConPTYSession::ConPTYSession(QObject* parent) : QObject(parent) {}

ConPTYSession::~ConPTYSession() { terminate(); }

QString ConPTYSession::quoteArg(const QString& arg) {
    if (arg.isEmpty()) return "\"\"";
    if (!arg.contains(QRegularExpression("[\\s\"]"))) return arg;

    QString out = "\"";
    int backslashes = 0;
    for (QChar ch : arg) {
        if (ch == '\\') {
            backslashes++;
        } else if (ch == '"') {
            out += QString(backslashes * 2 + 1, '\\');
            out += '"';
            backslashes = 0;
        } else {
            if (backslashes > 0) {
                out += QString(backslashes, '\\');
                backslashes = 0;
            }
            out += ch;
        }
    }
    if (backslashes > 0) out += QString(backslashes * 2, '\\');
    out += '"';
    return out;
}

bool ConPTYSession::start(const QString& command,
                          const QStringList& args,
                          const QProcessEnvironment& env)
{
    terminate();

    if (isConPtyAvailable()) {
        m_useFallback = false;
        if (startWithConPty(command, args, env)) {
            return true;
        }
        terminate();
    }

    m_useFallback = true;
    return startWithQProcess(command, args, env);
}

bool ConPTYSession::startWithQProcess(const QString& command,
                                       const QStringList& args,
                                       const QProcessEnvironment& env)
{
    m_useFallback = true;
    {
        QMutexLocker locker(&m_bufferMutex);
        m_buffer.clear();
    }

    m_process = new QProcess(this);
    m_process->setProcessEnvironment(env.isEmpty() ? QProcessEnvironment::systemEnvironment() : env);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        QByteArray data = m_process->readAllStandardOutput();
        if (!data.isEmpty()) {
            appendOutput(data);
        }
    });

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus) {
        m_running = false;
        emit processFinished(exitCode);
    });

    QStringList fullArgs = args;
    m_process->start(command, fullArgs);

    if (!m_process->waitForStarted(3000)) {
        delete m_process;
        m_process = nullptr;
        return false;
    }

    m_running = true;
    return true;
}

bool ConPTYSession::write(const QByteArray& data)
{
    if (m_useFallback && m_process) {
        return m_process->write(data) == data.size();
    }

    if (!m_running || !m_hInput || data.isEmpty()) return false;
    DWORD written = 0;
    if (!WriteFile(m_hInput, data.constData(), static_cast<DWORD>(data.size()), &written, nullptr)) {
        return false;
    }
    return written == static_cast<DWORD>(data.size());
}

QByteArray ConPTYSession::readOutput(int timeoutMs)
{
    QDateTime deadline = QDateTime::currentDateTimeUtc().addMSecs(timeoutMs);
    while (QDateTime::currentDateTimeUtc() < deadline) {
        if (m_useFallback && m_process) {
            QCoreApplication::processEvents();
        }
        {
            QMutexLocker locker(&m_bufferMutex);
            if (!m_buffer.isEmpty()) {
                QByteArray out = m_buffer;
                m_buffer.clear();
                return out;
            }
        }
        QThread::msleep(10);
    }

    QMutexLocker locker(&m_bufferMutex);
    QByteArray out = m_buffer;
    m_buffer.clear();
    return out;
}

bool ConPTYSession::waitForPattern(const QRegularExpression& pattern, int timeoutMs)
{
    QDateTime deadline = QDateTime::currentDateTimeUtc().addMSecs(timeoutMs);
    while (QDateTime::currentDateTimeUtc() < deadline) {
        if (m_useFallback && m_process) {
            QCoreApplication::processEvents();
        }
        {
            QMutexLocker locker(&m_bufferMutex);
            QString text = QString::fromLocal8Bit(m_buffer);
            if (pattern.match(text).hasMatch()) return true;
        }
        if (!m_running && !m_useFallback) break;
        QThread::msleep(25);
    }
    return false;
}

void ConPTYSession::terminate()
{
    bool wasRunning = m_running;
    m_running = false;

    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(2000);
        delete m_process;
        m_process = nullptr;
    }

    if (m_hInput) {
        closeHandle(m_hInput);
    }

    if (m_hProcess) {
        TerminateProcess(m_hProcess, 0);
        WaitForSingleObject(m_hProcess, 1000);
    }

    if (m_ptyHandle) {
        if (auto closePseudoConsole = closePseudoConsoleProc()) {
            closePseudoConsole(reinterpret_cast<HPCON>(m_ptyHandle));
        }
        m_ptyHandle = nullptr;
    }

    if (m_readerThread.joinable()) {
        CancelSynchronousIo(reinterpret_cast<HANDLE>(m_readerThread.native_handle()));
        m_readerThread.join();
    }

    closeHandle(m_hOutput);
    closeHandle(m_hThread);
    closeHandle(m_hProcess);
    if (wasRunning) emit processFinished(0);
}

bool ConPTYSession::isRunning() const
{
    if (m_useFallback && m_process) {
        return m_process->state() == QProcess::Running;
    }
    return m_running;
}

bool ConPTYSession::isConPtyAvailable()
{
    return createPseudoConsoleProc() != nullptr && closePseudoConsoleProc() != nullptr;
}

void ConPTYSession::readerLoop()
{
    char buffer[4096];
    while (m_running && m_hOutput) {
        DWORD bytesAvail = 0;
        if (!PeekNamedPipe(m_hOutput, nullptr, 0, nullptr, &bytesAvail, nullptr)) {
            DWORD err = GetLastError();
            if (err != ERROR_BROKEN_PIPE && err != ERROR_HANDLE_EOF) {
                qDebug() << "[ConPTY] PeekNamedPipe failed, error=" << err;
            }
            break;
        }
        if (bytesAvail > 0) {
            DWORD toRead = qMin(bytesAvail, static_cast<DWORD>(sizeof(buffer)));
            DWORD read = 0;
            BOOL ok = ReadFile(m_hOutput, buffer, toRead, &read, nullptr);
            if (!ok || read == 0) break;
            appendOutput(QByteArray(buffer, static_cast<int>(read)));
        } else {
            QThread::msleep(10);
        }
    }

    DWORD exitCode = 0;
    if (m_hProcess && GetExitCodeProcess(m_hProcess, &exitCode) && exitCode == STILL_ACTIVE) {
        WaitForSingleObject(m_hProcess, 1000);
        GetExitCodeProcess(m_hProcess, &exitCode);
    }
    m_running = false;
    emit processFinished(static_cast<int>(exitCode == STILL_ACTIVE ? 0 : exitCode));
}

void ConPTYSession::appendOutput(const QByteArray& data)
{
    {
        QMutexLocker locker(&m_bufferMutex);
        m_buffer.append(data);
    }
    emit outputReceived(data);
}

bool ConPTYSession::startWithConPty(const QString& command,
                                     const QStringList& args,
                                     const QProcessEnvironment& env)
{
    m_useFallback = false;
    auto createPseudoConsole = createPseudoConsoleProc();
    if (!createPseudoConsole) return false;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = TRUE;

    HANDLE ptyInputRead = nullptr;
    HANDLE ptyOutputWrite = nullptr;
    if (!CreatePipe(&ptyInputRead, &m_hInput, &sa, 0)) return false;
    if (!CreatePipe(&m_hOutput, &ptyOutputWrite, &sa, 0)) {
        closeHandle(ptyInputRead);
        closeHandle(m_hInput);
        return false;
    }

    SetHandleInformation(m_hInput, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(m_hOutput, HANDLE_FLAG_INHERIT, 0);

    HPCON hpc = nullptr;
    COORD size = {120, 30};
    HRESULT hr = createPseudoConsole(size, ptyInputRead, ptyOutputWrite, 0, &hpc);
    if (FAILED(hr)) {
        closeHandle(ptyInputRead);
        closeHandle(ptyOutputWrite);
        closeHandle(m_hInput);
        closeHandle(m_hOutput);
        return false;
    }
    m_ptyHandle = hpc;

    STARTUPINFOEXW siEx;
    ZeroMemory(&siEx, sizeof(siEx));
    siEx.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    siEx.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    siEx.StartupInfo.hStdInput = nullptr;
    siEx.StartupInfo.hStdOutput = nullptr;
    siEx.StartupInfo.hStdError = nullptr;

    SIZE_T attrListSize = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attrListSize);
    siEx.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
        HeapAlloc(GetProcessHeap(), 0, attrListSize));
    if (!siEx.lpAttributeList ||
        !InitializeProcThreadAttributeList(siEx.lpAttributeList, 1, 0, &attrListSize) ||
        !UpdateProcThreadAttribute(siEx.lpAttributeList,
                                   0,
                                   PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                                   hpc,
                                   sizeof(hpc),
                                   nullptr,
                                   nullptr)) {
        if (siEx.lpAttributeList) HeapFree(GetProcessHeap(), 0, siEx.lpAttributeList);
        closeHandle(ptyInputRead);
        closeHandle(ptyOutputWrite);
        terminate();
        return false;
    }

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    QString cmdLine = quoteArg(command);
    for (const auto& arg : args) {
        cmdLine += " " + quoteArg(arg);
    }
    std::wstring wideCmd = cmdLine.toStdWString();
    QByteArray envBlock = environmentBlock(env);

    DWORD createFlags = EXTENDED_STARTUPINFO_PRESENT;
    if (!envBlock.isEmpty()) createFlags |= CREATE_UNICODE_ENVIRONMENT;

    BOOL ok = CreateProcessW(
        nullptr,
        wideCmd.data(),
        nullptr,
        nullptr,
        FALSE,
        createFlags,
        envBlock.isEmpty() ? nullptr : envBlock.data(),
        nullptr,
        &siEx.StartupInfo,
        &pi);

    DeleteProcThreadAttributeList(siEx.lpAttributeList);
    HeapFree(GetProcessHeap(), 0, siEx.lpAttributeList);
    closeHandle(ptyInputRead);
    closeHandle(ptyOutputWrite);

    if (!ok) {
        terminate();
        return false;
    }

    m_hProcess = pi.hProcess;
    m_hThread = pi.hThread;
    m_running = true;
    {
        QMutexLocker locker(&m_bufferMutex);
        m_buffer.clear();
    }

    m_readerThread = std::thread([this]() { readerLoop(); });
    return true;
}
