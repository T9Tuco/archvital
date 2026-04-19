#include "metriccard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include "stylesheet.h"

MetricCard::MetricCard(const QString& title, QWidget* parent) : QWidget(parent) {
    setMinimumSize(136, 88);
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QString(
        "MetricCard { background: %1; border: 1px solid %2; border-radius: 8px; }")
        .arg(Style::card(), Style::border()));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 6);
    root->setSpacing(0);

    m_accentLine = new QWidget(this);
    m_accentLine->setFixedHeight(2);
    QColor accentLine = Style::accentColor();
    accentLine.setAlpha(70);
    m_accentLine->setStyleSheet(QString(
        "background: %1; border-top-left-radius: 8px; border-top-right-radius: 8px;")
        .arg(accentLine.name(QColor::HexArgb)));
    root->addWidget(m_accentLine);

    root->addSpacing(6);

    auto* pad = new QVBoxLayout();
    pad->setContentsMargins(10, 0, 10, 0);
    pad->setSpacing(1);

    m_titleLabel = new QLabel(title.toUpper(), this);
    m_titleLabel->setStyleSheet(QString(
        "color: %1; font-size: 9px; font-weight: 700; letter-spacing: 1.0px;")
        .arg(Style::textSec()));
    pad->addWidget(m_titleLabel);

    pad->addSpacing(1);

    auto* valueRow = new QHBoxLayout();
    valueRow->setSpacing(2);
    valueRow->setContentsMargins(0, 0, 0, 0);

    m_valueLabel = new QLabel("—", this);
    m_valueLabel->setStyleSheet(QString(
        "color: %1; font-size: 18px; font-weight: 700; "
        "font-family: 'JetBrains Mono', 'Fira Mono', 'Courier New', monospace;")
        .arg(Style::accent()));
    valueRow->addWidget(m_valueLabel, 0, Qt::AlignBottom);

    m_unitLabel = new QLabel(this);
    m_unitLabel->setStyleSheet(QString(
        "color: %1; font-size: 10px; padding-bottom: 1px;")
        .arg(Style::textSec()));
    m_unitLabel->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
    valueRow->addWidget(m_unitLabel, 0, Qt::AlignBottom);
    valueRow->addStretch();
    pad->addLayout(valueRow);

    m_subtextLabel = new QLabel(this);
    m_subtextLabel->setStyleSheet(QString("color: %1; font-size: 9px;")
        .arg(Style::textSec()));
    m_subtextLabel->setVisible(false);
    pad->addWidget(m_subtextLabel);

    pad->addStretch();

    m_sparkline = new SparklineWidget(this);
    m_sparkline->setFixedHeight(16);
    m_sparkline->setFixedMax(100.0);
    m_sparkline->setFillAlpha(16);
    pad->addWidget(m_sparkline);

    root->addLayout(pad);
}

void MetricCard::setValue(const QString& value, const QString& unit) {
    m_valueLabel->setText(value);
    m_unitLabel->setText(unit.isEmpty() ? "" : " " + unit);
    m_unitLabel->setVisible(!unit.isEmpty());
}

void MetricCard::setSubtext(const QString& text) {
    m_subtextLabel->setText(text);
    m_subtextLabel->setVisible(!text.isEmpty());
}

void MetricCard::setAccentColor(const QColor& c) {
    m_accent = c;
    m_valueLabel->setStyleSheet(QString(
        "color: %1; font-size: 18px; font-weight: 700; "
        "font-family: 'JetBrains Mono', 'Fira Mono', 'Courier New', monospace;")
        .arg(c.name()));
    QColor lineColor = c;
    lineColor.setAlpha(68);
    m_accentLine->setStyleSheet(QString(
        "background: %1; border-top-left-radius: 8px; border-top-right-radius: 8px;")
        .arg(lineColor.name(QColor::HexArgb)));
    m_sparkline->setColor(c);
}

void MetricCard::updateHistory(const QVector<double>& history) {
    m_sparkline->setValues(history);
}

void MetricCard::setFixedMax(double max) {
    m_sparkline->setFixedMax(max);
}
