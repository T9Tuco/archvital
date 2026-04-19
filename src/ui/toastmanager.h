#pragma once

#include <QObject>
#include <QVector>
#include "toastwidget.h"

class ToastManager : public QObject {
    Q_OBJECT
public:
    static ToastManager& instance();

    void setParentWidget(QWidget* parent);

    void show(const QString& message, ToastType type = ToastType::Info);
    void info(const QString& msg)    { show(msg, ToastType::Info);    }
    void success(const QString& msg) { show(msg, ToastType::Success); }
    void warning(const QString& msg) { show(msg, ToastType::Warning); }
    void error(const QString& msg)   { show(msg, ToastType::Error);   }

private:
    explicit ToastManager(QObject* parent = nullptr);
    void repositionAll();

    QWidget*             m_parent{nullptr};
    QVector<ToastWidget*> m_toasts;
    static constexpr int MARGIN  = 16;
    static constexpr int SPACING = 8;
};
