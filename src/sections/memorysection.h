#pragma once

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include "core/datastructs.h"
#include "ui/sparkline.h"
#include "ui/barwidget.h"

class MemorySection : public QWidget {
    Q_OBJECT
public:
    explicit MemorySection(QWidget* parent = nullptr);

private slots:
    void onMemoryUpdated(MemoryData data);
    void onProcessesUpdated(QVector<ProcessData> procs);

private:
    QLabel*          m_usedLabel{nullptr};
    QLabel*          m_totalLabel{nullptr};
    BarWidget*       m_bar{nullptr};
    QLabel*          m_pctLabel{nullptr};
    QLabel*          m_freeLabel{nullptr};
    QLabel*          m_availLabel{nullptr};
    QLabel*          m_buffLabel{nullptr};
    QLabel*          m_swapLabel{nullptr};
    SparklineWidget* m_graph{nullptr};
    QTableWidget*    m_procTable{nullptr};

};
