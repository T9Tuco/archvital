#include "sidebar.h"

#include <QPainter>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSizePolicy>
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

    auto* verLabel = new QLabel("v1.0.0", bottomWidget);
    verLabel->setStyleSheet(QString("color:%1; font-size:10px; background:transparent;")
        .arg(Style::textSec()));
    bottomLayout->addWidget(verLabel);
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
