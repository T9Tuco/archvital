#include "cpusection.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include "core/systemmonitor.h"
#include "ui/stylesheet.h"

static QColor barColor(double pct) {
    if (pct >= 85) return Style::errorColor();
    if (pct >= 65) return Style::warningColor();
    return Style::accentColor();
}

// small label helpers so the file does not drown in stylesheet strings

static QLabel* makeSecLabel(const QString& t, QWidget* p) {
    auto* l = new QLabel(t, p);
    l->setStyleSheet("color:#444;font-size:11px;background:transparent;");
    return l;
}
static QLabel* makeValLabel(const QString& t, QWidget* p) {
    auto* l = new QLabel(t, p);
    l->setStyleSheet(
        "color:#aaa;font-size:12px;"
        "font-family:'JetBrains Mono','Fira Mono',monospace;background:transparent;");
    return l;
}

// cpu page ui

CpuSection::CpuSection(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // title row, plus the model name off to the side
    auto* titleBar = new QWidget(this);
    titleBar->setFixedHeight(52);
    auto* tl = new QHBoxLayout(titleBar);
    tl->setContentsMargins(24, 0, 24, 0);
    auto* title = new QLabel("CPU", titleBar);
    title->setStyleSheet(
        "color:#c0c0c0;font-size:16px;font-weight:600;background:transparent;");
    tl->addWidget(title);
    tl->addStretch();
    m_modelLabel = new QLabel("", titleBar);
    m_modelLabel->setStyleSheet(
        "color:#444;font-size:12px;background:transparent;");
    tl->addWidget(m_modelLabel);
    outer->addWidget(titleBar);

    auto* divider = new QWidget(this);
    divider->setFixedHeight(1);
    divider->setStyleSheet("background:#1a1a1a;");
    outer->addWidget(divider);

    // scroll area because core counts can get silly fast
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);

    auto* inner = new QWidget();
    inner->setStyleSheet("background:transparent;");
    scroll->setWidget(inner);

    auto* vbox = new QVBoxLayout(inner);
    vbox->setContentsMargins(20, 16, 20, 20);
    vbox->setSpacing(12);

    // overall cpu card at the top
    auto* overallCard = new QWidget(inner);
    overallCard->setStyleSheet(
        "background:#161616;border:1px solid #1e1e1e;border-radius:9px;");
    auto* oc = new QVBoxLayout(overallCard);
    oc->setContentsMargins(16, 14, 16, 14);
    oc->setSpacing(8);

    // main usage row
    auto* usageRow = new QHBoxLayout();

    m_overallPctLabel = new QLabel("—", overallCard);
    m_overallPctLabel->setStyleSheet(
        "color:#00e5ff;font-size:28px;font-weight:700;"
        "font-family:'JetBrains Mono','Fira Mono',monospace;background:transparent;");
    usageRow->addWidget(m_overallPctLabel);
    usageRow->addSpacing(12);

    // small side stats under the big number
    auto* statsCol = new QVBoxLayout();
    statsCol->setSpacing(2);
    m_archLabel = makeSecLabel("", overallCard);
    m_tempLabel = makeSecLabel("", overallCard);
    statsCol->addWidget(m_archLabel);
    statsCol->addWidget(m_tempLabel);
    usageRow->addLayout(statsCol);
    usageRow->addStretch();
    oc->addLayout(usageRow);

    m_overallBar = new BarWidget(overallCard);
    m_overallBar->setBarHeight(5);
    oc->addWidget(m_overallBar);

    // recent history and load avg sit together pretty nicely
    m_loadGraph = new SparklineWidget(overallCard);
    m_loadGraph->setFixedHeight(44);
    m_loadGraph->setFixedMax(100.0);
    oc->addWidget(m_loadGraph);

    m_loadLabel = makeSecLabel("Load: — — —", overallCard);
    oc->addWidget(m_loadLabel);

    vbox->addWidget(overallCard);

    // per-core section header
    auto* coreHdr = new QHBoxLayout();
    auto* coreTitle = makeSecLabel("PER CORE", inner);
    coreHdr->addWidget(coreTitle);
    coreHdr->addStretch();
    vbox->addLayout(coreHdr);

    // core list box
    m_coresContainer = new QWidget(inner);
    m_coresContainer->setStyleSheet(
        "background:#161616;border:1px solid #1e1e1e;border-radius:9px;");
    vbox->addWidget(m_coresContainer);
    vbox->addStretch();

    // one signal is enough, the worker does the heavy lifting
    auto& mon = SystemMonitor::instance();
    connect(&mon, &SystemMonitor::cpuDataUpdated, this, &CpuSection::onCpuUpdated);
}

void CpuSection::buildCoreRows(int count) {
    if (m_coreRowsBuilt) return;
    m_coreRowsBuilt = true;

    auto* grid = new QGridLayout(m_coresContainer);
    grid->setContentsMargins(12, 10, 12, 10);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(7);

    // header row so the columns do not feel like a guessing game
    grid->addWidget(makeSecLabel("", m_coresContainer),    0, 0);
    grid->addWidget(makeSecLabel("USAGE", m_coresContainer), 0, 1);
    grid->addWidget(makeSecLabel("%", m_coresContainer),   0, 2);
    grid->addWidget(makeSecLabel("FREQ", m_coresContainer), 0, 3);
    grid->addWidget(makeSecLabel("TEMP", m_coresContainer), 0, 4);

    for (int i = 0; i < count; i++) {
        CoreRow row;
        row.labelId = makeSecLabel(QString("cpu%1").arg(i), m_coresContainer);

        row.bar = new BarWidget(m_coresContainer);
        row.bar->setBarHeight(4);

        row.pctLabel  = makeValLabel("—", m_coresContainer);
        row.pctLabel->setMinimumWidth(44);
        row.freqLabel = makeValLabel("—", m_coresContainer);
        row.tempLabel = makeValLabel("—", m_coresContainer);

        const int r = i + 1;
        grid->addWidget(row.labelId,   r, 0);
        grid->addWidget(row.bar,       r, 1);
        grid->addWidget(row.pctLabel,  r, 2);
        grid->addWidget(row.freqLabel, r, 3);
        grid->addWidget(row.tempLabel, r, 4);

        m_coreRows.append(row);
    }
    grid->setColumnStretch(1, 1);
    grid->setColumnMinimumWidth(0, 40);
    grid->setColumnMinimumWidth(2, 44);
    grid->setColumnMinimumWidth(3, 72);
    grid->setColumnMinimumWidth(4, 52);
}

void CpuSection::onCpuUpdated(CpuData d) {
    const double pct = d.overallUsage;
    const QColor c = barColor(pct);

    m_overallPctLabel->setText(QString::number(pct, 'f', 1) + "%");
    m_overallPctLabel->setStyleSheet(QString(
        "color:%1;font-size:28px;font-weight:700;"
        "font-family:'JetBrains Mono','Fira Mono',monospace;background:transparent;")
        .arg(c.name()));

    m_overallBar->setValue(pct);
    m_overallBar->setColor(c);

    if (!d.modelName.isEmpty())
        m_modelLabel->setText(d.modelName);

    m_archLabel->setText(QString("%1   %2C / %3T")
        .arg(d.architecture).arg(d.physicalCores).arg(d.logicalCores));

    if (d.packageTemp > 0)
        m_tempLabel->setText(QString("Package temp: %1 C").arg(d.packageTemp, 0,'f',1));

    m_loadLabel->setText(QString("Load avg:  %1   %2   %3")
        .arg(d.loadAvg1, 0,'f',2)
        .arg(d.loadAvg5, 0,'f',2)
        .arg(d.loadAvg15,0,'f',2));

    m_loadGraph->setValues(d.usageHistory);

    if (!m_coreRowsBuilt && !d.cores.isEmpty())
        buildCoreRows(d.cores.size());

    for (int i = 0; i < d.cores.size() && i < m_coreRows.size(); i++) {
        const auto& cd = d.cores[i];
        auto& row = m_coreRows[i];
        row.bar->setValue(cd.usagePercent);
        row.bar->setColor(barColor(cd.usagePercent));
        row.pctLabel->setText(QString::number(cd.usagePercent, 'f', 1) + "%");
        if (cd.freqKHz > 0)
            row.freqLabel->setText(QString::number(cd.freqKHz / 1000) + " MHz");
        if (cd.tempCelsius > 0)
            row.tempLabel->setText(QString::number(cd.tempCelsius, 'f', 0) + " C");
    }
}
