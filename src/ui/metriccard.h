#pragma once

#include <QWidget>
#include <QLabel>
#include "sparkline.h"

class MetricCard : public QWidget {
    Q_OBJECT
public:
    explicit MetricCard(const QString& title, QWidget* parent = nullptr);

    void setValue(const QString& value, const QString& unit = {});
    void setSubtext(const QString& text);
    void setAccentColor(const QColor& c);
    void updateHistory(const QVector<double>& history);
    void setFixedMax(double max);

private:
    QWidget*        m_accentLine{nullptr};
    QLabel*         m_titleLabel{nullptr};
    QLabel*         m_valueLabel{nullptr};
    QLabel*         m_unitLabel{nullptr};
    QLabel*         m_subtextLabel{nullptr};
    SparklineWidget* m_sparkline{nullptr};
    QColor          m_accent{0x00, 0xe5, 0xff};
};
