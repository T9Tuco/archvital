#pragma once

#include <array>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QVector>

class QNetworkAccessManager;
class QNetworkReply;

namespace AppSections {
enum Id {
    Overview = 0,
    Cpu,
    Memory,
    Gpu,
    Disk,
    Network,
    Tasks,
    Diagnostics,
    Settings,
    Count
};

struct Meta {
    const char* abbr;
    const char* label;
    const char* key;
};

inline constexpr std::array<Meta, Count> kSections{{
    { "OV",  "Overview",    "overview"    },
    { "CPU", "CPU",         "cpu"         },
    { "MEM", "Memory",      "memory"      },
    { "GPU", "GPU",         "gpu"         },
    { "DSK", "Disks",       "disk"        },
    { "NET", "Network",     "network"     },
    { "TSK", "Tasks",       "tasks"       },
    { "DX",  "Diagnostics", "diagnostics" },
    { "SET", "Settings",    "settings"    },
}};
}

class SidebarButton : public QPushButton {
    Q_OBJECT
public:
    SidebarButton(const QString& abbr, const QString& label, int id, QWidget* parent = nullptr);
    void setActive(bool active);
    void setCollapsed(bool collapsed);
    int  sectionId() const { return m_id; }

protected:
    void paintEvent(QPaintEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    QString m_abbr;
    QString m_label;
    int     m_id;
    bool    m_active{false};
    bool    m_collapsed{false};
    bool    m_hovered{false};
};

class Sidebar : public QWidget {
    Q_OBJECT
public:
    explicit Sidebar(QWidget* parent = nullptr);

    void setActiveSection(int id);
    void setCollapsed(bool collapsed);
    void setSectionVisible(int id, bool visible);
    bool isCollapsed() const { return m_collapsed; }

protected:
    void paintEvent(QPaintEvent*) override;

signals:
    void sectionRequested(int id);
    void collapseToggled(bool collapsed);

private slots:
    void onUpdateCheckFinished(QNetworkReply* reply);

private:
    void checkForUpdates();
    void showUpdateDialog();

    QVector<SidebarButton*> m_buttons;
    QPushButton*            m_collapseBtn{nullptr};
    QPushButton*            m_verBtn{nullptr};
    QNetworkAccessManager*  m_nam{nullptr};
    bool                    m_collapsed{false};
    bool                    m_updateAvailable{false};
    QString                 m_latestVersion;

    static constexpr int EXPANDED_W  = 152;
    static constexpr int COLLAPSED_W = 46;
};
