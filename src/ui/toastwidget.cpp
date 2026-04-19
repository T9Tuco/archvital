#include "toastwidget.h"

#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>

ToastWidget::ToastWidget(const QString& message, ToastType type, QWidget* parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedHeight(52);
    setMinimumWidth(260);
    setMaximumWidth(460);

    switch (type) {
        case ToastType::Success: m_accentColor = QColor("#00e5ff"); break;
        case ToastType::Warning: m_accentColor = QColor("#ff9800"); break;
        case ToastType::Error:   m_accentColor = QColor("#ff1744"); break;
        default:                 m_accentColor = QColor("#651fff"); break;
    }

    QString icon;
    switch (type) {
        case ToastType::Success: icon = "✓"; break;
        case ToastType::Warning: icon = "⚠"; break;
        case ToastType::Error:   icon = "✗"; break;
        default:                 icon = "ℹ"; break;
    }

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->setSpacing(10);

    auto* iconLabel = new QLabel(icon, this);
    iconLabel->setStyleSheet(
        QString("color: %1; font-size: 16px; font-weight: bold; background: transparent;")
            .arg(m_accentColor.name()));
    layout->addWidget(iconLabel);

    m_label = new QLabel(message, this);
    m_label->setStyleSheet("color: #e0e0e0; font-size: 13px; background: transparent;");
    m_label->setWordWrap(false);
    layout->addWidget(m_label, 1);

    // Slide-in animation
    m_slideAnim = new QPropertyAnimation(this, "pos", this);
    m_slideAnim->setDuration(280);
    m_slideAnim->setEasingCurve(QEasingCurve::OutCubic);

    // Fade-out animation
    m_fadeAnim = new QPropertyAnimation(this, "opacity", this);
    m_fadeAnim->setDuration(250);
    m_fadeAnim->setStartValue(1.0f);
    m_fadeAnim->setEndValue(0.0f);
    connect(m_fadeAnim, &QPropertyAnimation::finished, this, &ToastWidget::dismissed);

    // Auto-dismiss after 4 seconds
    m_dismissTimer = new QTimer(this);
    m_dismissTimer->setSingleShot(true);
    m_dismissTimer->setInterval(4000);
    connect(m_dismissTimer, &QTimer::timeout, this, [this]{
        m_fadeAnim->start();
    });
}

void ToastWidget::setOpacity(float v) {
    m_opacity = v;
    update();
}

void ToastWidget::slideIn(const QPoint& finalPos) {
    QPoint start(finalPos.x() + width() + 20, finalPos.y());
    move(start);
    show();
    m_slideAnim->setStartValue(start);
    m_slideAnim->setEndValue(finalPos);
    m_slideAnim->start();
    m_dismissTimer->start();
}

void ToastWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setOpacity(static_cast<double>(m_opacity));

    QPainterPath path;
    path.addRoundedRect(rect(), 10, 10);

    // Background
    p.fillPath(path, QColor(0x1a, 0x1a, 0x1a, 230));

    // Accent left bar
    QPainterPath bar;
    bar.addRoundedRect(QRectF(0, 4, 4, height() - 8), 2, 2);
    p.fillPath(bar, m_accentColor);

    // Border
    p.setPen(QPen(QColor(0x2a, 0x2a, 0x2a), 1));
    p.drawPath(path);
}
