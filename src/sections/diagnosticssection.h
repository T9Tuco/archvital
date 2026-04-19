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
};
