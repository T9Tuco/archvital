#pragma once

#include <QWidget>
#include <QPainter>
#include <algorithm>
#include "stylesheet.h"

// tiny custom bar because Qt styles love getting clever at the worst time
class BarWidget : public QWidget {
public:
    explicit BarWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setFixedHeight(6);
        setAttribute(Qt::WA_OpaquePaintEvent);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void setValue(double pct) {
        m_pct = std::clamp(pct, 0.0, 100.0);
        update();
    }
    void setBarHeight(int h) { setFixedHeight(h); }
    void setColor(const QColor& c) { m_color = c; update(); }
    QSize sizeHint() const override { return {100, height()}; }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        const int w = width(), h = height();
        // dark track first
        p.fillRect(0, 0, w, h, QColor(0x22, 0x22, 0x22));
        // then the filled bit on top
        if (m_pct > 0.01) {
            const int fw = std::max(1, static_cast<int>(w * m_pct / 100.0));
            p.fillRect(0, 0, fw, h, m_color);
        }
    }

private:
    double m_pct{0.0};
    QColor m_color{Style::accentColor()};
};
