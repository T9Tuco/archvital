#include "networksection.h"

#include <QScrollArea>
#include <QSet>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include "core/systemmonitor.h"
#include "ui/stylesheet.h"

static QString fmtSpeed(long long bps) {
    if (bps < 1024)          return QString("%1 B/s").arg(bps);
    if (bps < 1024*1024)     return QString("%1 KB/s").arg(bps/1024);
    if (bps < 1024*1024*1024) return QString("%1 MB/s").arg(bps/1024/1024);
    return QString("%1 GB/s").arg(bps/1024/1024/1024);
}

static QString fmtTotal(long long b) {
    if (b < 1024)              return QString("%1 B").arg(b);
    if (b < 1024*1024)         return QString("%1 KB").arg(b/1024);
    if (b < 1024LL*1024*1024)  return QString("%1 MB").arg(b/1024/1024);
    return QString("%1 GB").arg(b/1024/1024/1024);
}

// ── NetworkInterfaceCard ──────────────────────────────────────────────────────

NetworkInterfaceCard::NetworkInterfaceCard(const QString& name, QWidget* parent)
    : QWidget(parent)
{
    setStyleSheet("background:#1a1a1a;border:1px solid #2a2a2a;border-radius:10px;");

    auto* vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(14, 12, 14, 12);
    vbox->setSpacing(10);

    // Header row
    auto* hdr = new QHBoxLayout();
    m_statusDot = new QLabel("●", this);
    m_statusDot->setStyleSheet("color:#555;font-size:10px;background:transparent;");
    hdr->addWidget(m_statusDot);

    m_nameLabel = new QLabel(name, this);
    m_nameLabel->setStyleSheet(
        "color:#e0e0e0;font-size:15px;font-weight:600;background:transparent;");
    hdr->addWidget(m_nameLabel);
    hdr->addStretch();
    m_ipLabel = new QLabel("—", this);
    m_ipLabel->setStyleSheet(
        "color:#9e9e9e;font-size:12px;"
        "font-family:'JetBrains Mono','Fira Mono',monospace;background:transparent;");
    hdr->addWidget(m_ipLabel);
    vbox->addLayout(hdr);

    // Speed row
    auto* speeds = new QHBoxLayout();
    auto* dlHdr  = new QLabel("↓ Download", this);
    dlHdr->setStyleSheet("color:#9e9e9e;font-size:11px;background:transparent;");
    speeds->addWidget(dlHdr);
    m_downLabel = new QLabel("0 B/s", this);
    m_downLabel->setStyleSheet(QString("color:%1;font-size:15px;font-weight:700;"
        "font-family:'JetBrains Mono','Fira Mono',monospace;background:transparent;")
        .arg(Style::accent()));
    speeds->addWidget(m_downLabel);
    speeds->addStretch();
    auto* ulHdr  = new QLabel("↑ Upload", this);
    ulHdr->setStyleSheet("color:#9e9e9e;font-size:11px;background:transparent;");
    speeds->addWidget(ulHdr);
    m_upLabel = new QLabel("0 B/s", this);
    m_upLabel->setStyleSheet(QString("color:%1;font-size:15px;font-weight:700;"
        "font-family:'JetBrains Mono','Fira Mono',monospace;background:transparent;")
        .arg(Style::info()));
    speeds->addWidget(m_upLabel);
    vbox->addLayout(speeds);

    // Dual graph
    auto* graphRow = new QHBoxLayout();
    m_downGraph = new SparklineWidget(this);
    m_downGraph->setFixedHeight(50);
    m_downGraph->setFixedMax(0); // auto-scale
    m_downGraph->setColor(Style::accentColor());
    graphRow->addWidget(m_downGraph);

    m_upGraph = new SparklineWidget(this);
    m_upGraph->setFixedHeight(50);
    m_upGraph->setFixedMax(0); // auto-scale
    m_upGraph->setColor(Style::infoColor());
    graphRow->addWidget(m_upGraph);
    vbox->addLayout(graphRow);

    // Totals
    m_totalLabel = new QLabel("", this);
    m_totalLabel->setStyleSheet("color:#555;font-size:11px;background:transparent;");
    vbox->addWidget(m_totalLabel);
}

void NetworkInterfaceCard::update(const NetworkInterfaceData& d) {
    m_statusDot->setStyleSheet(QString("color:%1;font-size:10px;background:transparent;")
        .arg(d.isUp ? Style::accent() : QString("#555")));
    m_ipLabel->setText(d.ipAddress.isEmpty() ? "—" : d.ipAddress);
    m_downLabel->setText(fmtSpeed(d.downloadBytesPerSec));
    m_upLabel->setText(fmtSpeed(d.uploadBytesPerSec));
    m_downGraph->setValues(d.downloadHistory);
    m_upGraph->setValues(d.uploadHistory);
    m_totalLabel->setText(
        QString("Total: ↓ %1  ↑ %2")
            .arg(fmtTotal(d.totalDownloadBytes))
            .arg(fmtTotal(d.totalUploadBytes)));
}

// ── NetworkSection ────────────────────────────────────────────────────────────

NetworkSection::NetworkSection(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto* header = new QLabel("Network", this);
    header->setStyleSheet(
        "color:#e0e0e0;font-size:18px;font-weight:700;"
        "padding:20px 24px 8px;background:transparent;");
    outer->addWidget(header);

    auto* sep = new QWidget(this);
    sep->setFixedHeight(1);
    sep->setStyleSheet("background:#2a2a2a;");
    outer->addWidget(sep);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);

    auto* inner = new QWidget();
    inner->setStyleSheet("background:transparent;");
    scroll->setWidget(inner);

    auto* vbox = new QVBoxLayout(inner);
    vbox->setContentsMargins(24, 16, 24, 24);
    vbox->setSpacing(14);

    m_cardsContainer = new QWidget(inner);
    m_cardsContainer->setStyleSheet("background:transparent;");
    m_cardsLayout = new QVBoxLayout(m_cardsContainer);
    m_cardsLayout->setContentsMargins(0, 0, 0, 0);
    m_cardsLayout->setSpacing(14);
    vbox->addWidget(m_cardsContainer);
    vbox->addStretch();

    auto& mon = SystemMonitor::instance();
    connect(&mon, &SystemMonitor::networkDataUpdated,
            this, &NetworkSection::onNetworkUpdated);
}

void NetworkSection::onNetworkUpdated(QVector<NetworkInterfaceData> ifaces) {
    QSet<QString> active;
    for (const auto& iface : ifaces) active.insert(iface.name);

    for (auto it = m_cards.begin(); it != m_cards.end(); ) {
        if (!active.contains(it.key())) {
            m_cardsLayout->removeWidget(it.value());
            it.value()->deleteLater();
            it = m_cards.erase(it);
        } else {
            ++it;
        }
    }

    for (const auto& iface : ifaces) {
        if (!m_cards.contains(iface.name)) {
            auto* card = new NetworkInterfaceCard(iface.name, m_cardsContainer);
            m_cards[iface.name] = card;
            m_cardsLayout->addWidget(card);
        }
        m_cards[iface.name]->update(iface);
    }
}
