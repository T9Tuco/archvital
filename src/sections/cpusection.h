#pragma once

#include <QWidget>
#include <QVector>
#include <QLabel>
#include "core/datastructs.h"
#include "ui/sparkline.h"
#include "ui/barwidget.h"

class CpuSection : public QWidget {
    Q_OBJECT
public:
    explicit CpuSection(QWidget* parent = nullptr);

private slots:
    void onCpuUpdated(CpuData data);

private:
    struct CoreRow {
        QLabel*     labelId{nullptr};
        BarWidget*  bar{nullptr};
        QLabel*     pctLabel{nullptr};
        QLabel*     freqLabel{nullptr};
        QLabel*     tempLabel{nullptr};
    };

    QLabel*          m_overallPctLabel{nullptr};
    BarWidget*       m_overallBar{nullptr};
    QLabel*          m_modelLabel{nullptr};
    QLabel*          m_archLabel{nullptr};
    QLabel*          m_tempLabel{nullptr};
    QLabel*          m_loadLabel{nullptr};
    SparklineWidget* m_loadGraph{nullptr};
    QVector<CoreRow> m_coreRows;
    QWidget*         m_coresContainer{nullptr};
    bool             m_coreRowsBuilt{false};

    void buildCoreRows(int count);
};
