#pragma once

#include <QWidget>
#include <QTableView>
#include <QTreeWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QLabel>
#include <QTimer>
#include "core/datastructs.h"

class ProcessTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Col { PID=0, Name, User, CPU, MemMB, MemPct, Status, Started, COUNT };

    explicit ProcessTableModel(QObject* parent = nullptr);
    int      rowCount(const QModelIndex& = {})    const override;
    int      columnCount(const QModelIndex& = {}) const override;
    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const override;
    QVariant headerData(int, Qt::Orientation, int role = Qt::DisplayRole) const override;
    void     setProcesses(const QVector<ProcessData>& procs);
    const ProcessData& processAt(int row) const { return m_procs[row]; }

    static QString colText(const ProcessData& p, int col);

private:
    QVector<ProcessData> m_procs;
};

class ProcessFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit ProcessFilterProxy(QObject* parent = nullptr);
    void setShowKernelThreads(bool show);

protected:
    bool filterAcceptsRow(int row, const QModelIndex& parent) const override;

private:
    bool m_showKernel{false};
};

class TaskSection : public QWidget {
    Q_OBJECT
public:
    explicit TaskSection(QWidget* parent = nullptr);

private slots:
    void onProcessesUpdated(QVector<ProcessData> procs);
    void showContextMenu(const QPoint& pos);
    void sendSignal(int pid, int sig, const QString& actionName);
    void rebuildTree();

private:
    ProcessTableModel*   m_model{nullptr};
    ProcessFilterProxy*  m_proxy{nullptr};
    QTableView*          m_tableView{nullptr};
    QTreeWidget*         m_treeView{nullptr};
    QStackedWidget*      m_viewStack{nullptr};
    QLineEdit*           m_searchBox{nullptr};
    QCheckBox*           m_kernelCheck{nullptr};
    QPushButton*         m_treeToggle{nullptr};
    QLabel*              m_countLabel{nullptr};
    QTimer*              m_filterTimer{nullptr};
    bool                 m_treeMode{false};
    QVector<ProcessData> m_latestProcs;
    QString              m_pendingFilter;
};
