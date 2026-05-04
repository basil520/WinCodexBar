#pragma once

#include <QObject>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QStringList>

/**
 * @brief 批量 UI 更新控制器
 *
 * 解决信号风暴问题：worker thread 完成刷新后不立即发射 UI 信号，
 * 而是标记 dirty，由主线程在合适的时机批量发射。
 *
 * 所有公共方法均在主线程调用（通过 Qt::QueuedConnection 的 invokeMethod 回调）。
 */
class BatchUpdateController : public QObject {
    Q_OBJECT
public:
    explicit BatchUpdateController(QObject* parent = nullptr);

    // 生命周期
    void beginBatch();   // 开始批量周期
    void endBatch();     // 结束批量周期，触发最终刷新

    // 标记数据变化
    void markDirty(const QString& providerId);

    // 查询状态
    bool isBatchInProgress() const { return m_batchInProgress; }

signals:
    // 批量更新就绪，参数为所有变化的 provider ID
    void batchUpdateReady(const QStringList& providerIds);
    // 批量周期结束
    void batchFinished();

private slots:
    void onDebounceTimeout();

private:
    void emitBatch();

    QSet<QString> m_dirtyProviders;
    bool m_batchInProgress = false;
    QTimer* m_debounceTimer = nullptr;
    static constexpr int DEBOUNCE_INTERVAL_MS = 50;
};