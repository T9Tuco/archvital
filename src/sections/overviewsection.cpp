#include "overviewsection.h"

#include <QScrollArea>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include "core/systemmonitor.h"
#include "ui/stylesheet.h"

// a few tiny format helpers so the slots stay clean

static QString fmtBytes(long long b) {
    if (b < 1024)              return QString("%1 B").arg(b);
    if (b < 1024*1024)         return QString("%1 KB").arg(b / 1024);
    if (b < 1024LL*1024*1024)  return QString("%1 MB").arg(b / 1024 / 1024);
    return QString("%1 GB").arg(b / 1024 / 1024 / 1024);
}

static QString fmtUptime(long long sec) {
    long long d = sec / 86400, h = (sec % 86400) / 3600, m = (sec % 3600) / 60;
    if (d > 0) return QString("%1d %2h").arg(d).arg(h);
    if (h > 0) return QString("%1h %2m").arg(h).arg(m);
    return QString("%1m").arg(m);
}

static QColor usageColor(double pct) {
    if (pct >= 85) return Style::errorColor();
    if (pct >= 65) return Style::warningColor();
    return Style::accentColor();
}

// build the overview page

OverviewSection::OverviewSection(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // simple top bar, nothing dramatic
    auto* titleBar = new QWidget(this);
    titleBar->setFixedHeight(44);
    titleBar->setStyleSheet("background: transparent;");
    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(24, 0, 24, 0);

    auto* title = new QLabel("Overview", titleBar);
    title->setStyleSheet(
        "color: #c0c0c0; font-size: 14px; font-weight: 600; "
        "letter-spacing: 0.3px; background: transparent;");
    titleLayout->addWidget(title);
    titleLayout->addStretch();
    outer->addWidget(titleBar);

    auto* divider = new QWidget(this);
    divider->setFixedHeight(1);
    divider->setStyleSheet("background: #1a1a1a;");
    outer->addWidget(divider);

    // scroll area in case cards start stacking up on smaller windows
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);

    auto* inner = new QWidget();
    inner->setStyleSheet("background: transparent;");
    scroll->setWidget(inner);

    auto* vbox = new QVBoxLayout(inner);
    vbox->setContentsMargins(14, 10, 14, 14);
    vbox->setSpacing(6);

    // top info strip for the machine basics
    auto* infoBar = new QWidget(inner);
    infoBar->setStyleSheet(
        "background: #131313; border: 1px solid #1e1e1e; border-radius: 7px;");
    auto* infoLayout = new QHBoxLayout(infoBar);
    infoLayout->setContentsMargins(10, 7, 10, 7);
    infoLayout->setSpacing(14);

    auto makeInfoPair = [&](const QString& lbl) -> QLabel* {
        auto* f = new QLabel(lbl + ":", infoBar);
        f->setStyleSheet("color: #444; font-size: 10px; background: transparent;");
        infoLayout->addWidget(f);
        auto* v = new QLabel("—", infoBar);
        v->setStyleSheet(
            "color: #888; font-size: 11px; "
            "font-family: 'JetBrains Mono','Fira Mono',monospace; background: transparent;");
        infoLayout->addWidget(v);
        return v;
    };

    m_hostnameLabel = makeInfoPair("Host");
    m_kernelLabel   = makeInfoPair("Kernel");
    m_uptimeVal     = makeInfoPair("Uptime");
    m_loadVal       = makeInfoPair("Load");
    infoLayout->addStretch();
    vbox->addWidget(infoBar);

    // first row: the usual heavy hitters
    auto* row1 = new QHBoxLayout();
    row1->setSpacing(6);
    m_cpuCard  = new MetricCard("CPU",    inner);
    m_ramCard  = new MetricCard("Memory", inner);
    m_gpuCard  = new MetricCard("GPU",    inner);
    m_diskCard = new MetricCard("Disk /", inner);
    row1->addWidget(m_cpuCard);
    row1->addWidget(m_ramCard);
    row1->addWidget(m_gpuCard);
    row1->addWidget(m_diskCard);
    vbox->addLayout(row1);

    // second row fills in the rest
    auto* row2 = new QHBoxLayout();
    row2->setSpacing(6);
    m_netCard     = new MetricCard("Network",  inner);
    m_uptimeCard  = new MetricCard("Uptime",   inner);
    m_loadCard    = new MetricCard("Load Avg", inner);
    m_sysCard     = new MetricCard("System",   inner);
    row2->addWidget(m_netCard);
    row2->addWidget(m_uptimeCard);
    row2->addWidget(m_loadCard);
    row2->addWidget(m_sysCard);
    vbox->addLayout(row2);

    vbox->addStretch();

    // live data wiring
    auto& mon = SystemMonitor::instance();
    connect(&mon, &SystemMonitor::cpuDataUpdated,     this, &OverviewSection::onCpuUpdated);
    connect(&mon, &SystemMonitor::memoryDataUpdated,  this, &OverviewSection::onMemoryUpdated);
    connect(&mon, &SystemMonitor::gpuDataUpdated,     this, &OverviewSection::onGpuUpdated);
    connect(&mon, &SystemMonitor::diskDataUpdated,    this, &OverviewSection::onDiskUpdated);
    connect(&mon, &SystemMonitor::networkDataUpdated, this, &OverviewSection::onNetworkUpdated);
    connect(&mon, &SystemMonitor::systemInfoUpdated,  this, &OverviewSection::onSystemInfoUpdated);
}

// ui updates land here

void OverviewSection::onCpuUpdated(CpuData d) {
    const double pct = d.overallUsage;
    m_cpuCard->setValue(QString::number(pct, 'f', 1), "%");
    m_cpuCard->setAccentColor(usageColor(pct));
    m_cpuCard->updateHistory(d.usageHistory);
    if (d.packageTemp > 0)
        m_cpuCard->setSubtext(QString("%1 C   %2C/%3T")
            .arg(d.packageTemp, 0, 'f', 0)
            .arg(d.physicalCores)
            .arg(d.logicalCores));
    else if (d.logicalCores > 0)
        m_cpuCard->setSubtext(QString("%1C / %2T").arg(d.physicalCores).arg(d.logicalCores));
}

void OverviewSection::onMemoryUpdated(MemoryData d) {
    const double pct = d.usagePercent;
    const long long usedMB = d.usedKB  / 1024;
    const long long totMB  = d.totalKB / 1024;
    m_ramCard->setValue(
        totMB >= 1024 ? QString::number(usedMB / 1024.0, 'f', 1)
                      : QString::number(usedMB),
        totMB >= 1024 ? QString("/ %1 GB").arg(totMB / 1024)
                      : QString("/ %1 MB").arg(totMB));
    m_ramCard->setAccentColor(usageColor(pct));
    m_ramCard->updateHistory(d.usageHistory);
    m_ramCard->setSubtext(QString::number(pct, 'f', 1) + "% used");
}

void OverviewSection::onGpuUpdated(GpuData d) {
    if (!d.available) {
        m_gpuCard->setValue("N/A");
        m_gpuCard->setSubtext("no driver");
        return;
    }
    if (d.tempEdge >= 0) {
        m_gpuCard->setValue(QString::number(d.tempEdge, 'f', 0), "C");
        m_gpuCard->setAccentColor(usageColor(d.tempEdge / 105.0 * 100));
        QString sub = d.driverName;
        if (d.fanPercent >= 0) sub += QString("   fan %1%").arg(d.fanPercent);
        m_gpuCard->setSubtext(sub);
    }
}

void OverviewSection::onDiskUpdated(QVector<DiskPartitionData> parts, QVector<DiskDeviceData>) {
    for (const auto& p : parts) {
        if (p.mountPoint == "/") {
            const double pct = p.usagePercent;
            m_diskCard->setValue(
                QString::number(p.usedBytes / 1024.0 / 1024 / 1024, 'f', 0),
                QString("/ %1 GB").arg(p.totalBytes / 1024 / 1024 / 1024));
            m_diskCard->setAccentColor(usageColor(pct));
            m_diskCard->setSubtext(QString::number(pct, 'f', 1) + "% used");
            return;
        }
    }
}

void OverviewSection::onNetworkUpdated(QVector<NetworkInterfaceData> ifaces) {
    long long downTotal = 0, upTotal = 0;
    for (const auto& i : ifaces) { downTotal += i.downloadBytesPerSec; upTotal += i.uploadBytesPerSec; }
    m_netCard->setValue(fmtBytes(downTotal), "/s");
    m_netCard->setSubtext(QString("up  %1/s").arg(fmtBytes(upTotal)));
}

void OverviewSection::onSystemInfoUpdated(SystemInfo info) {
    m_hostnameLabel->setText(info.hostname);
    m_kernelLabel->setText(info.kernelVersion.section('-', 0, 0)); // keep it short, full kernel strings get noisy
    m_uptimeVal->setText(fmtUptime(info.uptimeSeconds));
    m_loadVal->setText(QString("%1  %2  %3")
        .arg(info.loadAvg1, 0,'f',2)
        .arg(info.loadAvg5, 0,'f',2)
        .arg(info.loadAvg15,0,'f',2));

    m_uptimeCard->setValue(fmtUptime(info.uptimeSeconds));
    m_uptimeCard->setSubtext(info.hostname);

    m_loadCard->setValue(QString::number(info.loadAvg1, 'f', 2));
    m_loadCard->setSubtext(QString("%1  %2")
        .arg(info.loadAvg5, 0,'f',2)
        .arg(info.loadAvg15,0,'f',2));

    m_sysCard->setValue(info.kernelVersion.section('-', 0, 0));
    m_sysCard->setSubtext(info.hostname);
}
