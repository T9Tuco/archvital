#include "sparkline.h"

#include <QPainter>
#include <QPainterPath>
#include <algorithm>

SparklineWidget::SparklineWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setMinimumSize(60, 30);
}

void SparklineWidget::setValues(const QVector<double>& values) {
    m_values = values;
    update();
}

void SparklineWidget::addValue(double v) {
    m_values.append(v);
    if (m_values.size() > 60) m_values.removeFirst();
    update();
}

void SparklineWidget::setColor(const QColor& c)  { m_color = c;   update(); }
void SparklineWidget::setFixedMax(double max)     { m_fixedMax = max; update(); }
void SparklineWidget::setFillAlpha(int alpha)     { m_fillAlpha = alpha; update(); }

QSize SparklineWidget::sizeHint()        const { return {120, 40}; }
QSize SparklineWidget::minimumSizeHint() const { return {60,  24}; }

void SparklineWidget::paintEvent(QPaintEvent*) {
    if (m_values.size() < 2) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int   w  = width();
    const int   h  = height();
    const int   n  = m_values.size();

    double maxV = m_fixedMax > 0 ? m_fixedMax :
        *std::max_element(m_values.begin(), m_values.end());
    if (maxV <= 0) maxV = 1.0;

    auto xOf = [&](int i) -> double {
        return i * (w - 1) / static_cast<double>(n - 1);
    };
    auto yOf = [&](double v) -> double {
        return h - 1 - (v / maxV) * (h - 2);
    };

    QPainterPath linePath, fillPath;
    fillPath.moveTo(xOf(0), h);
    fillPath.lineTo(xOf(0), yOf(m_values[0]));

    linePath.moveTo(xOf(0), yOf(m_values[0]));
    for (int i = 1; i < n; i++) {
        double x = xOf(i), y = yOf(m_values[i]);
        linePath.lineTo(x, y);
        fillPath.lineTo(x, y);
    }
    fillPath.lineTo(xOf(n - 1), h);
    fillPath.closeSubpath();

    QColor fill = m_color;
    fill.setAlpha(m_fillAlpha);
    p.fillPath(fillPath, fill);

    QPen pen(m_color, 1.5f);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawPath(linePath);
}
