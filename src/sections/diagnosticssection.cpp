#include "diagnosticssection.h"

#include <QScrollArea>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QProcess>
#include <QDateTime>
#include <QGuiApplication>
#include <QClipboard>
#include <QTextEdit>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include "core/systemmonitor.h"
#include "ui/stylesheet.h"
#include "ui/toastmanager.h"

namespace {
struct Check {
    DiagnosticFinding::Severity sev;
    QString title;
    QString details;
};
} // namespace

DiagnosticsSection::DiagnosticsSection(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    // Header row
    auto* hdrRow = new QHBoxLayout();
    hdrRow->setContentsMargins(24, 16, 24, 10);
    hdrRow->setSpacing(12);

    auto* hdrLabel = new QLabel("Diagnostics", this);
    hdrLabel->setStyleSheet(
        "color:#e0e0e0;font-size:18px;font-weight:700;background:transparent;");
    hdrRow->addWidget(hdrLabel);
    hdrRow->addStretch();

    m_runBtn = new QPushButton("▶  Run Diagnostics", this);
    m_runBtn->setObjectName("accentBtn");
    hdrRow->addWidget(m_runBtn);

    m_copyBtn = new QPushButton("⎘  Copy Report", this);
    hdrRow->addWidget(m_copyBtn);
    m_copyBtn->setEnabled(false);

    outer->addLayout(hdrRow);

    auto* sep = new QWidget(this);
    sep->setFixedHeight(1);
    sep->setStyleSheet("background:#2a2a2a;");
    outer->addWidget(sep);

    m_timestampLabel = new QLabel("Not yet run", this);
    m_timestampLabel->setStyleSheet(
        "color:#555;font-size:12px;padding:8px 24px 0;background:transparent;");
    outer->addWidget(m_timestampLabel);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);

    auto* inner = new QWidget();
    inner->setStyleSheet("background:transparent;");
    scroll->setWidget(inner);

    m_findingsLayout = new QVBoxLayout(inner);
    m_findingsLayout->setContentsMargins(24, 16, 24, 24);
    m_findingsLayout->setSpacing(10);

    m_findingsContainer = inner;
    m_findingsLayout->addStretch();

    // Connections
    connect(m_runBtn,  &QPushButton::clicked, this, &DiagnosticsSection::runDiagnostics);
    connect(m_copyBtn, &QPushButton::clicked, this, [this]{
        QGuiApplication::clipboard()->setText(m_reportText);
        ToastManager::instance().success("Report copied to clipboard");
    });

    auto& mon = SystemMonitor::instance();
    connect(&mon, &SystemMonitor::diskDataUpdated,
            this, &DiagnosticsSection::onDiskUpdated);
    connect(&mon, &SystemMonitor::cpuDataUpdated,
            this, &DiagnosticsSection::onCpuUpdated);
    connect(&mon, &SystemMonitor::processDataUpdated,
            this, &DiagnosticsSection::onProcessesUpdated);

    // Auto-run on first data available
    QTimer::singleShot(3000, this, &DiagnosticsSection::runDiagnostics);
}

void DiagnosticsSection::onDiskUpdated(QVector<DiskPartitionData> p, QVector<DiskDeviceData>) {
    m_diskParts = std::move(p);
}
void DiagnosticsSection::onCpuUpdated(CpuData d) { m_cpuData = std::move(d); }
void DiagnosticsSection::onProcessesUpdated(QVector<ProcessData> p) { m_procs = std::move(p); }

void DiagnosticsSection::clearFindings() {
    while (QLayoutItem* item = m_findingsLayout->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }
    m_findingsLayout->addStretch();
}

void DiagnosticsSection::addFinding(const DiagnosticFinding& f) {
    // Remove the stretch first
    if (m_findingsLayout->count() > 0) {
        QLayoutItem* last = m_findingsLayout->itemAt(m_findingsLayout->count() - 1);
        if (last && last->spacerItem()) {
            m_findingsLayout->removeItem(last);
            delete last;
        }
    }

    auto* card = new QWidget(m_findingsContainer);
    const QString borderColor =
        f.severity == DiagnosticFinding::Severity::Error   ? Style::error() :
        f.severity == DiagnosticFinding::Severity::Warning ? Style::warning() :
        f.severity == DiagnosticFinding::Severity::Info    ? Style::info() :
                                                              "#2a2a2a";
    card->setStyleSheet(QString(
        "background:#1a1a1a;border:1px solid %1;border-radius:10px;").arg(borderColor));

    auto* cl = new QVBoxLayout(card);
    cl->setContentsMargins(14, 12, 14, 12);
    cl->setSpacing(6);

    // Title row
    auto* titleRow = new QHBoxLayout();
    const QString icon =
        f.severity == DiagnosticFinding::Severity::Error   ? "✗" :
        f.severity == DiagnosticFinding::Severity::Warning ? "⚠" :
        f.severity == DiagnosticFinding::Severity::Info    ? "ℹ" : "✓";
    const QString color =
        f.severity == DiagnosticFinding::Severity::Error   ? Style::error() :
        f.severity == DiagnosticFinding::Severity::Warning ? Style::warning() :
        f.severity == DiagnosticFinding::Severity::Info    ? Style::info() :
                                                              Style::accent();
    auto* iconLbl = new QLabel(icon, card);
    iconLbl->setStyleSheet(
        QString("color:%1;font-size:16px;font-weight:bold;background:transparent;")
            .arg(color));
    titleRow->addWidget(iconLbl);

    auto* titleLbl = new QLabel(f.title, card);
    titleLbl->setStyleSheet(
        QString("color:%1;font-size:13px;font-weight:600;background:transparent;")
            .arg(color));
    titleRow->addWidget(titleLbl);
    titleRow->addStretch();
    cl->addLayout(titleRow);

    if (!f.details.isEmpty()) {
        auto* detLbl = new QLabel(f.details, card);
        detLbl->setStyleSheet(
            "color:#9e9e9e;font-size:12px;background:transparent;padding-left:24px;");
        detLbl->setWordWrap(true);
        cl->addWidget(detLbl);
    }

    m_findingsLayout->addWidget(card);
    m_findingsLayout->addStretch();
}

void DiagnosticsSection::runDiagnostics() {
    clearFindings();
    m_reportText.clear();
    m_copyBtn->setEnabled(false);

    const QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    m_timestampLabel->setText("Last run: " + ts);
    m_reportText += "Archvital Diagnostics Report\n";
    m_reportText += "Generated: " + ts + "\n";
    m_reportText += QString(40, '=') + "\n\n";

    QVector<DiagnosticFinding> findings;

    // ── 1. Systemd failed units ───────────────────────────────────────────
    {
        QProcess p;
        p.start("systemctl", {"--failed", "--no-pager", "--plain"});
        p.waitForFinished(5000);
        const QString out = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
        if (p.exitCode() == 0 && !out.isEmpty() && !out.startsWith("0 loaded")) {
            findings.append({DiagnosticFinding::Severity::Error,
                "Failed Systemd Units", out.left(400)});
        } else {
            findings.append({DiagnosticFinding::Severity::OK,
                "Systemd: All units healthy", {}});
        }
    }

    // ── 2. Critical journal errors ────────────────────────────────────────
    {
        QProcess p;
        p.start("journalctl", {"-p", "2", "-b", "--no-pager", "-n", "20"});
        p.waitForFinished(5000);
        const QString out = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
        const QStringList lines = out.split('\n', Qt::SkipEmptyParts);
        if (lines.size() > 1) {
            findings.append({DiagnosticFinding::Severity::Warning,
                QString("%1 critical journal error(s) since boot").arg(lines.size() - 1),
                lines.mid(1, 5).join('\n')});
        } else {
            findings.append({DiagnosticFinding::Severity::OK,
                "Journal: No critical errors since boot", {}});
        }
    }

    // ── 3. CPU temperatures ───────────────────────────────────────────────
    if (m_cpuData.packageTemp > 0) {
        if (m_cpuData.packageTemp >= 95) {
            findings.append({DiagnosticFinding::Severity::Error,
                QString("CPU temp critical: %1°C").arg(m_cpuData.packageTemp, 0,'f',1),
                "Consider improving cooling immediately."});
        } else if (m_cpuData.packageTemp >= 80) {
            findings.append({DiagnosticFinding::Severity::Warning,
                QString("CPU temp high: %1°C").arg(m_cpuData.packageTemp, 0,'f',1),
                "System is running hot — check thermal paste and airflow."});
        } else {
            findings.append({DiagnosticFinding::Severity::OK,
                QString("CPU temp normal: %1°C").arg(m_cpuData.packageTemp, 0,'f',1), {}});
        }
    }

    // ── 4. Disk usage ─────────────────────────────────────────────────────
    bool diskOk = true;
    for (const auto& part : m_diskParts) {
        if (part.usagePercent >= 85) {
            findings.append({DiagnosticFinding::Severity::Warning,
                QString("Disk usage high: %1 (%2%)")
                    .arg(part.mountPoint).arg(part.usagePercent, 0,'f',1),
                QString("Used: %1 / %2")
                    .arg(part.usedBytes / 1024/1024/1024)
                    .arg(part.totalBytes / 1024/1024/1024) + " GB"});
            diskOk = false;
        }
    }
    if (diskOk && !m_diskParts.isEmpty())
        findings.append({DiagnosticFinding::Severity::OK, "Disk usage: All filesystems OK", {}});

    // ── 5. Zombie processes ───────────────────────────────────────────────
    QStringList zombies;
    for (const auto& p : m_procs)
        if (p.status == "Z")
            zombies.append(QString("%1 [%2]").arg(p.name).arg(p.pid));
    if (!zombies.isEmpty()) {
        findings.append({DiagnosticFinding::Severity::Warning,
            QString("%1 zombie process(es) detected").arg(zombies.size()),
            zombies.join(", ")});
    } else {
        findings.append({DiagnosticFinding::Severity::OK, "No zombie processes", {}});
    }

    // ── 6. Long uptime ────────────────────────────────────────────────────
    if (m_cpuData.loadAvg1 >= 0) {
        QProcess up;
        up.start("cat", {"/proc/uptime"});
        up.waitForFinished(1000);
        double uptimeSec = QString::fromLocal8Bit(up.readAllStandardOutput())
            .split(' ').first().toDouble();
        if (uptimeSec > 30 * 86400) {
            findings.append({DiagnosticFinding::Severity::Warning,
                QString("System uptime > 30 days (%1 days)")
                    .arg(static_cast<int>(uptimeSec / 86400)),
                "Consider rebooting to apply pending kernel updates."});
        }
    }

    // ── 7. Swap ───────────────────────────────────────────────────────────
    // (Checked via /proc/meminfo in memory data — approximate here)
    {
        QFile f("/proc/meminfo");
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            long long swapTotal = 0;
            QTextStream ts(&f);
            while (!ts.atEnd()) {
                const QString line = ts.readLine();
                if (line.startsWith("SwapTotal:")) {
                    const QStringList sp = line.split(QRegularExpression(R"(\s+)"));
                    if (sp.size() > 1) swapTotal = sp[1].toLongLong();
                }
            }
            if (swapTotal == 0)
                findings.append({DiagnosticFinding::Severity::Info,
                    "No swap configured", "System has no swap space."});
        }
    }

    // ── Display findings ──────────────────────────────────────────────────
    for (const auto& f : findings) {
        addFinding(f);
        const QString sevStr =
            f.severity == DiagnosticFinding::Severity::Error   ? "[ERROR]  " :
            f.severity == DiagnosticFinding::Severity::Warning ? "[WARN]   " :
            f.severity == DiagnosticFinding::Severity::Info    ? "[INFO]   " : "[OK]     ";
        m_reportText += sevStr + f.title + "\n";
        if (!f.details.isEmpty())
            m_reportText += "         " + f.details + "\n";
        m_reportText += "\n";
    }

    m_copyBtn->setEnabled(true);

    // Toast for critical findings
    int errors = 0, warnings = 0;
    for (const auto& f : findings) {
        if (f.severity == DiagnosticFinding::Severity::Error)   errors++;
        if (f.severity == DiagnosticFinding::Severity::Warning) warnings++;
    }
    if (errors > 0)
        ToastManager::instance().error(
            QString("%1 critical diagnostic issue(s) found").arg(errors));
    else if (warnings > 0)
        ToastManager::instance().warning(
            QString("%1 warning(s) found in diagnostics").arg(warnings));
    else
        ToastManager::instance().success("Diagnostics: All checks passed");
}
