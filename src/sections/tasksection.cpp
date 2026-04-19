#include "tasksection.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QClipboard>
#include <QGuiApplication>
#include <QLabel>
#include <QProcess>
#include <QBrush>
#include <QColor>
#include <signal.h>
#include "core/systemmonitor.h"
#include "ui/stylesheet.h"
#include "ui/toastmanager.h"

// ── ProcessTableModel ─────────────────────────────────────────────────────────

ProcessTableModel::ProcessTableModel(QObject* parent) : QAbstractTableModel(parent) {}

int ProcessTableModel::rowCount(const QModelIndex&)    const { return m_procs.size(); }
int ProcessTableModel::columnCount(const QModelIndex&) const { return Col::COUNT; }

QVariant ProcessTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
        case PID:    return "PID";
        case Name:   return "Name";
        case User:   return "User";
        case CPU:    return "CPU %";
        case MemMB:  return "MEM (MB)";
        case MemPct: return "MEM %";
        case Status: return "Status";
        case Started: return "Started";
    }
    return {};
}

QVariant ProcessTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_procs.size()) return {};
    const auto& p = m_procs[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case PID:    return p.pid;
            case Name:   return p.name;
            case User:   return p.user;
            case CPU:    return QString::number(p.cpuPercent, 'f', 1);
            case MemMB:  return QString::number(p.memBytes / 1024 / 1024);
            case MemPct: return QString::number(p.memPercent, 'f', 1);
            case Status: return p.status;
            case Started: return p.startTime;
        }
    }

    if (role == Qt::ForegroundRole) {
        if (p.status == "Z") return QBrush(Style::errorColor());
        if (p.cpuPercent > 50) return QBrush(Style::warningColor());
    }

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == PID || index.column() == CPU ||
            index.column() == MemMB || index.column() == MemPct)
            return QVariant(static_cast<int>(Qt::AlignRight | Qt::AlignVCenter));
        return QVariant(static_cast<int>(Qt::AlignVCenter));
    }

    if (role == Qt::UserRole) {
        // Return raw numeric value for sorting numeric columns
        switch (index.column()) {
            case PID:    return p.pid;
            case CPU:    return p.cpuPercent;
            case MemMB:  return static_cast<long long>(p.memBytes / 1024 / 1024);
            case MemPct: return p.memPercent;
        }
    }

    return {};
}

void ProcessTableModel::setProcesses(const QVector<ProcessData>& procs) {
    beginResetModel();
    m_procs = procs;
    endResetModel();
}

// ── ProcessFilterProxy ────────────────────────────────────────────────────────

ProcessFilterProxy::ProcessFilterProxy(QObject* parent)
    : QSortFilterProxyModel(parent) {
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setFilterKeyColumn(-1);
}

void ProcessFilterProxy::setShowKernelThreads(bool show) {
    m_showKernel = show;
    beginFilterChange();
    endFilterChange();
}

bool ProcessFilterProxy::filterAcceptsRow(int row, const QModelIndex& parent) const {
    const auto* m = static_cast<const ProcessTableModel*>(sourceModel());
    if (!m_showKernel && m->processAt(row).isKernelThread) return false;
    return QSortFilterProxyModel::filterAcceptsRow(row, parent);
}

bool lessThanNumeric(const QModelIndex& left, const QModelIndex& right) {
    const auto lv = left.data(Qt::UserRole);
    const auto rv = right.data(Qt::UserRole);
    if (lv.isValid() && rv.isValid()) {
        if (lv.typeId() == QMetaType::Double)
            return lv.toDouble() < rv.toDouble();
        return lv.toLongLong() < rv.toLongLong();
    }
    return left.data().toString() < right.data().toString();
}

// ── TaskSection ───────────────────────────────────────────────────────────────

TaskSection::TaskSection(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Header
    auto* headerRow = new QHBoxLayout();
    headerRow->setContentsMargins(24, 16, 24, 10);
    headerRow->setSpacing(12);

    auto* headerLabel = new QLabel("Task Manager", this);
    headerLabel->setStyleSheet(
        "color:#e0e0e0;font-size:18px;font-weight:700;background:transparent;");
    headerRow->addWidget(headerLabel);
    headerRow->addStretch();

    m_countLabel = new QLabel("", this);
    m_countLabel->setStyleSheet("color:#555;font-size:12px;background:transparent;");
    headerRow->addWidget(m_countLabel);
    outer->addLayout(headerRow);

    // Toolbar
    auto* toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(24, 0, 24, 10);
    toolbar->setSpacing(10);

    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText("Filter by name or PID…");
    m_searchBox->setMinimumWidth(220);
    toolbar->addWidget(m_searchBox);

    m_kernelCheck = new QCheckBox("Kernel Threads", this);
    m_kernelCheck->setChecked(false);
    toolbar->addWidget(m_kernelCheck);

    m_treeToggle = new QPushButton("Tree View", this);
    m_treeToggle->setCheckable(true);
    toolbar->addWidget(m_treeToggle);

    toolbar->addStretch();
    outer->addLayout(toolbar);

    // Separator
    auto* sep = new QWidget(this);
    sep->setFixedHeight(1);
    sep->setStyleSheet("background:#2a2a2a;");
    outer->addWidget(sep);

    // View stack
    m_viewStack = new QStackedWidget(this);
    outer->addWidget(m_viewStack, 1);

    // Flat table view
    m_model = new ProcessTableModel(this);
    m_proxy = new ProcessFilterProxy(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setSortRole(Qt::UserRole);

    m_tableView = new QTableView(this);
    m_tableView->setModel(m_proxy);
    m_tableView->setSortingEnabled(true);
    m_tableView->sortByColumn(ProcessTableModel::CPU, Qt::DescendingOrder);
    m_tableView->horizontalHeader()->setSectionResizeMode(
        ProcessTableModel::Name, QHeaderView::Stretch);
    m_tableView->horizontalHeader()->setSectionResizeMode(
        ProcessTableModel::PID,  QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setSectionResizeMode(
        ProcessTableModel::User, QHeaderView::ResizeToContents);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tableView->setShowGrid(false);
    m_viewStack->addWidget(m_tableView);

    // Tree view
    m_treeView = new QTreeWidget(this);
    m_treeView->setColumnCount(ProcessTableModel::COUNT);
    QStringList treeHeaders = {"PID","Name","User","CPU %","MEM (MB)","MEM %","Status","Started"};
    m_treeView->setHeaderLabels(treeHeaders);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setAlternatingRowColors(true);
    m_viewStack->addWidget(m_treeView);

    // Connections
    connect(m_searchBox, &QLineEdit::textChanged, this, [this](const QString& t){
        m_proxy->setFilterFixedString(t);
    });
    connect(m_kernelCheck, &QCheckBox::toggled, this, [this](bool c){
        m_proxy->setShowKernelThreads(c);
        if (m_treeMode) rebuildTree();
    });
    connect(m_treeToggle, &QPushButton::toggled, this, [this](bool checked){
        m_treeMode = checked;
        m_viewStack->setCurrentIndex(checked ? 1 : 0);
        if (checked) rebuildTree();
    });
    connect(m_tableView, &QTableView::customContextMenuRequested,
            this, &TaskSection::showContextMenu);
    connect(m_treeView, &QTreeWidget::customContextMenuRequested,
            this, [this](const QPoint& pos){ showContextMenu(pos); });

    auto& mon = SystemMonitor::instance();
    connect(&mon, &SystemMonitor::processDataUpdated,
            this, &TaskSection::onProcessesUpdated);
}

void TaskSection::onProcessesUpdated(QVector<ProcessData> procs) {
    m_latestProcs = procs;
    m_model->setProcesses(procs);
    m_countLabel->setText(QString("%1 processes").arg(procs.size()));
    if (m_treeMode) rebuildTree();
}

void TaskSection::rebuildTree() {
    m_treeView->clear();
    const bool showKernel = m_kernelCheck->isChecked();
    const QString filter  = m_searchBox->text().toLower();

    QHash<int, QTreeWidgetItem*> items;

    auto makeItem = [&](const ProcessData& p) -> QTreeWidgetItem* {
        auto* item = new QTreeWidgetItem();
        item->setText(0, QString::number(p.pid));
        item->setText(1, p.name);
        item->setText(2, p.user);
        item->setText(3, QString::number(p.cpuPercent, 'f', 1));
        item->setText(4, QString::number(p.memBytes / 1024 / 1024));
        item->setText(5, QString::number(p.memPercent, 'f', 1));
        item->setText(6, p.status);
        item->setText(7, p.startTime);
        item->setData(0, Qt::UserRole, p.pid);
        if (p.status == "Z") {
            for (int c = 0; c < 8; c++)
                item->setForeground(c, QBrush(Style::errorColor()));
        } else if (p.cpuPercent > 50) {
            for (int c = 0; c < 8; c++)
                item->setForeground(c, QBrush(Style::warningColor()));
        }
        return item;
    };

    // Build by PID lookup
    for (const auto& p : m_latestProcs) {
        if (!showKernel && p.isKernelThread) continue;
        if (!filter.isEmpty() &&
            !p.name.toLower().contains(filter) &&
            !QString::number(p.pid).contains(filter)) continue;
        items[p.pid] = makeItem(p);
    }

    // Build tree
    for (const auto& p : m_latestProcs) {
        if (!items.contains(p.pid)) continue;
        auto* item = items[p.pid];
        if (items.contains(p.ppid) && p.ppid != p.pid)
            items[p.ppid]->addChild(item);
        else
            m_treeView->addTopLevelItem(item);
    }
    m_treeView->expandToDepth(1);
}

void TaskSection::showContextMenu(const QPoint& pos) {
    int pid = -1;
    QString procName;

    if (!m_treeMode) {
        const QModelIndex idx = m_tableView->indexAt(pos);
        if (!idx.isValid()) return;
        const QModelIndex src = m_proxy->mapToSource(idx);
        const auto& p = m_model->processAt(src.row());
        pid = p.pid;
        procName = p.name;
    } else {
        const auto* item = m_treeView->itemAt(pos);
        if (!item) return;
        pid = item->data(0, Qt::UserRole).toInt();
        procName = item->text(1);
    }
    if (pid < 0) return;

    QMenu menu(this);
    menu.addAction(QString("PID: %1 — %2").arg(pid).arg(procName))->setEnabled(false);
    menu.addSeparator();

    auto* killAct    = menu.addAction("Kill (SIGKILL)");
    auto* termAct    = menu.addAction("Terminate (SIGTERM)");
    auto* suspAct    = menu.addAction("Suspend (SIGSTOP)");
    auto* resumeAct  = menu.addAction("Resume (SIGCONT)");
    menu.addSeparator();
    auto* copyAct    = menu.addAction("Copy PID");
    auto* folderAct  = menu.addAction("Open /proc folder");

    connect(killAct,   &QAction::triggered, [this, pid]{ sendSignal(pid, SIGKILL, "Killed");     });
    connect(termAct,   &QAction::triggered, [this, pid]{ sendSignal(pid, SIGTERM, "Terminated"); });
    connect(suspAct,   &QAction::triggered, [this, pid]{ sendSignal(pid, SIGSTOP, "Suspended");  });
    connect(resumeAct, &QAction::triggered, [this, pid]{ sendSignal(pid, SIGCONT, "Resumed");    });
    connect(copyAct,   &QAction::triggered, [pid]{
        QGuiApplication::clipboard()->setText(QString::number(pid));
    });
    connect(folderAct, &QAction::triggered, [pid]{
        QProcess::startDetached("xdg-open", {QString("/proc/%1").arg(pid)});
    });

    const QPoint globalPos = m_treeMode ?
        m_treeView->mapToGlobal(pos) :
        m_tableView->mapToGlobal(pos);
    menu.exec(globalPos);
}

void TaskSection::sendSignal(int pid, int sig, const QString& actionName) {
    if (sig == SIGKILL || sig == SIGTERM) {
        const auto reply = QMessageBox::question(
            this,
            actionName + " process?",
            QString("%1 process %2?").arg(actionName).arg(pid),
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) return;
    }
    if (::kill(static_cast<pid_t>(pid), sig) == 0)
        ToastManager::instance().success(
            actionName + " PID " + QString::number(pid));
    else
        ToastManager::instance().error(
            "Failed to signal PID " + QString::number(pid) +
            ": " + QString::fromLocal8Bit(strerror(errno)));
}
