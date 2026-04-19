#include "disksection.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QFrame>
#include "core/systemmonitor.h"
#include "ui/stylesheet.h"

static QString fmtBytes(long long b) {
    if (b < 1024LL)                return QString("%1 B").arg(b);
    if (b < 1024LL*1024)           return QString("%1 KB").arg(b/1024);
    if (b < 1024LL*1024*1024)      return QString("%1 MB").arg(b/1024/1024);
    if (b < 1024LL*1024*1024*1024) return QString("%1 GB").arg(b/1024/1024/1024);
    return QString("%1 TB").arg(b/1024/1024/1024/1024);
}

static QColor barColor(double pct) {
    if (pct >= 90) return Style::errorColor();
    if (pct >= 75) return Style::warningColor();
    return Style::accentColor();
}

DiskSection::DiskSection(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto* header = new QLabel("Disks", this);
    header->setStyleSheet(
        "color:#e0e0e0;font-size:18px;font-weight:700;"
        "padding:20px 24px 8px;background:transparent;");
    outer->addWidget(header);

    auto* sep = new QWidget(this);
    sep->setFixedHeight(1);
    sep->setStyleSheet("background:#2a2a2a;");
    outer->addWidget(sep);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);

    auto* inner = new QWidget();
    inner->setStyleSheet("background:transparent;");
    scroll->setWidget(inner);

    auto* vbox = new QVBoxLayout(inner);
    vbox->setContentsMargins(24, 16, 24, 24);
    vbox->setSpacing(16);

    auto* fsHdr = new QLabel("Filesystems", inner);
    fsHdr->setStyleSheet(
        "color:#9e9e9e;font-size:12px;font-weight:600;background:transparent;");
    vbox->addWidget(fsHdr);

    m_partContainer = new QWidget(inner);
    m_partContainer->setStyleSheet("background:transparent;");
    m_partLayout = new QVBoxLayout(m_partContainer);
    m_partLayout->setContentsMargins(0, 0, 0, 0);
    m_partLayout->setSpacing(10);
    vbox->addWidget(m_partContainer);

    auto* devHdr = new QLabel("Block Devices I/O", inner);
    devHdr->setStyleSheet(
        "color:#9e9e9e;font-size:12px;font-weight:600;background:transparent;");
    vbox->addWidget(devHdr);

    m_devContainer = new QWidget(inner);
    m_devContainer->setStyleSheet("background:transparent;");
    m_devLayout = new QVBoxLayout(m_devContainer);
    m_devLayout->setContentsMargins(0, 0, 0, 0);
    m_devLayout->setSpacing(10);
    vbox->addWidget(m_devContainer);

    vbox->addStretch();

    auto& mon = SystemMonitor::instance();
    connect(&mon, &SystemMonitor::diskDataUpdated,
            this, &DiskSection::onDiskUpdated);
}

static void clearLayout(QLayout* layout) {
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }
}

void DiskSection::onDiskUpdated(QVector<DiskPartitionData> parts,
                                 QVector<DiskDeviceData> devs) {
    clearLayout(m_partLayout);
    clearLayout(m_devLayout);

    for (const auto& p : parts) {
        auto* card = new QWidget(m_partContainer);
        card->setStyleSheet(
            "background:#1a1a1a;border:1px solid #2a2a2a;border-radius:10px;");
        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(14, 12, 14, 12);
        cl->setSpacing(8);

        // Top row: mount point and percentage
        auto* row1 = new QHBoxLayout();
        auto* mpLabel = new QLabel(p.mountPoint, card);
        mpLabel->setStyleSheet(
            "color:#e0e0e0;font-size:14px;font-weight:600;background:transparent;");
        row1->addWidget(mpLabel);
        row1->addStretch();
        auto* pctLbl = new QLabel(
            QString::number(p.usagePercent, 'f', 1) + "%", card);
        pctLbl->setStyleSheet(QString("color:%1;font-size:14px;font-weight:700;"
            "font-family:'JetBrains Mono','Fira Mono',monospace;background:transparent;")
            .arg(barColor(p.usagePercent).name()));
        row1->addWidget(pctLbl);
        cl->addLayout(row1);

        // Usage bar
        auto* bar = new QProgressBar(card);
        bar->setFixedHeight(8);
        bar->setRange(0, 1000);
        bar->setValue(static_cast<int>(p.usagePercent * 10));
        bar->setTextVisible(false);
        bar->setStyleSheet(QString(
            "QProgressBar{background:#222;border:none;border-radius:4px;}"
            "QProgressBar::chunk{background:%1;border-radius:4px;}")
            .arg(barColor(p.usagePercent).name()));
        cl->addWidget(bar);

        // Detail row
        auto* row2 = new QHBoxLayout();
        auto* devLbl = new QLabel(p.device + "  [" + p.fsType + "]", card);
        devLbl->setStyleSheet("color:#555;font-size:11px;background:transparent;");
        row2->addWidget(devLbl);
        row2->addStretch();
        auto* sizeLbl = new QLabel(
            fmtBytes(p.usedBytes) + " / " + fmtBytes(p.totalBytes), card);
        sizeLbl->setStyleSheet(
            "color:#9e9e9e;font-size:12px;"
            "font-family:'JetBrains Mono','Fira Mono',monospace;background:transparent;");
        row2->addWidget(sizeLbl);
        cl->addLayout(row2);

        m_partLayout->addWidget(card);
    }

    // Block device I/O cards
    for (const auto& d : devs) {
        auto* card = new QWidget(m_devContainer);
        card->setStyleSheet(
            "background:#1a1a1a;border:1px solid #2a2a2a;border-radius:10px;");
        auto* cl = new QHBoxLayout(card);
        cl->setContentsMargins(14, 12, 14, 12);
        cl->setSpacing(20);

        auto* devLbl = new QLabel(d.device, card);
        devLbl->setStyleSheet(
            "color:#e0e0e0;font-size:14px;font-weight:600;background:transparent;");
        cl->addWidget(devLbl);
        cl->addStretch();

        auto* rdLbl = new QLabel("↓ " + fmtBytes(d.readBytesPerSec) + "/s", card);
        rdLbl->setStyleSheet(QString("color:%1;font-size:13px;"
            "font-family:'JetBrains Mono','Fira Mono',monospace;background:transparent;")
            .arg(Style::accent()));
        cl->addWidget(rdLbl);

        auto* wrLbl = new QLabel("↑ " + fmtBytes(d.writeBytesPerSec) + "/s", card);
        wrLbl->setStyleSheet(QString("color:%1;font-size:13px;"
            "font-family:'JetBrains Mono','Fira Mono',monospace;background:transparent;")
            .arg(Style::info()));
        cl->addWidget(wrLbl);

        if (d.tempCelsius >= 0) {
            const QString tc = d.tempCelsius >= 60 ? Style::error() :
                               d.tempCelsius >= 45 ? Style::warning() : Style::accent();
            auto* tLbl = new QLabel(QString::number(d.tempCelsius, 'f', 0) + "°C", card);
            tLbl->setStyleSheet(QString("color:%1;font-size:13px;"
                "font-family:'JetBrains Mono','Fira Mono',monospace;background:transparent;")
                .arg(tc));
            cl->addWidget(tLbl);
        }

        m_devLayout->addWidget(card);
    }
}
