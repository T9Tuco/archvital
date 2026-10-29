#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include "core/datastructs.h"

class DiagnosticsSection : public QWidget {
    Q_OBJECT
public:
    explicit DiagnosticsSection(QWidget* parent = nullptr);

private slots:
    void runDiagnostics();
    void onDiskUpdated(QVector<DiskPartitionData> partitions,
                       QVector<DiskDeviceData> devices);
    void onCpuUpdated(CpuData data);
    void onProcessesUpdated(QVector<ProcessData> procs);

private:
    void addFinding(const DiagnosticFinding& f);
    void clearFindings();
    void finishDiagnostics();

    QVBoxLayout* m_findingsLayout{nullptr};
    QWidget*     m_findingsContainer{nullptr};
    QLabel*      m_timestampLabel{nullptr};
    QPushButton* m_runBtn{nullptr};
    QPushButton* m_copyBtn{nullptr};

    // Latest data for diagnostics
    QVector<DiskPartitionData> m_diskParts;
    QVector<ProcessData>       m_procs;
    CpuData                    m_cpuData;
    QString                    m_reportText;
    QString                    m_runTimestamp;

    // systemctl and journalctl run through an async QProcess instead of
    // blocking, these hold their results until both are in, so findings
    // keep a stable order
    DiagnosticFinding m_systemdFinding;
    DiagnosticFinding m_journalFinding;
    int               m_pendingAsyncChecks{0};
};
