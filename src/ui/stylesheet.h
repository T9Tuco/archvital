#pragma once
#include <QString>
#include <QColor>
#include <QSettings>

namespace Style {

struct Theme {
    QString bg{"#0d0d0d"};
    QString surface{"#111111"};
    QString card{"#161616"};
    QString border{"#222222"};
    QString textPri{"#e2e2e2"};
    QString textSec{"#888888"};
    QString accent{"#00e5ff"};
    QString warning{"#ff9800"};
    QString error{"#ff1744"};
    QString info{"#7c4dff"};
    QString hover{"#1c1c1c"};
    QString selected{"#0a2a30"};
};

inline Theme defaultTheme() { return {}; }
inline Theme& currentThemeStorage() {
    static Theme theme = defaultTheme();
    return theme;
}
inline const Theme& theme() { return currentThemeStorage(); }
inline void setTheme(const Theme& theme) { currentThemeStorage() = theme; }

inline const QString& bg()       { return theme().bg; }
inline const QString& surface()  { return theme().surface; }
inline const QString& card()     { return theme().card; }
inline const QString& border()   { return theme().border; }
inline const QString& textPri()  { return theme().textPri; }
inline const QString& textSec()  { return theme().textSec; }
inline const QString& accent()   { return theme().accent; }
inline const QString& warning()  { return theme().warning; }
inline const QString& error()    { return theme().error; }
inline const QString& info()     { return theme().info; }
inline const QString& hover()    { return theme().hover; }
inline const QString& selected() { return theme().selected; }

inline QColor bgColor()       { return QColor(bg()); }
inline QColor surfaceColor()  { return QColor(surface()); }
inline QColor cardColor()     { return QColor(card()); }
inline QColor borderColor()   { return QColor(border()); }
inline QColor textPriColor()  { return QColor(textPri()); }
inline QColor textSecColor()  { return QColor(textSec()); }
inline QColor accentColor()   { return QColor(accent()); }
inline QColor warningColor()  { return QColor(warning()); }
inline QColor errorColor()    { return QColor(error()); }
inline QColor infoColor()     { return QColor(info()); }
inline QColor hoverColor()    { return QColor(hover()); }
inline QColor selectedColor() { return QColor(selected()); }

inline QString themeColor(const Theme& theme, const QString& role) {
    if (role == "bg") return theme.bg;
    if (role == "surface") return theme.surface;
    if (role == "card") return theme.card;
    if (role == "border") return theme.border;
    if (role == "textPri") return theme.textPri;
    if (role == "textSec") return theme.textSec;
    if (role == "accent") return theme.accent;
    if (role == "warning") return theme.warning;
    if (role == "error") return theme.error;
    if (role == "info") return theme.info;
    if (role == "hover") return theme.hover;
    if (role == "selected") return theme.selected;
    return {};
}

inline void setThemeColor(Theme& theme, const QString& role, const QString& color) {
    if (role == "bg") theme.bg = color;
    else if (role == "surface") theme.surface = color;
    else if (role == "card") theme.card = color;
    else if (role == "border") theme.border = color;
    else if (role == "textPri") theme.textPri = color;
    else if (role == "textSec") theme.textSec = color;
    else if (role == "accent") theme.accent = color;
    else if (role == "warning") theme.warning = color;
    else if (role == "error") theme.error = color;
    else if (role == "info") theme.info = color;
    else if (role == "hover") theme.hover = color;
    else if (role == "selected") theme.selected = color;
}

inline Theme loadTheme(QSettings& settings) {
    Theme loaded = defaultTheme();
    settings.beginGroup("theme");
    loaded.bg = settings.value("bg", loaded.bg).toString();
    loaded.surface = settings.value("surface", loaded.surface).toString();
    loaded.card = settings.value("card", loaded.card).toString();
    loaded.border = settings.value("border", loaded.border).toString();
    loaded.textPri = settings.value("textPri", loaded.textPri).toString();
    loaded.textSec = settings.value("textSec", loaded.textSec).toString();
    loaded.accent = settings.value("accent", loaded.accent).toString();
    loaded.warning = settings.value("warning", loaded.warning).toString();
    loaded.error = settings.value("error", loaded.error).toString();
    loaded.info = settings.value("info", loaded.info).toString();
    loaded.hover = settings.value("hover", loaded.hover).toString();
    loaded.selected = settings.value("selected", loaded.selected).toString();
    settings.endGroup();
    setTheme(loaded);
    return loaded;
}

inline void saveTheme(QSettings& settings, const Theme& theme) {
    settings.beginGroup("theme");
    settings.setValue("bg", theme.bg);
    settings.setValue("surface", theme.surface);
    settings.setValue("card", theme.card);
    settings.setValue("border", theme.border);
    settings.setValue("textPri", theme.textPri);
    settings.setValue("textSec", theme.textSec);
    settings.setValue("accent", theme.accent);
    settings.setValue("warning", theme.warning);
    settings.setValue("error", theme.error);
    settings.setValue("info", theme.info);
    settings.setValue("hover", theme.hover);
    settings.setValue("selected", theme.selected);
    settings.endGroup();
    setTheme(theme);
}

inline QString appStyleSheet() {
    const auto& t = theme();
    return QStringLiteral(R"(

/* ── Base ──────────────────────────────────────────────────────────── */
QMainWindow, QDialog {
    background: %1;
    color: %2;
}
QWidget {
    background: transparent;
    color: %2;
    font-family: "Inter", "Segoe UI", "Noto Sans", sans-serif;
    font-size: 13px;
}

/* ── Scroll bars ────────────────────────────────────────────────────── */
QScrollArea            { border: none; background: transparent; }
QScrollBar:vertical    { background: transparent; width: 5px; margin: 0; }
QScrollBar::handle:vertical {
    background: %3; min-height: 24px; border-radius: 2px;
}
QScrollBar::handle:vertical:hover { background: %4; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical,  QScrollBar::sub-page:vertical { background: none; }
QScrollBar:horizontal  { background: transparent; height: 5px; margin: 0; }
QScrollBar::handle:horizontal {
    background: %3; min-width: 24px; border-radius: 2px;
}
QScrollBar::handle:horizontal:hover { background: %4; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

/* ── Buttons ────────────────────────────────────────────────────────── */
QPushButton {
    background: %5;
    color: %6;
    border: 1px solid %7;
    border-radius: 7px;
    padding: 5px 14px;
    font-size: 13px;
}
QPushButton:hover  { background: %8; border-color: %9; color: %2; }
QPushButton:pressed{ background: %10; }
QPushButton:disabled{ color: #444; border-color: %11; }

QPushButton#accentBtn {
    background: %12;
    color: %4;
    border-color: %13;
}
QPushButton#accentBtn:hover { background: %14; border-color: %4; }
QPushButton:checked {
    background: %12;
    color: %4;
    border-color: %13;
}

/* ── Inputs ─────────────────────────────────────────────────────────── */
QLineEdit {
    background: %10;
    color: %2;
    border: 1px solid %7;
    border-radius: 7px;
    padding: 6px 10px;
    selection-background-color: %13;
}
QLineEdit:focus { border-color: %4; }
QLineEdit::placeholder{ color: #3a3a3a; }

/* ── Tables / Trees ─────────────────────────────────────────────────── */
QTableView, QTreeView {
    background: %15;
    alternate-background-color: %16;
    color: #d0d0d0;
    border: 1px solid %17;
    border-radius: 8px;
    gridline-color: %5;
    selection-background-color: %12;
    selection-color: %4;
    outline: none;
    font-size: 12px;
}
QTableView::item, QTreeView::item { padding: 3px 8px; border: none; }
QTableView::item:selected, QTreeView::item:selected {
    background: %12; color: %4;
}
QHeaderView::section {
    background: %10;
    color: #666;
    border: none;
    border-bottom: 1px solid %11;
    border-right: 1px solid %11;
    padding: 5px 8px;
    font-size: 11px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.5px;
}
QHeaderView::section:hover        { background: %5; color: #aaa; }
QHeaderView::section:last-child   { border-right: none; }

/* ── Menus ──────────────────────────────────────────────────────────── */
QMenu {
    background: %18;
    color: #d0d0d0;
    border: 1px solid %7;
    border-radius: 8px;
    padding: 4px 0;
}
QMenu::item         { padding: 6px 18px 6px 12px; border-radius: 4px; margin: 1px 4px; }
QMenu::item:selected{ background: %11; color: %4; }
QMenu::separator    { height: 1px; background: %17; margin: 3px 8px; }

/* ── Progress bars ──────────────────────────────────────────────────── */
QProgressBar {
    background: %5;
    border: none;
    border-radius: 3px;
    color: transparent;
}
QProgressBar::chunk { background: %4; border-radius: 3px; }

/* ── Misc ───────────────────────────────────────────────────────────── */
QCheckBox             { spacing: 7px; color: #aaa; }
QCheckBox::indicator  { width:15px; height:15px; background:%5; border:1px solid #333; border-radius:4px; }
QCheckBox::indicator:checked { background:%4; border-color:%4; }

QToolTip { background:%5; color:#d0d0d0; border:1px solid %7; border-radius:6px; padding:4px 8px; }

QSplitter::handle { background: %11; }

)")
        .arg(t.bg)
        .arg(t.textPri)
        .arg(t.border)
        .arg(t.accent)
        .arg(t.hover)
        .arg(t.textSec)
        .arg(t.border)
        .arg(t.hover)
        .arg(t.border)
        .arg(t.card)
        .arg(t.border)
        .arg(t.selected)
        .arg(t.selected)
        .arg(t.selected)
        .arg(t.surface)
        .arg(t.card)
        .arg(t.border)
        .arg(t.card);
}

} // namespace Style
