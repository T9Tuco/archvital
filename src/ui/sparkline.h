#pragma once

#include <QWidget>
#include <QVector>
#include <QColor>

class SparklineWidget : public QWidget {
    Q_OBJECT
public:
    explicit SparklineWidget(QWidget* parent = nullptr);

    void setValues(const QVector<double>& values);
    void addValue(double v);
    void setColor(const QColor& c);
    void setFixedMax(double max);   // 0 = auto-scale
    void setFillAlpha(int alpha);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QVector<double> m_values;
    QColor          m_color{0x00, 0xe5, 0xff};
    double          m_fixedMax{100.0};
    int             m_fillAlpha{40};
};
