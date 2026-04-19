#include "toastmanager.h"
#include <QRect>

ToastManager& ToastManager::instance() {
    static ToastManager inst;
    return inst;
}

ToastManager::ToastManager(QObject* parent) : QObject(parent) {}

void ToastManager::setParentWidget(QWidget* parent) {
    m_parent = parent;
}

void ToastManager::show(const QString& message, ToastType type) {
    if (!m_parent) return;

    auto* toast = new ToastWidget(message, type, nullptr);
    // Fit text
    toast->adjustSize();
    int w = std::max(toast->width(), 260);
    toast->setFixedWidth(w);

    m_toasts.append(toast);

    connect(toast, &ToastWidget::dismissed, this, [this, toast]{
        m_toasts.removeAll(toast);
        toast->hide();
        toast->deleteLater();
        repositionAll();
    });

    repositionAll();

    const QRect pr = m_parent->geometry();
    const QPoint globalBR = m_parent->mapToGlobal(QPoint(pr.width(), pr.height()));
    const int x = globalBR.x() - toast->width() - MARGIN;
    const int y = globalBR.y() - toast->height() - MARGIN
                  - (m_toasts.size() - 1) * (toast->height() + SPACING);

    toast->slideIn(QPoint(x, y));
}

void ToastManager::repositionAll() {
    if (!m_parent || m_toasts.isEmpty()) return;
    const QRect pr = m_parent->geometry();
    const QPoint globalBR = m_parent->mapToGlobal(QPoint(pr.width(), pr.height()));

    for (int i = 0; i < m_toasts.size(); i++) {
        auto* t = m_toasts[i];
        const int x = globalBR.x() - t->width() - MARGIN;
        const int y = globalBR.y() - t->height() - MARGIN
                      - i * (t->height() + SPACING);
        t->move(x, y);
    }
}
