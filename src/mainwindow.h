#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QSettings>
#include <QHash>
#include "core/datastructs.h"
#include "ui/sidebar.h"
#include "ui/toastmanager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void moveEvent(QMoveEvent*) override;

private slots:
    void onSectionRequested(int id);
    void onThemeChanged();
    void onSectionVisibilityChanged(int id, bool visible);
    void checkTemperatureAlerts(CpuData data);
    void checkDiskAlerts(QVector<DiskPartitionData> parts,
                         QVector<DiskDeviceData> devs);

private:
    void setupUI();
    void saveGeometry();
    void restoreGeometry();
    void applyTheme();
    void applySectionVisibility();
    bool isSectionVisible(int id) const;
    int firstVisibleSection() const;

    Sidebar*        m_sidebar{nullptr};
    QStackedWidget* m_stack{nullptr};
    QSettings       m_settings{"archvital", "archvital"};
    int             m_currentSection{0};

    // Alert throttling: avoid spamming toasts
    QHash<QString, qint64> m_lastAlertTime;
    static constexpr qint64 ALERT_COOLDOWN_MS = 60000;
};
