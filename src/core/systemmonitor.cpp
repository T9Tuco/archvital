#include "systemmonitor.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QStringList>
#include <QRegularExpression>

#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <pwd.h>
#include <time.h>
#include <signal.h>
#include <cerrno>
#include <algorithm>
#include <cstring>

// ── helpers ──────────────────────────────────────────────────────────────────

QString SystemMonitorWorker::readFile(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return QString::fromLocal8Bit(f.readAll()).trimmed();
}

long long SystemMonitorWorker::readFileLong(const QString& path, long long def) {
    const QString s = readFile(path);
    if (s.isEmpty()) return def;
    bool ok = false;
    long long v = s.toLongLong(&ok);
    return ok ? v : def;
}

long long SystemMonitorWorker::monoNs() {
    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1'000'000'000LL + ts.tv_nsec;
}

QString SystemMonitorWorker::uidToUsername(int uid) {
    if (m_uidCache.contains(uid)) return m_uidCache[uid];
    struct passwd pwd{};
    struct passwd* result = nullptr;
    char buf[512];
    getpwuid_r(static_cast<uid_t>(uid), &pwd, buf, sizeof(buf), &result);
    QString name = result ? QString::fromLocal8Bit(pwd.pw_name) : QString::number(uid);
    m_uidCache[uid] = name;
    return name;
}

// ── init ─────────────────────────────────────────────────────────────────────

SystemMonitorWorker::SystemMonitorWorker(QObject* parent) : QObject(parent) {
    m_clkTck = sysconf(_SC_CLK_TCK);
    if (m_clkTck <= 0) m_clkTck = 100;
}

void SystemMonitorWorker::initialize() {
    // Cache static CPU info
    QFile cpuinfo("/proc/cpuinfo");
    if (cpuinfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream s(&cpuinfo);
        QSet<int> physIds;
        while (!s.atEnd()) {
            const QString line = s.readLine();
            const auto idx = line.indexOf(':');
            if (idx < 0) continue;
            const QString key = line.left(idx).trimmed();
            const QString val = line.mid(idx + 1).trimmed();
            if (key == "model name" && m_cpuModel.isEmpty())
                m_cpuModel = val;
            if (key == "architecture" && m_cpuArch.isEmpty())
                m_cpuArch = val;
            if (key == "processor")
                m_logCores = val.toInt() + 1;
            if (key == "physical id")
                physIds.insert(val.toInt());
        }
        // If architecture not in cpuinfo, use uname
        if (m_cpuArch.isEmpty()) {
            struct utsname u{};
            uname(&u);
            m_cpuArch = QString::fromLocal8Bit(u.machine);
        }
        // Count physical cores via "cpu cores" field
        cpuinfo.seek(0);
        QTextStream s2(&cpuinfo);
        QHash<int, int> coreCounts;
        int currentPhysId = 0;
        while (!s2.atEnd()) {
            const QString line = s2.readLine();
            const auto idx = line.indexOf(':');
            if (idx < 0) continue;
            const QString key = line.left(idx).trimmed();
            const QString val = line.mid(idx + 1).trimmed();
            if (key == "physical id") currentPhysId = val.toInt();
            if (key == "cpu cores") coreCounts[currentPhysId] = val.toInt();
        }
        for (auto c : coreCounts) m_physCores += c;
        if (m_physCores == 0) m_physCores = m_logCores;
        m_cpuInfoCached = true;
    }
    detectGpu();
    refreshIPAddresses();
}

// ── GPU detection ─────────────────────────────────────────────────────────────

void SystemMonitorWorker::detectGpu() {
    m_gpuSearchDone = true;
    const QDir hwmonDir("/sys/class/hwmon");
    const auto entries = hwmonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& e : entries) {
        const QString base = "/sys/class/hwmon/" + e;
        const QString name = readFile(base + "/name");
        if (name == "amdgpu" || name == "nvidia" || name == "i915") {
            m_gpuHwmonPath = base;
            m_gpuDriver = name;
            break;
        }
    }
    // If hwmon not found, try nvidia-smi
    if (m_gpuHwmonPath.isEmpty()) {
        QProcess p;
        p.start("nvidia-smi", {"--query-gpu=name", "--format=csv,noheader"});
        p.waitForFinished(2000);
        if (p.exitCode() == 0 && !p.readAllStandardOutput().trimmed().isEmpty()) {
            m_gpuNvidiaSmi = true;
            m_gpuDriver = "nvidia-smi";
        }
    }
    // Locate DRM path for VRAM (amdgpu)
    if (m_gpuDriver == "amdgpu") {
        const QDir drmDir("/sys/class/drm");
        const auto cards = drmDir.entryList(QStringList{"card*"}, QDir::Dirs);
        for (const QString& c : cards) {
            if (!c.contains('-')) {
                const QString p = "/sys/class/drm/" + c + "/device";
                if (QFile::exists(p + "/mem_info_vram_total")) {
                    m_gpuDrmPath = p;
                    break;
                }
            }
        }
    }
}

// ── poll entry point ──────────────────────────────────────────────────────────

void SystemMonitorWorker::poll() {
    readCpuData();
    readMemoryData();
    readGpuData();
    readDiskData();
    readNetworkData();
    readProcessData();
    readSystemInfo();

    // Refresh IPs every ~30 s
    if (monoNs() - m_lastIPRefreshNs > 30'000'000'000LL) {
        refreshIPAddresses();
    }
}

// ── CPU ───────────────────────────────────────────────────────────────────────

SystemMonitorWorker::RawCpuStat
SystemMonitorWorker::parseCpuStatLine(const QStringList& p) {
    RawCpuStat s;
    if (p.size() > 1) s.user    = p[1].toLongLong();
    if (p.size() > 2) s.nice    = p[2].toLongLong();
    if (p.size() > 3) s.system  = p[3].toLongLong();
    if (p.size() > 4) s.idle    = p[4].toLongLong();
    if (p.size() > 5) s.iowait  = p[5].toLongLong();
    if (p.size() > 6) s.irq     = p[6].toLongLong();
    if (p.size() > 7) s.softirq = p[7].toLongLong();
    if (p.size() > 8) s.steal   = p[8].toLongLong();
    return s;
}

void SystemMonitorWorker::readCpuData() {
    QFile f("/proc/stat");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QVector<RawCpuStat> newStats;
    QTextStream ts(&f);
    while (!ts.atEnd()) {
        const QString line = ts.readLine();
        if (!line.startsWith("cpu")) break;
        const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        newStats.append(parseCpuStatLine(parts));
    }

    CpuData data;
    data.modelName    = m_cpuModel;
    data.architecture = m_cpuArch;
    data.physicalCores = m_physCores;
    data.logicalCores  = m_logCores > 0 ? m_logCores : (newStats.size() > 1 ? newStats.size() - 1 : 1);

    if (m_prevCpuStats.size() == newStats.size() && !newStats.isEmpty()) {
        // Index 0 = aggregate, 1+ = per-core
        for (int i = 0; i < newStats.size(); i++) {
            const auto& cur  = newStats[i];
            const auto& prev = m_prevCpuStats[i];
            long long dTotal = cur.total() - prev.total();
            long long dIdle  = cur.idleTotal() - prev.idleTotal();
            double usage = dTotal > 0 ? 100.0 * (dTotal - dIdle) / dTotal : 0.0;
            usage = std::clamp(usage, 0.0, 100.0);

            if (i == 0) {
                data.overallUsage = usage;
            } else {
                CpuCoreData cd;
                cd.coreId       = i - 1;
                cd.usagePercent = usage;
                const QString freqPath = QString(
                    "/sys/devices/system/cpu/cpu%1/cpufreq/scaling_cur_freq").arg(i - 1);
                cd.freqKHz = readFileLong(freqPath, 0);
                data.cores.append(cd);
            }
        }
    } else if (!newStats.isEmpty()) {
        int ncores = newStats.size() - 1;
        if (ncores < 1) ncores = 1;
        for (int i = 0; i < ncores; i++) {
            CpuCoreData cd;
            cd.coreId = i;
            data.cores.append(cd);
        }
    }

    // CPU temperatures via hwmon coretemp / k10temp
    {
        const QDir hwmon("/sys/class/hwmon");
        const auto entries = hwmon.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& e : entries) {
            const QString base = "/sys/class/hwmon/" + e;
            const QString name = readFile(base + "/name");
            if (name != "coretemp" && name != "k10temp") continue;
            // Read package temp
            for (int ti = 1; ti <= 20; ti++) {
                const QString lbl = readFile(
                    QString("%1/temp%2_label").arg(base).arg(ti));
                if (lbl.contains("Package", Qt::CaseInsensitive) ||
                    lbl.contains("Tdie",    Qt::CaseInsensitive) ||
                    lbl.contains("Tctl",    Qt::CaseInsensitive)) {
                    long long raw = readFileLong(
                        QString("%1/temp%2_input").arg(base).arg(ti), -1);
                    if (raw > 0) { data.packageTemp = raw / 1000.0; break; }
                }
            }
            // Per-core temps
            for (int ti = 1; ti <= 40; ti++) {
                const QString lbl = readFile(
                    QString("%1/temp%2_label").arg(base).arg(ti));
                if (!lbl.startsWith("Core ")) continue;
                bool ok = false;
                int cid = lbl.mid(5).toInt(&ok);
                if (!ok) continue;
                long long raw = readFileLong(
                    QString("%1/temp%2_input").arg(base).arg(ti), -1);
                if (raw > 0 && cid < data.cores.size())
                    data.cores[cid].tempCelsius = raw / 1000.0;
            }
            break;
        }
    }

    // Load averages
    {
        const QString la = readFile("/proc/loadavg");
        const QStringList lp = la.split(' ', Qt::SkipEmptyParts);
        if (lp.size() >= 3) {
            data.loadAvg1  = lp[0].toDouble();
            data.loadAvg5  = lp[1].toDouble();
            data.loadAvg15 = lp[2].toDouble();
        }
    }

    pushHistory(m_cpuHistory, data.overallUsage);
    data.usageHistory = m_cpuHistory;

    m_prevCpuStats = newStats;
    emit cpuDataReady(data);
}

// ── Memory ────────────────────────────────────────────────────────────────────

void SystemMonitorWorker::readMemoryData() {
    QFile f("/proc/meminfo");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    MemoryData data;
    QTextStream ts(&f);
    while (!ts.atEnd()) {
        const QString line = ts.readLine();
        const auto c = line.indexOf(':');
        if (c < 0) continue;
        const QString key = line.left(c).trimmed();
        bool ok = false;
        long long val = line.mid(c + 1).trimmed().split(' ').first().toLongLong(&ok);
        if (!ok) continue;
        if      (key == "MemTotal")     data.totalKB     = val;
        else if (key == "MemFree")      data.freeKB      = val;
        else if (key == "MemAvailable") data.availableKB = val;
        else if (key == "Buffers")      data.buffersKB   = val;
        else if (key == "Cached")       data.cachedKB    = val;
        else if (key == "SwapTotal")    data.swapTotalKB = val;
        else if (key == "SwapFree")     { data.swapUsedKB = data.swapTotalKB - val; }
    }
    long long buffCache = data.buffersKB + data.cachedKB;
    data.usedKB = data.totalKB - data.freeKB - buffCache;
    if (data.totalKB > 0)
        data.usagePercent = 100.0 * data.usedKB / data.totalKB;

    pushHistory(m_memHistory, data.usagePercent);
    data.usageHistory = m_memHistory;
    emit memoryDataReady(data);
}

// ── GPU ───────────────────────────────────────────────────────────────────────

void SystemMonitorWorker::readGpuData() {
    GpuData data;

    if (!m_gpuSearchDone) detectGpu();

    if (!m_gpuHwmonPath.isEmpty()) {
        data.available  = true;
        data.driverName = m_gpuDriver;
        data.hwmonPath  = m_gpuHwmonPath;

        auto rdTemp = [&](int idx) -> double {
            long long v = readFileLong(
                QString("%1/temp%2_input").arg(m_gpuHwmonPath).arg(idx), -1000);
            return v > -999 ? v / 1000.0 : -1.0;
        };

        if (m_gpuDriver == "amdgpu") {
            data.tempEdge     = rdTemp(1);
            data.tempJunction = rdTemp(2);
            data.tempMemory   = rdTemp(3);

            data.fanRpm = static_cast<int>(
                readFileLong(m_gpuHwmonPath + "/fan1_input", -1));
            long long pwm = readFileLong(m_gpuHwmonPath + "/pwm1", -1);
            if (pwm >= 0) data.fanPercent = static_cast<int>(pwm * 100 / 255);

            long long pAvg = readFileLong(m_gpuHwmonPath + "/power1_average", -1);
            long long pCap = readFileLong(m_gpuHwmonPath + "/power1_cap",     -1);
            if (pAvg >= 0) data.powerDraw = pAvg / 1'000'000.0;
            if (pCap >= 0) data.powerCap  = pCap / 1'000'000.0;

            long long f1 = readFileLong(m_gpuHwmonPath + "/freq1_input", -1);
            long long f2 = readFileLong(m_gpuHwmonPath + "/freq2_input", -1);
            if (f1 >= 0) data.shaderClockMHz = f1 / 1'000'000;
            if (f2 >= 0) data.memClockMHz    = f2 / 1'000'000;

            long long in0 = readFileLong(m_gpuHwmonPath + "/in0_input", -1);
            if (in0 >= 0) data.voltage = in0 / 1000.0;

            if (!m_gpuDrmPath.isEmpty()) {
                long long vt = readFileLong(m_gpuDrmPath + "/mem_info_vram_total", -1);
                long long vu = readFileLong(m_gpuDrmPath + "/mem_info_vram_used",  -1);
                if (vt > 0) data.vramTotalMB = vt / (1024 * 1024);
                if (vu >= 0) data.vramUsedMB  = vu / (1024 * 1024);
            }
        } else {
            // i915 / generic hwmon
            data.tempEdge = rdTemp(1);
        }

    } else if (m_gpuNvidiaSmi) {
        // Query nvidia-smi
        QProcess p;
        p.start("nvidia-smi", {
            "--query-gpu=temperature.gpu,fan.speed,power.draw,power.limit,"
            "clocks.sm,clocks.mem,memory.used,memory.total",
            "--format=csv,noheader,nounits"});
        p.waitForFinished(3000);
        if (p.exitCode() == 0) {
            const QStringList f = QString::fromLocal8Bit(p.readAllStandardOutput())
                .trimmed().split(',');
            data.available  = true;
            data.driverName = "nvidia-smi";
            if (f.size() >= 8) {
                data.tempEdge       = f[0].trimmed().toDouble();
                data.fanPercent     = f[1].trimmed().toInt();
                data.powerDraw      = f[2].trimmed().toDouble();
                data.powerCap       = f[3].trimmed().toDouble();
                data.shaderClockMHz = f[4].trimmed().toLongLong();
                data.memClockMHz    = f[5].trimmed().toLongLong();
                data.vramUsedMB     = f[6].trimmed().toLongLong();
                data.vramTotalMB    = f[7].trimmed().toLongLong();
            }
        }
    }

    emit gpuDataReady(data);
}

// ── Disks ─────────────────────────────────────────────────────────────────────

void SystemMonitorWorker::readDiskData() {
    static const QStringList SKIP_FS = {
        "tmpfs","devtmpfs","proc","sysfs","cgroup","cgroup2","devpts",
        "hugetlbfs","mqueue","pstore","bpf","tracefs","securityfs",
        "ramfs","overlay","autofs","debugfs","configfs","fusectl",
        "fuse.gvfsd-fuse","nsfs","efivarfs","squashfs","iso9660"
    };

    QVector<DiskPartitionData> partitions;

    QFile mounts("/proc/mounts");
    if (mounts.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream ts(&mounts);
        while (!ts.atEnd()) {
            const QStringList p = ts.readLine().split(' ', Qt::SkipEmptyParts);
            if (p.size() < 3) continue;
            if (SKIP_FS.contains(p[2])) continue;
            const QString mp = p[1];
            struct statvfs sv{};
            if (statvfs(mp.toLocal8Bit().constData(), &sv) != 0) continue;
            DiskPartitionData d;
            d.device     = p[0];
            d.mountPoint = mp;
            d.fsType     = p[2];
            d.totalBytes = static_cast<long long>(sv.f_blocks) * sv.f_frsize;
            d.freeBytes  = static_cast<long long>(sv.f_bfree)  * sv.f_frsize;
            d.usedBytes  = d.totalBytes - d.freeBytes;
            if (d.totalBytes > 0)
                d.usagePercent = 100.0 * d.usedBytes / d.totalBytes;
            if (d.totalBytes > 0) partitions.append(d);
        }
    }

    // I/O rates from /proc/diskstats
    QVector<DiskDeviceData> devices;
    QSet<QString> baseDevs;
    {
        QDir sb("/sys/block");
        const auto entries = sb.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto& e : entries) baseDevs.insert(e);
    }

    const long long now = monoNs();
    const double elapsedSec = m_prevDiskNs > 0 ?
        (now - m_prevDiskNs) / 1e9 : 0.0;

    QHash<QString, DiskIOState> newDiskIO;

    QFile dsf("/proc/diskstats");
    if (dsf.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream ts(&dsf);
        while (!ts.atEnd()) {
            const QStringList p = ts.readLine().split(' ', Qt::SkipEmptyParts);
            if (p.size() < 11) continue;
            const QString name = p[2];
            if (!baseDevs.contains(name)) continue;

            long long rs = p[5].toLongLong();
            long long ws = p[9].toLongLong();
            newDiskIO[name] = {rs, ws};

            DiskDeviceData dd;
            dd.device = name;
            if (elapsedSec > 0 && m_prevDiskIO.contains(name)) {
                const auto& prev = m_prevDiskIO[name];
                dd.readBytesPerSec  =
                    static_cast<long long>((rs - prev.readSectors)  * 512 / elapsedSec);
                dd.writeBytesPerSec =
                    static_cast<long long>((ws - prev.writeSectors) * 512 / elapsedSec);
                dd.readBytesPerSec  = std::max(0LL, dd.readBytesPerSec);
                dd.writeBytesPerSec = std::max(0LL, dd.writeBytesPerSec);
            }
            // NVMe temp via hwmon
            {
                const QDir hwmon("/sys/class/hwmon");
                for (const QString& e : hwmon.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                    const QString hBase = "/sys/class/hwmon/" + e;
                    const QString hName = readFile(hBase + "/name");
                    if (!hName.startsWith("nvme")) continue;
                    // Check if this hwmon is for our device
                    const QString devLink =
                        QFile::symLinkTarget("/sys/block/" + name + "/device");
                    if (QFile::symLinkTarget(hBase + "/device").contains(name) ||
                        QFile(hBase + "/device").exists()) {
                        long long t = readFileLong(hBase + "/temp1_input", -1);
                        if (t > 0) dd.tempCelsius = t / 1000.0;
                    }
                    break;
                }
                // simpler: just scan all nvme hwmons and attribute first to first nvme device
                if (dd.tempCelsius < 0 && name.startsWith("nvme")) {
                    for (const QString& e : hwmon.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                        const QString hBase = "/sys/class/hwmon/" + e;
                        if (readFile(hBase + "/name").startsWith("nvme")) {
                            long long t = readFileLong(hBase + "/temp1_input", -1);
                            if (t > 0) { dd.tempCelsius = t / 1000.0; break; }
                        }
                    }
                }
            }
            devices.append(dd);
        }
    }

    m_prevDiskIO  = newDiskIO;
    m_prevDiskNs  = now;

    emit diskDataReady(partitions, devices);
}

// ── Network ───────────────────────────────────────────────────────────────────

void SystemMonitorWorker::refreshIPAddresses() {
    m_lastIPRefreshNs = monoNs();
    m_ifaceIPs.clear();

    QProcess p;
    p.start("ip", {"addr", "show"});
    p.waitForFinished(3000);
    if (p.exitCode() != 0) return;

    const QString out = QString::fromLocal8Bit(p.readAllStandardOutput());
    QString currentIface;
    static const QRegularExpression ifaceRe(R"(^\d+:\s+(\S+):)");
    static const QRegularExpression inetRe(R"(^\s+inet\s+(\d+\.\d+\.\d+\.\d+))");

    for (const QString& line : out.split('\n')) {
        auto m = ifaceRe.match(line);
        if (m.hasMatch()) {
            currentIface = m.captured(1);
            if (currentIface.endsWith('@')) currentIface.chop(1);
            continue;
        }
        auto m2 = inetRe.match(line);
        if (m2.hasMatch() && !currentIface.isEmpty())
            m_ifaceIPs[currentIface] = m2.captured(1);
    }
}

void SystemMonitorWorker::readNetworkData() {
    QFile f("/proc/net/dev");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    const long long now = monoNs();
    const double elapsedSec = m_prevNetNs > 0 ?
        (now - m_prevNetNs) / 1e9 : 0.0;

    QHash<QString, NetState> newStats;
    QVector<NetworkInterfaceData> interfaces;

    QTextStream ts(&f);
    ts.readLine(); ts.readLine(); // skip 2 header lines
    while (!ts.atEnd()) {
        const QString line = ts.readLine().trimmed();
        const int colon = line.indexOf(':');
        if (colon < 0) continue;
        const QString name = line.left(colon).trimmed();
        if (name == "lo") continue;

        const QStringList parts = line.mid(colon + 1).split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 9) continue;

        long long rxBytes = parts[0].toLongLong();
        long long txBytes = parts[8].toLongLong();
        newStats[name] = {rxBytes, txBytes};

        NetworkInterfaceData nd;
        nd.name = name;
        nd.totalDownloadBytes = rxBytes;
        nd.totalUploadBytes   = txBytes;
        nd.ipAddress          = m_ifaceIPs.value(name, "");

        // Check link state
        nd.isUp = readFile("/sys/class/net/" + name + "/operstate") == "up";

        if (elapsedSec > 0 && m_prevNetStats.contains(name)) {
            const auto& prev = m_prevNetStats[name];
            nd.downloadBytesPerSec =
                std::max(0LL, static_cast<long long>((rxBytes - prev.rxBytes) / elapsedSec));
            nd.uploadBytesPerSec   =
                std::max(0LL, static_cast<long long>((txBytes - prev.txBytes) / elapsedSec));
        }

        // Histories
        if (!m_netDownHistory.contains(name))
            m_netDownHistory[name] = QVector<double>();
        if (!m_netUpHistory.contains(name))
            m_netUpHistory[name] = QVector<double>();

        pushHistory(m_netDownHistory[name], static_cast<double>(nd.downloadBytesPerSec));
        pushHistory(m_netUpHistory[name],   static_cast<double>(nd.uploadBytesPerSec));

        nd.downloadHistory = m_netDownHistory[name];
        nd.uploadHistory   = m_netUpHistory[name];

        interfaces.append(nd);
    }

    m_prevNetStats = newStats;
    m_prevNetNs    = now;

    emit networkDataReady(interfaces);
}

// ── Processes ─────────────────────────────────────────────────────────────────

void SystemMonitorWorker::readProcessData() {
    const long long now    = monoNs();
    const double elapsed   = m_prevPollNs > 0 ? (now - m_prevPollNs) / 1e9 : 0.0;

    long long memTotalKB = 0;
    {
        const QString mt = readFile("/proc/meminfo").section('\n', 0, 0);
        memTotalKB = mt.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts).value(1).toLongLong();
    }
    const long long pageSize = sysconf(_SC_PAGESIZE);

    QVector<ProcessData> result;
    QHash<int, ProcTimes> newTimes;

    const QDir procDir("/proc");
    const auto entries = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& e : entries) {
        bool ok = false;
        int pid = e.toInt(&ok);
        if (!ok) continue;

        const QString statPath = "/proc/" + e + "/stat";
        const QString statContent = readFile(statPath);
        if (statContent.isEmpty()) continue;

        int nameStart = statContent.indexOf('(');
        int nameEnd   = statContent.lastIndexOf(')');
        if (nameStart < 0 || nameEnd < 0) continue;

        int pidVal  = statContent.left(nameStart).trimmed().toInt();
        QString procName = statContent.mid(nameStart + 1, nameEnd - nameStart - 1);
        const QStringList rest = statContent.mid(nameEnd + 2).split(' ', Qt::SkipEmptyParts);
        if (rest.size() < 22) continue;

        const QString state = rest[0];
        int ppid    = rest[1].toInt();
        long long utime = rest[11].toLongLong();
        long long stime = rest[12].toLongLong();
        long long starttime = rest[19].toLongLong();
        long long rss       = rest[21].toLongLong(); // in pages

        long long totalTicks = utime + stime;
        newTimes[pid] = {totalTicks};

        double cpuPct = 0.0;
        if (elapsed > 0 && m_prevProcTimes.contains(pid)) {
            long long delta = totalTicks - m_prevProcTimes[pid].ticks;
            cpuPct = (delta / static_cast<double>(m_clkTck)) / elapsed * 100.0;
            cpuPct = std::clamp(cpuPct, 0.0, 100.0 * std::max(1, m_logCores));
        }

        long long memBytes = rss * pageSize;
        double memPct = memTotalKB > 0 ?
            100.0 * (rss * pageSize / 1024) / memTotalKB : 0.0;

        // UID -> username
        QString user;
        {
            const QString statusPath = "/proc/" + e + "/status";
            QFile sf(statusPath);
            if (sf.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream sts(&sf);
                while (!sts.atEnd()) {
                    const QString line = sts.readLine();
                    if (line.startsWith("Uid:")) {
                        int uid = line.split('\t', Qt::SkipEmptyParts).value(1).toInt();
                        user = uidToUsername(uid);
                        break;
                    }
                }
            }
        }

        // Kernel thread check: cmdline empty
        bool isKernel = false;
        {
            QFile cl("/proc/" + e + "/cmdline");
            if (cl.open(QIODevice::ReadOnly))
                isKernel = cl.read(1).isEmpty();
        }

        // Format start time
        long long boottime = 0;
        {
            QFile upf("/proc/uptime");
            if (upf.open(QIODevice::ReadOnly | QIODevice::Text)) {
                double upSec = QString::fromLocal8Bit(upf.readAll()).split(' ').first().toDouble();
                QDateTime boot = QDateTime::currentDateTime().addSecs(-static_cast<qint64>(upSec));
                double procStartSec = static_cast<double>(starttime) / m_clkTck;
                QDateTime procStart = boot.addMSecs(static_cast<qint64>(procStartSec * 1000));
                static_cast<void>(boottime);
                Q_UNUSED(boottime)

                ProcessData pd;
                pd.pid           = pidVal;
                pd.ppid          = ppid;
                pd.name          = procName;
                pd.user          = user;
                pd.cpuPercent    = cpuPct;
                pd.memBytes      = memBytes;
                pd.memPercent    = memPct;
                pd.status        = state;
                pd.startTime     = procStart.toString("hh:mm");
                pd.isKernelThread = isKernel;
                result.append(pd);
                continue;
            }
        }

        ProcessData pd;
        pd.pid            = pidVal;
        pd.ppid           = ppid;
        pd.name           = procName;
        pd.user           = user;
        pd.cpuPercent     = cpuPct;
        pd.memBytes       = memBytes;
        pd.memPercent     = memPct;
        pd.status         = state;
        pd.isKernelThread = isKernel;
        result.append(pd);
    }

    m_prevProcTimes = newTimes;
    m_prevPollNs    = now;

    emit processDataReady(result);
}

// ── System Info ───────────────────────────────────────────────────────────────

void SystemMonitorWorker::readSystemInfo() {
    SystemInfo info;

    // Hostname
    {
        char buf[256]{};
        gethostname(buf, sizeof(buf));
        info.hostname = QString::fromLocal8Bit(buf);
    }

    // Kernel version
    {
        struct utsname u{};
        uname(&u);
        info.kernelVersion = QString::fromLocal8Bit(u.release);
    }

    // Uptime
    {
        QFile f("/proc/uptime");
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const double up = QString::fromLocal8Bit(f.readAll())
                .split(' ').first().toDouble();
            info.uptimeSeconds = static_cast<long long>(up);
        }
    }

    // Load averages
    {
        const QString la = readFile("/proc/loadavg");
        const QStringList p = la.split(' ', Qt::SkipEmptyParts);
        if (p.size() >= 3) {
            info.loadAvg1  = p[0].toDouble();
            info.loadAvg5  = p[1].toDouble();
            info.loadAvg15 = p[2].toDouble();
        }
    }

    emit systemInfoReady(info);
}

// ── SystemMonitor (singleton + thread management) ─────────────────────────────

SystemMonitor& SystemMonitor::instance() {
    static SystemMonitor inst;
    return inst;
}

SystemMonitor::SystemMonitor(QObject* parent) : QObject(parent) {
    qRegisterMetaType<CpuData>("CpuData");
    qRegisterMetaType<MemoryData>("MemoryData");
    qRegisterMetaType<GpuData>("GpuData");
    qRegisterMetaType<SystemInfo>("SystemInfo");
    qRegisterMetaType<QVector<DiskPartitionData>>("QVector<DiskPartitionData>");
    qRegisterMetaType<QVector<DiskDeviceData>>("QVector<DiskDeviceData>");
    qRegisterMetaType<QVector<NetworkInterfaceData>>("QVector<NetworkInterfaceData>");
    qRegisterMetaType<QVector<ProcessData>>("QVector<ProcessData>");

    m_worker = new SystemMonitorWorker();
    m_worker->moveToThread(&m_thread);

    connect(&m_thread, &QThread::started,
            m_worker, &SystemMonitorWorker::initialize);
    connect(this, &SystemMonitor::triggerPoll,
            m_worker, &SystemMonitorWorker::poll,
            Qt::QueuedConnection);

    // Re-emit worker signals as our own
    connect(m_worker, &SystemMonitorWorker::cpuDataReady,
            this, &SystemMonitor::cpuDataUpdated,    Qt::QueuedConnection);
    connect(m_worker, &SystemMonitorWorker::memoryDataReady,
            this, &SystemMonitor::memoryDataUpdated, Qt::QueuedConnection);
    connect(m_worker, &SystemMonitorWorker::gpuDataReady,
            this, &SystemMonitor::gpuDataUpdated,    Qt::QueuedConnection);
    connect(m_worker, &SystemMonitorWorker::diskDataReady,
            this, &SystemMonitor::diskDataUpdated,   Qt::QueuedConnection);
    connect(m_worker, &SystemMonitorWorker::networkDataReady,
            this, &SystemMonitor::networkDataUpdated, Qt::QueuedConnection);
    connect(m_worker, &SystemMonitorWorker::processDataReady,
            this, &SystemMonitor::processDataUpdated, Qt::QueuedConnection);
    connect(m_worker, &SystemMonitorWorker::systemInfoReady,
            this, &SystemMonitor::systemInfoUpdated,  Qt::QueuedConnection);
}

SystemMonitor::~SystemMonitor() {
    stop();
    m_thread.quit();
    m_thread.wait();
    delete m_worker;
}

void SystemMonitor::start(int intervalMs) {
    m_intervalMs = intervalMs;
    m_thread.start();

    m_timer = new QTimer(this);
    m_timer->setInterval(intervalMs);
    connect(m_timer, &QTimer::timeout, this, [this]{ emit triggerPoll(); });
    // First poll after short delay to let initialize() run
    QTimer::singleShot(500, this, [this]{ emit triggerPoll(); });
    m_timer->start();
}

void SystemMonitor::stop() {
    if (m_timer) { m_timer->stop(); m_timer->deleteLater(); m_timer = nullptr; }
}
