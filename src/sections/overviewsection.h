#pragma once

#include <QWidget>
#include <QLabel>
#include "core/datastructs.h"
#include "ui/metriccard.h"

class OverviewSection : public QWidget {
    Q_OBJECT
public:
    explicit OverviewSection(QWidget* parent = nullptr);

private slots:
    void onCpuUpdated(CpuData data);
    void onMemoryUpdated(MemoryData data);
    void onGpuUpdated(GpuData data);
    void onDiskUpdated(QVector<DiskPartitionData> partitions,
                       QVector<DiskDeviceData> devices);
    void onNetworkUpdated(QVector<NetworkInterfaceData> ifaces);
    void onSystemInfoUpdated(SystemInfo info);

private:
    // top info strip labels
    QLabel* m_hostnameLabel{nullptr};
    QLabel* m_kernelLabel{nullptr};
    QLabel* m_uptimeVal{nullptr};
    QLabel* m_loadVal{nullptr};

    // overview cards
    MetricCard* m_cpuCard{nullptr};
    MetricCard* m_ramCard{nullptr};
    MetricCard* m_gpuCard{nullptr};
    MetricCard* m_diskCard{nullptr};
    MetricCard* m_netCard{nullptr};
    MetricCard* m_uptimeCard{nullptr};
    MetricCard* m_loadCard{nullptr};
    MetricCard* m_sysCard{nullptr};
};
