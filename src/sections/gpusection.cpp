#include "gpusection.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QStackedWidget>
#include "core/systemmonitor.h"
#include "ui/stylesheet.h"

namespace {
QLabel* fld(const QString& t, QWidget* p) {
    auto* l = new QLabel(t, p);
    l->setStyleSheet("color:#9e9e9e;font-size:12px;background:transparent;");
    return l;
}
QLabel* val(const QString& t, QWidget* p) {
    auto* l = new QLabel(t, p);
    l->setStyleSheet(
        "color:#e0e0e0;font-size:13px;"
        "font-family:'JetBrains Mono','Fira Mono',monospace;background:transparent;");
    return l;
}
} // namespace

GpuSection::GpuSection(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto* header = new QLabel("GPU", this);
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

    // No-GPU message
    m_noGpuMsg = new QWidget(inner);
    {
        auto* nl = new QVBoxLayout(m_noGpuMsg);
        nl->setContentsMargins(0, 40, 0, 0);
        nl->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        auto* msg = new QLabel(
            "🎮  GPU monitoring not available\n\n"
            "Ensure amdgpu / nvidia / i915 drivers are loaded\n"
            "and the corresponding hwmon kernel interface is accessible.",
            m_noGpuMsg);
        msg->setAlignment(Qt::AlignCenter);
        msg->setWordWrap(true);
        msg->setStyleSheet("color:#555;font-size:14px;background:transparent;");
        nl->addWidget(msg);
    }
    vbox->addWidget(m_noGpuMsg);
    m_noGpuMsg->hide();

    // Main GPU data card
    m_dataWidget = new QWidget(inner);
    m_dataWidget->setStyleSheet(
        "background:#1a1a1a;border:1px solid #2a2a2a;border-radius:10px;");
    auto* dl = new QVBoxLayout(m_dataWidget);
    dl->setContentsMargins(16, 14, 16, 14);
    dl->setSpacing(14);

    // Driver/name row
    auto* topRow = new QHBoxLayout();
    m_driverLabel = new QLabel("—", m_dataWidget);
    m_driverLabel->setStyleSheet(
        "color:#00e5ff;font-size:15px;font-weight:600;background:transparent;");
    topRow->addWidget(m_driverLabel);
    topRow->addStretch();
    dl->addLayout(topRow);

    // Separator
    auto* s1 = new QWidget(m_dataWidget);
    s1->setFixedHeight(1); s1->setStyleSheet("background:#2a2a2a;");
    dl->addWidget(s1);

    // Temperature section
    auto* tempHdr = fld("TEMPERATURES", m_dataWidget);
    dl->addWidget(tempHdr);

    auto* tempGrid = new QGridLayout();
    tempGrid->setSpacing(8);
    tempGrid->addWidget(fld("Edge:",     m_dataWidget), 0, 0);
    m_tempEdgeLabel = val("—", m_dataWidget);  tempGrid->addWidget(m_tempEdgeLabel, 0, 1);
    tempGrid->addWidget(fld("Junction:", m_dataWidget), 1, 0);
    m_tempJunctLabel = val("—", m_dataWidget); tempGrid->addWidget(m_tempJunctLabel, 1, 1);
    tempGrid->addWidget(fld("Memory:",   m_dataWidget), 2, 0);
    m_tempMemLabel = val("—", m_dataWidget);   tempGrid->addWidget(m_tempMemLabel, 2, 1);
    tempGrid->setColumnStretch(1, 1);
    dl->addLayout(tempGrid);

    m_tempGraph = new SparklineWidget(m_dataWidget);
    m_tempGraph->setFixedHeight(50);
    m_tempGraph->setFixedMax(120.0);
    m_tempGraph->setColor(Style::warningColor());
    dl->addWidget(m_tempGraph);

    // Fan section
    auto* s2 = new QWidget(m_dataWidget);
    s2->setFixedHeight(1); s2->setStyleSheet("background:#2a2a2a;");
    dl->addWidget(s2);

    auto* fanHdr = fld("FAN", m_dataWidget);
    dl->addWidget(fanHdr);

    auto* fanGrid = new QGridLayout();
    fanGrid->setSpacing(8);
    fanGrid->addWidget(fld("RPM:",  m_dataWidget), 0, 0);
    m_fanRpmLabel = val("—", m_dataWidget); fanGrid->addWidget(m_fanRpmLabel, 0, 1);
    fanGrid->addWidget(fld("Duty:", m_dataWidget), 1, 0);
    m_fanPctLabel = val("—", m_dataWidget); fanGrid->addWidget(m_fanPctLabel, 1, 1);
    fanGrid->setColumnStretch(1, 1);
    dl->addLayout(fanGrid);

    // Power section
    auto* s3 = new QWidget(m_dataWidget);
    s3->setFixedHeight(1); s3->setStyleSheet("background:#2a2a2a;");
    dl->addWidget(s3);

    auto* pwrHdr = fld("POWER", m_dataWidget);
    dl->addWidget(pwrHdr);

    m_powerLabel = val("—", m_dataWidget);
    dl->addWidget(m_powerLabel);
    m_powerBar = new QProgressBar(m_dataWidget);
    m_powerBar->setFixedHeight(6);
    m_powerBar->setRange(0, 1000);
    m_powerBar->setTextVisible(false);
    m_powerBar->setStyleSheet(
        "QProgressBar{background:#222;border:none;border-radius:3px;}"
        "QProgressBar::chunk{background:#ff9800;border-radius:3px;}");
    dl->addWidget(m_powerBar);

    // Clocks + voltage section
    auto* s4 = new QWidget(m_dataWidget);
    s4->setFixedHeight(1); s4->setStyleSheet("background:#2a2a2a;");
    dl->addWidget(s4);

    auto* clkHdr = fld("CLOCKS & VOLTAGE", m_dataWidget);
    dl->addWidget(clkHdr);

    auto* clkGrid = new QGridLayout();
    clkGrid->setSpacing(8);
    clkGrid->addWidget(fld("Shader:",  m_dataWidget), 0, 0);
    m_sclkLabel = val("—", m_dataWidget); clkGrid->addWidget(m_sclkLabel, 0, 1);
    clkGrid->addWidget(fld("Memory:",  m_dataWidget), 1, 0);
    m_mclkLabel = val("—", m_dataWidget); clkGrid->addWidget(m_mclkLabel, 1, 1);
    clkGrid->addWidget(fld("Voltage:", m_dataWidget), 2, 0);
    m_voltLabel = val("—", m_dataWidget); clkGrid->addWidget(m_voltLabel, 2, 1);
    clkGrid->setColumnStretch(1, 1);
    dl->addLayout(clkGrid);

    // VRAM section
    auto* s5 = new QWidget(m_dataWidget);
    s5->setFixedHeight(1); s5->setStyleSheet("background:#2a2a2a;");
    dl->addWidget(s5);

    auto* vramHdr = fld("VRAM", m_dataWidget);
    dl->addWidget(vramHdr);

    m_vramLabel = val("—", m_dataWidget);
    dl->addWidget(m_vramLabel);
    m_vramBar = new QProgressBar(m_dataWidget);
    m_vramBar->setFixedHeight(6);
    m_vramBar->setRange(0, 1000);
    m_vramBar->setTextVisible(false);
    m_vramBar->setStyleSheet(
        "QProgressBar{background:#222;border:none;border-radius:3px;}"
        "QProgressBar::chunk{background:#00e5ff;border-radius:3px;}");
    dl->addWidget(m_vramBar);

    vbox->addWidget(m_dataWidget);
    vbox->addStretch();

    auto& mon = SystemMonitor::instance();
    connect(&mon, &SystemMonitor::gpuDataUpdated,
            this, &GpuSection::onGpuUpdated);
}

void GpuSection::onGpuUpdated(GpuData d) {
    static QVector<double> tempHistory;

    if (!d.available) {
        m_dataWidget->hide();
        m_noGpuMsg->show();
        return;
    }

    m_noGpuMsg->hide();
    m_dataWidget->show();

    m_driverLabel->setText(d.driverName.toUpper());

    auto fmtTemp = [](double t) -> QString {
        return t >= 0 ? QString::number(t, 'f', 1) + " °C" : "N/A";
    };
    m_tempEdgeLabel->setText(fmtTemp(d.tempEdge));
    m_tempJunctLabel->setText(fmtTemp(d.tempJunction));
    m_tempMemLabel->setText(fmtTemp(d.tempMemory));

    if (d.tempEdge >= 0) {
        tempHistory.append(d.tempEdge);
        if (tempHistory.size() > 60) tempHistory.removeFirst();
        m_tempGraph->setValues(tempHistory);
        QColor tc = d.tempEdge >= 95 ? Style::errorColor() :
                    d.tempEdge >= 80 ? Style::warningColor() :
                                       Style::accentColor();
        m_tempGraph->setColor(tc);
    }

    if (d.fanRpm >= 0) {
        m_fanRpmLabel->setText(QString::number(d.fanRpm) + " RPM");
        const QString fanColor =
            d.fanPercent >= 90 ? Style::error() :
            d.fanPercent >= 70 ? Style::warning() : Style::accent();
        m_fanPctLabel->setText(QString("<span style='color:%1'>%2%</span>")
            .arg(fanColor).arg(d.fanPercent));
    } else {
        m_fanRpmLabel->setText("N/A");
        m_fanPctLabel->setText("N/A");
    }

    if (d.powerDraw >= 0) {
        m_powerLabel->setText(QString("%1 W / %2 W")
            .arg(d.powerDraw, 0, 'f', 1)
            .arg(d.powerCap >= 0 ? QString::number(d.powerCap, 'f', 1) : "?"));
        if (d.powerCap > 0)
            m_powerBar->setValue(static_cast<int>(d.powerDraw / d.powerCap * 1000));
    } else {
        m_powerLabel->setText("N/A");
    }

    m_sclkLabel->setText(d.shaderClockMHz >= 0 ?
        QString::number(d.shaderClockMHz) + " MHz" : "N/A");
    m_mclkLabel->setText(d.memClockMHz >= 0 ?
        QString::number(d.memClockMHz) + " MHz" : "N/A");
    m_voltLabel->setText(d.voltage >= 0 ?
        QString::number(d.voltage, 'f', 3) + " V" : "N/A");

    if (d.vramTotalMB > 0) {
        m_vramLabel->setText(QString("%1 MB / %2 MB")
            .arg(d.vramUsedMB).arg(d.vramTotalMB));
        m_vramBar->setValue(static_cast<int>(
            static_cast<double>(d.vramUsedMB) / d.vramTotalMB * 1000));
    } else {
        m_vramLabel->setText("N/A");
    }
}
