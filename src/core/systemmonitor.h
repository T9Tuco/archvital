#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QHash>
#include "datastructs.h"

class SystemMonitorWorker : public QObject {
    Q_OBJECT
public:
    explicit SystemMonitorWorker(QObject* parent = nullptr);

public slots:
    void initialize();
    void poll();

signals:
    void cpuDataReady(CpuData data);
    void memoryDataReady(MemoryData data);
    void gpuDataReady(GpuData data);
    void diskDataReady(QVector<DiskPartitionData> partitions, QVector<DiskDeviceData> devices);
    void networkDataReady(QVector<NetworkInterfaceData> interfaces);
    void processDataReady(QVector<ProcessData> processes);
    void systemInfoReady(SystemInfo info);

private:
    struct RawCpuStat {
        long long user{0}, nice{0}, system{0}, idle{0},
                  iowait{0}, irq{0}, softirq{0}, steal{0};
        long long total() const {
            return user + nice + system + idle + iowait + irq + softirq + steal;
        }
        long long idleTotal() const { return idle + iowait; }
    };

    struct ProcTimes {
        long long ticks{0};
    };

    struct DiskIOState {
        long long readSectors{0};
        long long writeSectors{0};
    };

    struct NetState {
        long long rxBytes{0};
        long long txBytes{0};
    };

    // CPU state
    QVector<RawCpuStat> m_prevCpuStats;

    // Process state
    QHash<int, ProcTimes> m_prevProcTimes;
    long long m_prevPollNs{0};
    long long m_clkTck{100};

    // Disk I/O state
    QHash<QString, DiskIOState> m_prevDiskIO;
    long long m_prevDiskNs{0};

    // Network state
    QHash<QString, NetState> m_prevNetStats;
    long long m_prevNetNs{0};

    // History (managed in worker, included in emitted structs)
    QVector<double> m_cpuHistory;
    QVector<double> m_memHistory;
    QHash<QString, QVector<double>> m_netDownHistory;
    QHash<QString, QVector<double>> m_netUpHistory;

    // Cached static info
    bool m_cpuInfoCached{false};
    QString m_cpuModel;
    QString m_cpuArch;
    int m_physCores{0};
    int m_logCores{0};

    // GPU detection
    bool m_gpuSearchDone{false};
    QString m_gpuHwmonPath;
    QString m_gpuDriver;
    QString m_gpuDrmPath;
    bool m_gpuNvidiaSmi{false};

    // IP address cache
    QHash<QString, QString> m_ifaceIPs;
    long long m_lastIPRefreshNs{0};

    // UID -> username cache
    QHash<int, QString> m_uidCache;

    void readCpuData();
    void readMemoryData();
    void readGpuData();
    void readDiskData();
    void readNetworkData();
    void readProcessData();
    void readSystemInfo();

    RawCpuStat parseCpuStatLine(const QStringList& parts);
    void detectGpu();
    void refreshIPAddresses();
    QString uidToUsername(int uid);
    QString readFile(const QString& path);
    long long readFileLong(const QString& path, long long def = -1);
    long long monoNs();

    static constexpr int HISTORY_LEN = 60;
    static void pushHistory(QVector<double>& hist, double val) {
        hist.append(val);
        if (hist.size() > HISTORY_LEN) hist.removeFirst();
    }
};

class SystemMonitor : public QObject {
    Q_OBJECT
public:
    static SystemMonitor& instance();

    void start(int intervalMs = 2000);
    void stop();
    int interval() const { return m_intervalMs; }

signals:
    void cpuDataUpdated(CpuData data);
    void memoryDataUpdated(MemoryData data);
    void gpuDataUpdated(GpuData data);
    void diskDataUpdated(QVector<DiskPartitionData> partitions, QVector<DiskDeviceData> devices);
    void networkDataUpdated(QVector<NetworkInterfaceData> interfaces);
    void processDataUpdated(QVector<ProcessData> processes);
    void systemInfoUpdated(SystemInfo info);

    void triggerPoll();

private:
    explicit SystemMonitor(QObject* parent = nullptr);
    ~SystemMonitor() override;

    QThread m_thread;
    SystemMonitorWorker* m_worker{nullptr};
    QTimer* m_timer{nullptr};
    int m_intervalMs{2000};
};
