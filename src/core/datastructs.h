#pragma once

#include <QString>
#include <QVector>
#include <QMetaType>

struct CpuCoreData {
    int coreId{0};
    double usagePercent{0.0};
    long long freqKHz{0};
    double tempCelsius{-1.0};
};

struct CpuData {
    QString modelName;
    QString architecture;
    int physicalCores{0};
    int logicalCores{0};
    double overallUsage{0.0};
    double packageTemp{-1.0};
    double loadAvg1{0.0};
    double loadAvg5{0.0};
    double loadAvg15{0.0};
    QVector<CpuCoreData> cores;
    QVector<double> usageHistory;
};

struct MemoryData {
    long long totalKB{0};
    long long usedKB{0};
    long long freeKB{0};
    long long availableKB{0};
    long long buffersKB{0};
    long long cachedKB{0};
    long long swapTotalKB{0};
    long long swapUsedKB{0};
    double usagePercent{0.0};
    QVector<double> usageHistory;
};

struct GpuData {
    bool available{false};
    QString driverName;
    QString hwmonPath;
    double tempEdge{-1.0};
    double tempJunction{-1.0};
    double tempMemory{-1.0};
    int fanRpm{-1};
    int fanPercent{-1};
    double powerDraw{-1.0};
    double powerCap{-1.0};
    long long shaderClockMHz{-1};
    long long memClockMHz{-1};
    double voltage{-1.0};
    long long vramUsedMB{-1};
    long long vramTotalMB{-1};
};

struct DiskPartitionData {
    QString device;
    QString mountPoint;
    QString fsType;
    long long totalBytes{0};
    long long usedBytes{0};
    long long freeBytes{0};
    double usagePercent{0.0};
};

struct DiskDeviceData {
    QString device;
    long long readBytesPerSec{0};
    long long writeBytesPerSec{0};
    double tempCelsius{-1.0};
};

struct NetworkInterfaceData {
    QString name;
    bool isUp{false};
    QString ipAddress;
    long long downloadBytesPerSec{0};
    long long uploadBytesPerSec{0};
    long long totalDownloadBytes{0};
    long long totalUploadBytes{0};
    QVector<double> downloadHistory;
    QVector<double> uploadHistory;
};

struct ProcessData {
    int pid{0};
    int ppid{0};
    QString name;
    QString user;
    double cpuPercent{0.0};
    long long memBytes{0};
    double memPercent{0.0};
    QString status;
    QString startTime;
    bool isKernelThread{false};
};

struct SystemInfo {
    QString hostname;
    QString kernelVersion;
    long long uptimeSeconds{0};
    double loadAvg1{0.0};
    double loadAvg5{0.0};
    double loadAvg15{0.0};
};

struct DiagnosticFinding {
    enum class Severity { OK, Info, Warning, Error };
    Severity severity{Severity::OK};
    QString title;
    QString details;
};

Q_DECLARE_METATYPE(CpuData)
Q_DECLARE_METATYPE(MemoryData)
Q_DECLARE_METATYPE(GpuData)
Q_DECLARE_METATYPE(SystemInfo)
Q_DECLARE_METATYPE(QVector<DiskPartitionData>)
Q_DECLARE_METATYPE(QVector<DiskDeviceData>)
Q_DECLARE_METATYPE(QVector<NetworkInterfaceData>)
Q_DECLARE_METATYPE(QVector<ProcessData>)
