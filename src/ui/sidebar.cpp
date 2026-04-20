#include "sidebar.h"

#include <QPainter>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QDialog>
#include <QUrl>
#include "ui/stylesheet.h"

// one sidebar button, custom-painted so it keeps the look we want

SidebarButton::SidebarButton(const QString& abbr, const QString& label,
                             int id, QWidget* parent)
    : QPushButton(parent), m_abbr(abbr), m_label(label), m_id(id)
{
    setFlat(true);
    setCursor(Qt::PointingHandCursor);
    setFixedHeight(36);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setToolTip(label);
}

void SidebarButton::setActive(bool active)    { m_active = active;    update(); }
void SidebarButton::setCollapsed(bool c)      { m_collapsed = c;      update(); }
void SidebarButton::enterEvent(QEnterEvent*)  { m_hovered = true;     update(); }
void SidebarButton::leaveEvent(QEvent*)       { m_hovered = false;    update(); }

void SidebarButton::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF bg = QRectF(rect()).adjusted(6, 3, -6, -3);

    if (m_active) {
        QColor selected = Style::selectedColor();
        selected.setAlpha(190);
        p.fillRect(bg, selected);
        // little accent stripe so the active page is obvious at a glance
        p.fillRect(QRectF(6, 9, 2, height() - 18), Style::accentColor());
    } else if (m_hovered) {
        QColor hover = Style::hoverColor();
        hover.setAlpha(180);
        p.fillRect(bg, hover);
    }

    // the short label always stays, even when the sidebar squishes down
    const int abbrW = m_collapsed ? width() : 34;
    const int abbrX = m_collapsed ? 0 : 12;

    QFont abbrFont("Inter,Noto Sans,sans-serif");
    abbrFont.setPixelSize(10);
    abbrFont.setWeight(QFont::Bold);
    abbrFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    p.setFont(abbrFont);
    p.setPen(m_active ? Style::accentColor() : Style::textSecColor());
    p.drawText(QRect(abbrX, 0, abbrW, height()),
               Qt::AlignVCenter | Qt::AlignHCenter, m_abbr);

    // full label only shows when there is room for it
    if (!m_collapsed) {
        QFont labelFont("Inter,Noto Sans,sans-serif");
        labelFont.setPixelSize(12);
        labelFont.setWeight(m_active ? QFont::Medium : QFont::Normal);
        p.setFont(labelFont);
        p.setPen(m_active ? Style::textPriColor() : Style::textSecColor());
        p.drawText(QRect(abbrX + abbrW + 2, 0, width() - abbrX - abbrW - 8, height()),
                   Qt::AlignVCenter | Qt::AlignLeft, m_label);
    }
}

// the whole left rail

Sidebar::Sidebar(QWidget* parent) : QWidget(parent) {
    setFixedWidth(EXPANDED_W);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // brand row up top
    auto* brandWidget = new QWidget(this);
    brandWidget->setFixedHeight(48);
    auto* brandLayout = new QHBoxLayout(brandWidget);
    brandLayout->setContentsMargins(14, 0, 10, 0);
    brandLayout->setSpacing(0);

    auto* brandLabel = new QLabel("archvital", brandWidget);
    brandLabel->setStyleSheet(
        QString("color:%1; font-size:13px; font-weight:700; "
                "letter-spacing:1.6px; background:transparent;")
            .arg(Style::accent()));
    brandLayout->addWidget(brandLabel);
    brandLayout->addStretch();
    root->addWidget(brandWidget);

    // tiny spacer line so the top does not melt into the buttons
    auto* sep1 = new QWidget(this);
    sep1->setFixedHeight(1);
    sep1->setStyleSheet(QString("background:%1;").arg(Style::border()));
    root->addWidget(sep1);
    root->addSpacing(4);

    // section buttons, same order as the stacked pages
    for (int i = 0; i < AppSections::Count; i++) {
        auto* btn = new SidebarButton(
            AppSections::kSections[i].abbr, AppSections::kSections[i].label, i, this);
        connect(btn, &QPushButton::clicked, this, [this, i]{
            emit sectionRequested(i);
        });
        m_buttons.append(btn);
        root->addWidget(btn);
    }

    root->addStretch();

    // bottom row gets version text and the collapse toggle
    auto* sep2 = new QWidget(this);
    sep2->setFixedHeight(1);
    sep2->setStyleSheet(QString("background:%1;").arg(Style::border()));
    root->addWidget(sep2);

    auto* bottomWidget = new QWidget(this);
    bottomWidget->setFixedHeight(34);
    auto* bottomLayout = new QHBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(14, 0, 7, 0);
    bottomLayout->setSpacing(0);

    m_verBtn = new QPushButton(QString("v" APP_VERSION), bottomWidget);
    m_verBtn->setFlat(true);
    m_verBtn->setCursor(Qt::ArrowCursor);
    m_verBtn->setStyleSheet(QString(
        "QPushButton{color:%1;font-size:10px;background:transparent;"
        "border:none;padding:0;text-align:left;}"
        "QPushButton:hover{color:%1;}")
        .arg(Style::textSec()));
    connect(m_verBtn, &QPushButton::clicked, this, [this]{
        if (m_updateAvailable) showUpdateDialog();
    });
    bottomLayout->addWidget(m_verBtn);
    bottomLayout->addStretch();

    m_collapseBtn = new QPushButton("<", bottomWidget);
    m_collapseBtn->setFlat(true);
    m_collapseBtn->setCursor(Qt::PointingHandCursor);
    m_collapseBtn->setFixedSize(24, 24);
    m_collapseBtn->setStyleSheet(QString(
        "QPushButton{color:%1;font-size:12px;font-weight:bold;"
        "background:transparent;border:none;border-radius:5px;}"
        "QPushButton:hover{color:%2;background:%3;}")
        .arg(Style::textSec(), Style::accent(), Style::hover()));
    connect(m_collapseBtn, &QPushButton::clicked, this, [this]{
        setCollapsed(!m_collapsed);
        emit collapseToggled(m_collapsed);
    });
    bottomLayout->addWidget(m_collapseBtn);
    root->addWidget(bottomWidget);

    setActiveSection(0);

    QTimer::singleShot(3000, this, &Sidebar::checkForUpdates);
}

void Sidebar::setActiveSection(int id) {
    for (auto* btn : m_buttons)
        btn->setActive(btn->sectionId() == id);
}

void Sidebar::setSectionVisible(int id, bool visible) {
    if (id < 0 || id >= m_buttons.size()) return;
    m_buttons[id]->setVisible(visible);
}

void Sidebar::setCollapsed(bool c) {
    m_collapsed = c;
    setFixedWidth(c ? COLLAPSED_W : EXPANDED_W);
    m_collapseBtn->setText(c ? ">" : "<");
    m_verBtn->setVisible(!c);
    for (auto* btn : m_buttons)
        btn->setCollapsed(c);
}

void Sidebar::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), Style::surfaceColor());
    // soft border on the right so the content area has a clean edge
    p.setPen(QPen(Style::borderColor(), 1));
    p.drawLine(width() - 1, 0, width() - 1, height());
}

void Sidebar::checkForUpdates() {
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &Sidebar::onUpdateCheckFinished);

    QNetworkRequest req(QUrl("https://api.github.com/repos/T9Tuco/archvital/releases/latest"));
    req.setHeader(QNetworkRequest::UserAgentHeader, "archvital/" APP_VERSION);
    m_nam->get(req);
}

void Sidebar::onUpdateCheckFinished(QNetworkReply* reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) return;

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isNull()) return;

    QString latest = doc.object()["tag_name"].toString();
    if (latest.startsWith('v')) latest.remove(0, 1);
    if (latest.isEmpty()) return;

    auto toNums = [](const QString& v) -> QVector<int> {
        QVector<int> n;
        for (const QString& p : v.split('.')) n.append(p.toInt());
        while (n.size() < 3) n.append(0);
        return n;
    };

    const auto cur = toNums(QString(APP_VERSION));
    const auto lat = toNums(latest);

    const bool newer = lat[0] > cur[0]
        || (lat[0] == cur[0] && lat[1] > cur[1])
        || (lat[0] == cur[0] && lat[1] == cur[1] && lat[2] > cur[2]);

    if (!newer) return;

    m_updateAvailable = true;
    m_latestVersion   = latest;

    m_verBtn->setText(QString("v" APP_VERSION " ↑"));
    m_verBtn->setCursor(Qt::PointingHandCursor);
    m_verBtn->setToolTip(QString("v%1 available — click to update").arg(latest));
    m_verBtn->setStyleSheet(QString(
        "QPushButton{color:%1;font-size:10px;background:transparent;"
        "border:none;padding:0;text-align:left;text-decoration:underline;}"
        "QPushButton:hover{color:%2;}")
        .arg(Style::accent(), Style::accent()));
}

void Sidebar::showUpdateDialog() {
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("Update Available");
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setFixedWidth(460);
    dlg->setStyleSheet("background:#121212; color:#e0e0e0;");

    auto* layout = new QVBoxLayout(dlg);
    layout->setSpacing(12);
    layout->setContentsMargins(20, 20, 20, 20);

    auto* title = new QLabel(
        QString("archvital v%1 is available").arg(m_latestVersion), dlg);
    title->setStyleSheet("font-size:14px;font-weight:600;color:#e0e0e0;background:transparent;");
    layout->addWidget(title);

    auto* desc = new QLabel("Run this in your terminal to update:", dlg);
    desc->setStyleSheet("font-size:12px;color:#9e9e9e;background:transparent;");
    layout->addWidget(desc);

    auto* cmd = new QLabel(
        "curl -fsSL https://raw.githubusercontent.com/"
        "T9Tuco/archvital/main/install.sh | bash", dlg);
    cmd->setWordWrap(true);
    cmd->setTextInteractionFlags(Qt::TextSelectableByMouse);
    cmd->setStyleSheet(
        "font-family:'JetBrains Mono','Fira Mono',monospace;"
        "font-size:11px;color:#e0e0e0;background:#1a1a1a;"
        "border:1px solid #2a2a2a;border-radius:6px;padding:10px;");
    layout->addWidget(cmd);

    auto* closeBtn = new QPushButton("Close", dlg);
    closeBtn->setFlat(true);
    closeBtn->setStyleSheet(QString(
        "QPushButton{color:%1;font-size:12px;background:transparent;"
        "border:1px solid %2;border-radius:5px;padding:5px 14px;}"
        "QPushButton:hover{color:%3;border-color:%3;}")
        .arg(Style::textSec(), Style::border(), Style::accent()));
    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);

    dlg->exec();
}
