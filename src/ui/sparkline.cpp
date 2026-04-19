#include "sparkline.h"

#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include "stylesheet.h"

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

    const QRectF plot = rect().adjusted(m_paddingX, m_paddingY, -m_paddingX, -m_paddingY);
    const double w  = plot.width();
    const double h  = plot.height();
    const int   n  = m_values.size();

    double maxV = m_fixedMax > 0 ? m_fixedMax :
        *std::max_element(m_values.begin(), m_values.end());
    if (maxV <= 0) maxV = 1.0;
    double minV = 0.0;
    if (m_fixedMax <= 0) {
        minV = *std::min_element(m_values.begin(), m_values.end());
        if (maxV - minV < 0.001) minV = 0.0;
    }

    QColor gridColor = m_color;
    gridColor.setAlpha(22);
    p.setPen(QPen(gridColor, 1.0));
    for (int i = 1; i <= 2; i++) {
        const double y = plot.top() + (plot.height() * i / 3.0);
        p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
    }

    auto xOf = [&](int i) -> double {
        return plot.left() + i * w / static_cast<double>(n - 1);
    };
    auto yOf = [&](double v) -> double {
        const double range = std::max(0.001, maxV - minV);
        const double ratio = std::clamp((v - minV) / range, 0.0, 1.0);
        return plot.bottom() - ratio * h;
    };

    QPainterPath linePath, fillPath;
    fillPath.moveTo(xOf(0), plot.bottom());
    fillPath.lineTo(xOf(0), yOf(m_values[0]));

    linePath.moveTo(xOf(0), yOf(m_values[0]));
    for (int i = 1; i < n; i++) {
        double x = xOf(i), y = yOf(m_values[i]);
        linePath.lineTo(x, y);
        fillPath.lineTo(x, y);
    }
    fillPath.lineTo(xOf(n - 1), plot.bottom());
    fillPath.closeSubpath();

    QColor fill = m_color;
    fill.setAlpha(m_fillAlpha);
    p.fillPath(fillPath, fill);

    QColor lineColor = m_color.lighter(110);
    QPen pen(lineColor, 1.25f);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawPath(linePath);

    QColor dotColor = lineColor;
    dotColor.setAlpha(220);
    p.setPen(Qt::NoPen);
    p.setBrush(dotColor);
    p.drawEllipse(QPointF(xOf(n - 1), yOf(m_values.back())), 2.2, 2.2);
}
