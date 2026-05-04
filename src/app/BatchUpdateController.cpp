#include "BatchUpdateController.h"

#include <QDebug>

BatchUpdateController::BatchUpdateController(QObject* parent)
    : QObject(parent)
{
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(DEBOUNCE_INTERVAL_MS);
    connect(m_debounceTimer, &QTimer::timeout, this, &BatchUpdateController::onDebounceTimeout);
}

void BatchUpdateController::beginBatch()
{
    m_batchInProgress = true;
    m_dirtyProviders.clear();
}

void BatchUpdateController::endBatch()
{
    m_batchInProgress = false;

    if (m_dirtyProviders.isEmpty()) {
        // 没有待处理的 dirty，直接发射结束信号
        emit batchFinished();
        return;
    }

    // 强制触发防抖定时器（如果还有 dirty providers）
    if (!m_debounceTimer->isActive()) {
        m_debounceTimer->start();
    }
    // 否则等定时器自然触发，会在 onDebounceTimeout 中发射 batchFinished
}

void BatchUpdateController::markDirty(const QString& providerId)
{
    m_dirtyProviders.insert(providerId);

    if (m_batchInProgress) {
        // 批量模式：启动/重启防抖定时器
        m_debounceTimer->start();
    } else {
        // 单刷模式：立即发射
        emit batchUpdateReady({providerId});
    }
}

void BatchUpdateController::onDebounceTimeout()
{
    if (m_dirtyProviders.isEmpty()) {
        return;
    }

    emitBatch();

    // 如果批量已结束且 dirty 已清空，发射 batchFinished
    if (!m_batchInProgress) {
        emit batchFinished();
    }
}

void BatchUpdateController::emitBatch()
{
    if (m_dirtyProviders.isEmpty()) {
        return;
    }

    QStringList dirty = m_dirtyProviders.values();
    m_dirtyProviders.clear();
    emit batchUpdateReady(dirty);
}