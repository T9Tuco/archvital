#include "mainwindow.h"

#include <QHBoxLayout>
#include <QWidget>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QMoveEvent>
#include <QDateTime>
#include <QTimer>
#include <QApplication>

#include "core/systemmonitor.h"
#include "ui/stylesheet.h"

#include "sections/overviewsection.h"
#include "sections/cpusection.h"
#include "sections/memorysection.h"
#include "sections/gpusection.h"
#include "sections/disksection.h"
#include "sections/networksection.h"
#include "sections/tasksection.h"
#include "sections/diagnosticssection.h"
#include "sections/settingssection.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("archvital");
    setMinimumSize(1000, 650);
    Style::loadTheme(m_settings);
    setupUI();
    restoreGeometry();

    ToastManager::instance().setParentWidget(this);

    auto& mon = SystemMonitor::instance();
    connect(&mon, &SystemMonitor::cpuDataUpdated,
            this, &MainWindow::checkTemperatureAlerts);
    connect(&mon, &SystemMonitor::diskDataUpdated,
            this, &MainWindow::checkDiskAlerts);

    mon.start(2000);
}

MainWindow::~MainWindow() {
    SystemMonitor::instance().stop();
}

void MainWindow::setupUI() {
    auto* central = new QWidget(this);
    central->setStyleSheet(QString("background: %1;").arg(Style::bg()));
    setCentralWidget(central);

    auto* hbox = new QHBoxLayout(central);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(0);

    m_sidebar = new Sidebar(central);
    hbox->addWidget(m_sidebar);

    m_stack = new QStackedWidget(central);
    m_stack->setStyleSheet("background: transparent;");
    hbox->addWidget(m_stack, 1);

    // keep this order in sync with the sidebar, otherwise navigation gets weird
    m_stack->addWidget(new OverviewSection(m_stack));
    m_stack->addWidget(new CpuSection(m_stack));
    m_stack->addWidget(new MemorySection(m_stack));
    m_stack->addWidget(new GpuSection(m_stack));
    m_stack->addWidget(new DiskSection(m_stack));
    m_stack->addWidget(new NetworkSection(m_stack));
    m_stack->addWidget(new TaskSection(m_stack));
    m_stack->addWidget(new DiagnosticsSection(m_stack));
    auto* settingsSection = new SettingsSection(&m_settings, m_stack);
    m_stack->addWidget(settingsSection);

    connect(m_sidebar, &Sidebar::sectionRequested,
            this, &MainWindow::onSectionRequested);
    connect(m_sidebar, &Sidebar::collapseToggled,
            this, [this](bool collapsed){
                m_settings.setValue("sidebar/collapsed", collapsed);
            });
    connect(settingsSection, &SettingsSection::themeChanged,
            this, &MainWindow::onThemeChanged);
    connect(settingsSection, &SettingsSection::sectionVisibilityChanged,
            this, &MainWindow::onSectionVisibilityChanged);

    applySectionVisibility();

    // last open section is a nice little quality-of-life thing
    const int lastSection = m_settings.value("window/lastSection", 0).toInt();
    onSectionRequested(lastSection);
}

void MainWindow::onSectionRequested(int id) {
    if (!isSectionVisible(id))
        id = firstVisibleSection();
    m_currentSection = id;
    m_sidebar->setActiveSection(id);
    m_stack->setCurrentIndex(id);
    m_settings.setValue("window/lastSection", id);
}

void MainWindow::onThemeChanged() {
    applyTheme();
    setupUI();
    restoreGeometry();
    onSectionRequested(AppSections::Settings);
}

void MainWindow::onSectionVisibilityChanged(int id, bool visible) {
    if (id == AppSections::Settings) return;
    m_settings.setValue(QString("sections/%1/visible").arg(AppSections::kSections[id].key), visible);
    applySectionVisibility();
    if (!visible && m_currentSection == id)
        onSectionRequested(firstVisibleSection());
}

void MainWindow::saveGeometry() {
    m_settings.setValue("window/geometry", QMainWindow::saveGeometry());
    m_settings.setValue("window/state",    saveState());
}

void MainWindow::restoreGeometry() {
    if (m_settings.contains("window/geometry"))
        QMainWindow::restoreGeometry(m_settings.value("window/geometry").toByteArray());
    if (m_settings.contains("sidebar/collapsed"))
        m_sidebar->setCollapsed(m_settings.value("sidebar/collapsed").toBool());
}

void MainWindow::applyTheme() {
    Style::loadTheme(m_settings);
    qApp->setStyleSheet(Style::appStyleSheet());
}

bool MainWindow::isSectionVisible(int id) const {
    if (id < 0 || id >= AppSections::Count) return false;
    if (id == AppSections::Settings) return true;
    return m_settings.value(
        QString("sections/%1/visible").arg(AppSections::kSections[id].key), true).toBool();
}

int MainWindow::firstVisibleSection() const {
    for (int i = 0; i < AppSections::Count; i++) {
        if (isSectionVisible(i)) return i;
    }
    return AppSections::Settings;
}

void MainWindow::applySectionVisibility() {
    if (!m_sidebar) return;
    for (int i = 0; i < AppSections::Count; i++)
        m_sidebar->setSectionVisible(i, isSectionVisible(i));
}

void MainWindow::closeEvent(QCloseEvent* e) {
    saveGeometry();
    e->accept();
}

void MainWindow::resizeEvent(QResizeEvent* e) {
    QMainWindow::resizeEvent(e);
    saveGeometry();
}

void MainWindow::moveEvent(QMoveEvent* e) {
    QMainWindow::moveEvent(e);
    saveGeometry();
}

void MainWindow::checkTemperatureAlerts(CpuData data) {
    if (data.packageTemp <= 0) return;

    const QString key = "cpu_temp";
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastAlertTime.value(key, 0) < ALERT_COOLDOWN_MS) return;

    if (data.packageTemp >= 95) {
        ToastManager::instance().error(
            QString("CPU temp critical: %1°C").arg(data.packageTemp, 0, 'f', 1));
        m_lastAlertTime[key] = now;
    } else if (data.packageTemp >= 80) {
        ToastManager::instance().warning(
            QString("CPU temp high: %1°C").arg(data.packageTemp, 0, 'f', 1));
        m_lastAlertTime[key] = now;
    }
}

void MainWindow::checkDiskAlerts(QVector<DiskPartitionData> parts, QVector<DiskDeviceData>) {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (const auto& p : parts) {
        if (p.usagePercent < 85) continue;
        const QString key = "disk_" + p.mountPoint;
        if (now - m_lastAlertTime.value(key, 0) < ALERT_COOLDOWN_MS) continue;
        ToastManager::instance().warning(
            QString("Disk %1 at %2% capacity").arg(p.mountPoint)
                .arg(p.usagePercent, 0, 'f', 1));
        m_lastAlertTime[key] = now;
    }
}
