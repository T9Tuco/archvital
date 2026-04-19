#pragma once

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include "core/datastructs.h"
#include "ui/sparkline.h"

class GpuSection : public QWidget {
    Q_OBJECT
public:
    explicit GpuSection(QWidget* parent = nullptr);

private slots:
    void onGpuUpdated(GpuData data);

private:
    QWidget* m_noGpuMsg{nullptr};
    QWidget* m_dataWidget{nullptr};

    QLabel* m_driverLabel{nullptr};
    QLabel* m_tempEdgeLabel{nullptr};
    QLabel* m_tempJunctLabel{nullptr};
    QLabel* m_tempMemLabel{nullptr};
    QLabel* m_fanRpmLabel{nullptr};
    QLabel* m_fanPctLabel{nullptr};
    QLabel* m_powerLabel{nullptr};
    QLabel* m_sclkLabel{nullptr};
    QLabel* m_mclkLabel{nullptr};
    QLabel* m_voltLabel{nullptr};
    QLabel* m_vramLabel{nullptr};
    QProgressBar* m_powerBar{nullptr};
    QProgressBar* m_vramBar{nullptr};
    SparklineWidget* m_tempGraph{nullptr};
};
