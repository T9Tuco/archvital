#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include "core/datastructs.h"

class DiskSection : public QWidget {
    Q_OBJECT
public:
    explicit DiskSection(QWidget* parent = nullptr);

private slots:
    void onDiskUpdated(QVector<DiskPartitionData> partitions,
                       QVector<DiskDeviceData> devices);

private:
    QVBoxLayout* m_partLayout{nullptr};
    QVBoxLayout* m_devLayout{nullptr};
    QWidget*     m_partContainer{nullptr};
    QWidget*     m_devContainer{nullptr};

};
