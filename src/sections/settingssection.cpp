#include "settingssection.h"

#include <QButtonGroup>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "ui/sidebar.h"
#include "ui/stylesheet.h"

namespace {
QString sectionVisibleKey(int id) {
    return QString("sections/%1/visible").arg(AppSections::kSections[id].key);
}

bool sectionVisible(QSettings& settings, int id) {
    if (id == AppSections::Settings) return true;
    return settings.value(sectionVisibleKey(id), true).toBool();
}

QWidget* makeCard(QWidget* parent) {
    auto* card = new QWidget(parent);
    card->setStyleSheet(QString(
        "background:%1;border:1px solid %2;border-radius:12px;")
        .arg(Style::card(), Style::border()));
    return card;
}
}

SettingsSection::SettingsSection(QSettings* settings, QWidget* parent)
    : QWidget(parent), m_settings(settings) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* titleBar = new QWidget(this);
    titleBar->setFixedHeight(48);
    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(20, 0, 20, 0);

    auto* title = new QLabel("Settings", titleBar);
    title->setStyleSheet(QString(
        "color:%1;font-size:15px;font-weight:600;background:transparent;")
        .arg(Style::textPri()));
    titleLayout->addWidget(title);
    titleLayout->addStretch();
    outer->addWidget(titleBar);

    auto* divider = new QWidget(this);
    divider->setFixedHeight(1);
    divider->setStyleSheet(QString("background:%1;").arg(Style::border()));
    outer->addWidget(divider);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    outer->addWidget(scroll);

    auto* inner = new QWidget();
    scroll->setWidget(inner);

    auto* root = new QVBoxLayout(inner);
    root->setContentsMargins(18, 14, 18, 18);
    root->setSpacing(12);

    buildAppearanceCard(root);
    buildVisibilityCard(root);
    root->addStretch();
}

void SettingsSection::buildAppearanceCard(QVBoxLayout* root) {
    static const QStringList palette = {
        "#00e5ff", "#4cc9f0", "#06d6a0", "#ffd166",
        "#ff8c42", "#ff4d6d", "#c77dff", "#90be6d",
        "#f72585", "#577590", "#1f8a70", "#e76f51"
    };
    static const QStringList surfacePalette = {
        "#08090b", "#0d0d0d", "#101418", "#111111",
        "#141821", "#17151d", "#1a1c22", "#0f131a"
    };

    auto* card = makeCard(this);
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 16);
    layout->setSpacing(12);

    auto* title = new QLabel("Appearance", card);
    title->setStyleSheet(QString(
        "color:%1;font-size:14px;font-weight:600;background:transparent;")
        .arg(Style::textPri()));
    layout->addWidget(title);

    auto* subtitle = new QLabel(
        "Pick what each accent does. Main accent drives highlights, warning is for caution, "
        "error is for hot alerts, info is the second pop color, and the surface colors steer the overall mood.",
        card);
    subtitle->setWordWrap(true);
    subtitle->setStyleSheet(QString(
        "color:%1;font-size:11px;line-height:1.35;background:transparent;")
        .arg(Style::textSec()));
    layout->addWidget(subtitle);

    addRolePicker(layout, {"accent", "Main Accent", "active states, graphs, highlights"}, palette);
    addRolePicker(layout, {"warning", "Warning Accent", "hot but not on fire yet"}, palette);
    addRolePicker(layout, {"error", "Error Accent", "critical values and danger states"}, palette);
    addRolePicker(layout, {"info", "Info Accent", "secondary chart line and helper highlights"}, palette);
    addRolePicker(layout, {"bg", "App Background", "the outer shell of the whole app"}, surfacePalette);
    addRolePicker(layout, {"surface", "Surface Tone", "main panes and tables"}, surfacePalette);
    addRolePicker(layout, {"card", "Card Tone", "cards, panels and contained blocks"}, surfacePalette);
    addRolePicker(layout, {"selected", "Selected State", "active pills and selected table rows"}, palette);

    auto* resetBtn = new QPushButton("Reset Theme", card);
    connect(resetBtn, &QPushButton::clicked, this, [this] {
        const auto defaults = Style::defaultTheme();
        Style::saveTheme(*m_settings, defaults);
        updateSwatchStyles();
        emit themeChanged();
    });
    layout->addWidget(resetBtn, 0, Qt::AlignLeft);

    root->addWidget(card);
}

void SettingsSection::buildVisibilityCard(QVBoxLayout* root) {
    auto* card = makeCard(this);
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 16);
    layout->setSpacing(10);

    auto* title = new QLabel("Visible Sections", card);
    title->setStyleSheet(QString(
        "color:%1;font-size:14px;font-weight:600;background:transparent;")
        .arg(Style::textPri()));
    layout->addWidget(title);

    auto* subtitle = new QLabel(
        "Hide pages you do not care about. Settings stays visible so you never lock yourself out of the controls.",
        card);
    subtitle->setWordWrap(true);
    subtitle->setStyleSheet(QString(
        "color:%1;font-size:11px;background:transparent;")
        .arg(Style::textSec()));
    layout->addWidget(subtitle);

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(8);

    for (int i = 0; i < AppSections::Count; i++) {
        auto* box = new QCheckBox(AppSections::kSections[i].label, card);
        const bool forceVisible = i == AppSections::Settings;
        box->setChecked(forceVisible ? true : sectionVisible(*m_settings, i));
        box->setEnabled(!forceVisible);
        connect(box, &QCheckBox::toggled, this, [this, i](bool checked) {
            if (i == AppSections::Settings) return;
            m_settings->setValue(sectionVisibleKey(i), checked);
            emit sectionVisibilityChanged(i, checked);
        });
        m_visibilityChecks[i] = box;
        grid->addWidget(box, i / 2, i % 2);
    }

    layout->addLayout(grid);
    root->addWidget(card);
}

void SettingsSection::addRolePicker(QVBoxLayout* root, const RoleMeta& role,
                                    const QStringList& palette) {
    auto* wrap = new QWidget(this);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto* hdr = new QLabel(role.label, wrap);
    hdr->setStyleSheet(QString(
        "color:%1;font-size:12px;font-weight:600;background:transparent;")
        .arg(Style::textPri()));
    layout->addWidget(hdr);

    auto* desc = new QLabel(role.description, wrap);
    desc->setStyleSheet(QString(
        "color:%1;font-size:10px;background:transparent;")
        .arg(Style::textSec()));
    layout->addWidget(desc);

    auto* row = new QHBoxLayout();
    row->setSpacing(8);
    auto* group = new QButtonGroup(wrap);
    group->setExclusive(true);

    const QString current = Style::themeColor(Style::theme(), role.key);
    for (const QString& color : palette) {
        auto* btn = new QPushButton(wrap);
        btn->setCheckable(true);
        btn->setFixedSize(26, 26);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("roleKey", role.key);
        btn->setProperty("swatchColor", color);
        btn->setChecked(color.compare(current, Qt::CaseInsensitive) == 0);
        group->addButton(btn);
        m_roleButtons[role.key].append(btn);
        row->addWidget(btn);

        connect(btn, &QPushButton::clicked, this, [this, role, color] {
            auto nextTheme = Style::theme();
            Style::setThemeColor(nextTheme, role.key, color);
            Style::saveTheme(*m_settings, nextTheme);
            updateSwatchStyles();
            emit themeChanged();
        });
    }
    row->addStretch();
    layout->addLayout(row);
    root->addWidget(wrap);
    updateSwatchStyles();
}

void SettingsSection::syncVisibilityChecks() {
    for (auto it = m_visibilityChecks.begin(); it != m_visibilityChecks.end(); ++it) {
        const int id = it.key();
        if (id == AppSections::Settings) {
            it.value()->setChecked(true);
            continue;
        }
        it.value()->setChecked(sectionVisible(*m_settings, id));
    }
}

void SettingsSection::updateSwatchStyles() {
    const auto currentTheme = Style::theme();
    for (auto it = m_roleButtons.begin(); it != m_roleButtons.end(); ++it) {
        const QString current = Style::themeColor(currentTheme, it.key());
        for (auto* button : it.value()) {
            const QString color = button->property("swatchColor").toString();
            const bool selected = color.compare(current, Qt::CaseInsensitive) == 0;
            button->setChecked(selected);
            button->setStyleSheet(QString(
                "QPushButton{background:%1;border:%2 solid %3;border-radius:13px;padding:0;}"
                "QPushButton:hover{border-color:%4;}"
                "QPushButton:checked{border-color:%4;}")
                .arg(color,
                     selected ? "2px" : "1px",
                     selected ? Style::textPri() : Style::border(),
                     Style::accent()));
        }
    }
    syncVisibilityChecks();
}
