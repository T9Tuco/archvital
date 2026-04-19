#pragma once

#include <QAbstractButton>
#include <QCheckBox>
#include <QHash>
#include <QList>
#include <QSettings>
#include <QVBoxLayout>
#include <QWidget>

class SettingsSection : public QWidget {
    Q_OBJECT
public:
    explicit SettingsSection(QSettings* settings, QWidget* parent = nullptr);

signals:
    void themeChanged();
    void sectionVisibilityChanged(int id, bool visible);

private:
    struct RoleMeta {
        QString key;
        QString label;
        QString description;
    };

    void buildAppearanceCard(QVBoxLayout* root);
    void buildVisibilityCard(QVBoxLayout* root);
    void addRolePicker(QVBoxLayout* root, const RoleMeta& role,
                       const QStringList& palette);
    void syncVisibilityChecks();
    void updateSwatchStyles();

    QSettings* m_settings{nullptr};
    QHash<QString, QList<QAbstractButton*>> m_roleButtons;
    QHash<int, QCheckBox*> m_visibilityChecks;
};
