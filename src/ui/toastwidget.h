#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>

enum class ToastType { Info, Success, Warning, Error };

class ToastWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(float opacity READ opacity WRITE setOpacity)
public:
    explicit ToastWidget(const QString& message, ToastType type, QWidget* parent = nullptr);

    float opacity() const { return m_opacity; }
    void  setOpacity(float v);
    void  slideIn(const QPoint& finalPos);

signals:
    void dismissed();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QLabel*            m_label{nullptr};
    QPropertyAnimation* m_slideAnim{nullptr};
    QPropertyAnimation* m_fadeAnim{nullptr};
    QTimer*            m_dismissTimer{nullptr};
    float              m_opacity{1.0f};
    QColor             m_accentColor;
};
