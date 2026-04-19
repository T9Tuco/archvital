#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QHash>
#include "core/datastructs.h"
#include "ui/sparkline.h"

class NetworkInterfaceCard : public QWidget {
    Q_OBJECT
public:
    explicit NetworkInterfaceCard(const QString& name, QWidget* parent = nullptr);
    void update(const NetworkInterfaceData& data);

private:
    QLabel*          m_nameLabel{nullptr};
    QLabel*          m_statusDot{nullptr};
    QLabel*          m_ipLabel{nullptr};
    QLabel*          m_downLabel{nullptr};
    QLabel*          m_upLabel{nullptr};
    QLabel*          m_totalLabel{nullptr};
    SparklineWidget* m_downGraph{nullptr};
    SparklineWidget* m_upGraph{nullptr};
};

class NetworkSection : public QWidget {
    Q_OBJECT
public:
    explicit NetworkSection(QWidget* parent = nullptr);

private slots:
    void onNetworkUpdated(QVector<NetworkInterfaceData> ifaces);

private:
    QVBoxLayout* m_cardsLayout{nullptr};
    QWidget*     m_cardsContainer{nullptr};
    QHash<QString, NetworkInterfaceCard*> m_cards;

    static QString fmtSpeed(long long bps);
    static QString fmtBytes(long long b);
};
