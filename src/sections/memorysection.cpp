#include "memorysection.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QHeaderView>
#include <algorithm>
#include "core/systemmonitor.h"
#include "ui/stylesheet.h"

static QString fmtKB(long long kb) {
    if (kb < 1024)       return QString("%1 KB").arg(kb);
    if (kb < 1024*1024)  return QString("%1 MB").arg(kb / 1024);
    return QString("%1 GB").arg(kb / 1024 / 1024);
}

static QLabel* field(const QString& text, QWidget* parent) {
    auto* l = new QLabel(text, parent);
    l->setStyleSheet("color: #9e9e9e; font-size: 12px; background: transparent;");
    return l;
}

static QLabel* value(const QString& text, QWidget* parent) {
    auto* l = new QLabel(text, parent);
    l->setStyleSheet(
        "color: #e0e0e0; font-size: 13px; "
        "font-family: 'JetBrains Mono','Fira Mono',monospace; background: transparent;");
    return l;
}

MemorySection::MemorySection(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto* header = new QLabel("Memory", this);
    header->setStyleSheet(
        "color: #e0e0e0; font-size: 18px; font-weight: 700; "
        "padding: 20px 24px 8px; background: transparent;");
    outer->addWidget(header);

    auto* sep = new QWidget(this);
    sep->setFixedHeight(1);
    sep->setStyleSheet("background: #2a2a2a;");
    outer->addWidget(sep);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);

    auto* inner = new QWidget();
    inner->setStyleSheet("background: transparent;");
    scroll->setWidget(inner);

    auto* vbox = new QVBoxLayout(inner);
    vbox->setContentsMargins(24, 16, 24, 24);
    vbox->setSpacing(16);

    // this card is the quick "how bad is it right now" snapshot
    auto* usageCard = new QWidget(inner);
    usageCard->setStyleSheet(
        "background: #1a1a1a; border: 1px solid #2a2a2a; border-radius: 10px;");
    auto* ul = new QVBoxLayout(usageCard);
    ul->setContentsMargins(16, 14, 16, 14);
    ul->setSpacing(10);

    // big percent on the left, raw numbers on the right
    auto* topRow = new QHBoxLayout();
    m_pctLabel = new QLabel("—", usageCard);
    m_pctLabel->setStyleSheet(
        "color: #00e5ff; font-size: 24px; font-weight: 700; "
        "font-family: 'JetBrains Mono','Fira Mono',monospace; background: transparent;");
    topRow->addWidget(m_pctLabel);
    topRow->addStretch();
    m_usedLabel = new QLabel("—", usageCard);
    m_usedLabel->setStyleSheet(
        "color: #e0e0e0; font-size: 14px; background: transparent;");
    topRow->addWidget(m_usedLabel);
    ul->addLayout(topRow);

    m_bar = new BarWidget(usageCard);
    m_bar->setBarHeight(12);
    ul->addWidget(m_bar);

    // details live down here so the main number can stay uncluttered
    auto* detGrid = new QGridLayout();
    detGrid->setSpacing(8);

    detGrid->addWidget(field("Total:",      usageCard), 0, 0);
    m_totalLabel = value("—", usageCard);
    detGrid->addWidget(m_totalLabel, 0, 1);

    detGrid->addWidget(field("Free:",       usageCard), 1, 0);
    m_freeLabel = value("—", usageCard);
    detGrid->addWidget(m_freeLabel, 1, 1);

    detGrid->addWidget(field("Available:",  usageCard), 2, 0);
    m_availLabel = value("—", usageCard);
    detGrid->addWidget(m_availLabel, 2, 1);

    detGrid->addWidget(field("Buff/Cache:", usageCard), 3, 0);
    m_buffLabel = value("—", usageCard);
    detGrid->addWidget(m_buffLabel, 3, 1);

    detGrid->addWidget(field("Swap:",       usageCard), 4, 0);
    m_swapLabel = value("—", usageCard);
    detGrid->addWidget(m_swapLabel, 4, 1);

    detGrid->setColumnStretch(1, 1);
    ul->addLayout(detGrid);

    // tiny trend line so the page has a memory longer than one poll
    m_graph = new SparklineWidget(usageCard);
    m_graph->setFixedHeight(60);
    m_graph->setFixedMax(100.0);
    ul->addWidget(m_graph);

    vbox->addWidget(usageCard);

    // the usual suspects that are eating memory right now
    auto* procHeader = new QLabel("Top Memory Consumers", inner);
    procHeader->setStyleSheet(
        "color: #9e9e9e; font-size: 12px; font-weight: 600; background: transparent;");
    vbox->addWidget(procHeader);

    m_procTable = new QTableWidget(0, 4, inner);
    m_procTable->setHorizontalHeaderLabels({"PID", "Name", "MEM (MB)", "MEM %"});
    m_procTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_procTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_procTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_procTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_procTable->verticalHeader()->setVisible(false);
    m_procTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_procTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_procTable->setAlternatingRowColors(true);
    m_procTable->setMinimumHeight(240);
    vbox->addWidget(m_procTable);

    vbox->addStretch();

    auto& mon = SystemMonitor::instance();
    connect(&mon, &SystemMonitor::memoryDataUpdated,
            this, &MemorySection::onMemoryUpdated);
    connect(&mon, &SystemMonitor::processDataUpdated,
            this, &MemorySection::onProcessesUpdated);
}

void MemorySection::onMemoryUpdated(MemoryData d) {
    m_pctLabel->setText(QString::number(d.usagePercent, 'f', 1) + "%");
    m_pctLabel->setStyleSheet(QString(
        "color: %1; font-size: 24px; font-weight: 700; "
        "font-family: 'JetBrains Mono','Fira Mono',monospace; background: transparent;")
        .arg(d.usagePercent >= 90 ? Style::error() :
             d.usagePercent >= 70 ? Style::warning() : Style::accent()));

    m_usedLabel->setText(fmtKB(d.usedKB) + " / " + fmtKB(d.totalKB));
    m_bar->setValue(d.usagePercent);
    m_bar->setColor(d.usagePercent >= 90 ? Style::errorColor() :
                    d.usagePercent >= 70 ? Style::warningColor() : Style::accentColor());

    m_totalLabel->setText(fmtKB(d.totalKB));
    m_freeLabel->setText(fmtKB(d.freeKB));
    m_availLabel->setText(fmtKB(d.availableKB));
    m_buffLabel->setText(fmtKB(d.buffersKB + d.cachedKB));

    if (d.swapTotalKB == 0)
        m_swapLabel->setText("No swap configured");
    else
        m_swapLabel->setText(fmtKB(d.swapUsedKB) + " / " + fmtKB(d.swapTotalKB));

    m_graph->setValues(d.usageHistory);
}

void MemorySection::onProcessesUpdated(QVector<ProcessData> procs) {
    // biggest memory hogs first, because nobody scrolls a blame list on purpose
    std::sort(procs.begin(), procs.end(),
        [](const ProcessData& a, const ProcessData& b){
            return a.memBytes > b.memBytes;
        });

    const int n = static_cast<int>(std::min(procs.size(), qsizetype{10}));
    m_procTable->setRowCount(n);
    for (int i = 0; i < n; i++) {
        const auto& p = procs[i];
        m_procTable->setItem(i, 0, new QTableWidgetItem(QString::number(p.pid)));
        m_procTable->setItem(i, 1, new QTableWidgetItem(p.name));
        m_procTable->setItem(i, 2, new QTableWidgetItem(
            QString::number(p.memBytes / 1024 / 1024)));
        m_procTable->setItem(i, 3, new QTableWidgetItem(
            QString::number(p.memPercent, 'f', 1) + "%"));
    }
}
