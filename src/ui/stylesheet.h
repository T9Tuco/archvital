#pragma once
#include <QString>

namespace Style {

constexpr const char* BG          = "#0d0d0d";
constexpr const char* SURFACE     = "#141414";
constexpr const char* CARD        = "#1a1a1a";
constexpr const char* BORDER      = "#2a2a2a";
constexpr const char* TEXT_PRI    = "#e0e0e0";
constexpr const char* TEXT_SEC    = "#9e9e9e";
constexpr const char* ACCENT      = "#00e5ff";
constexpr const char* WARNING     = "#ff9800";
constexpr const char* ERROR       = "#ff1744";
constexpr const char* INFO        = "#651fff";
constexpr const char* HOVER       = "#1e1e1e";
constexpr const char* SELECTED    = "#1a2a2d";

inline QString appStyleSheet() {
    return QString(R"(
QMainWindow, QDialog {
    background-color: #0d0d0d;
    color: #e0e0e0;
}

QWidget {
    background-color: transparent;
    color: #e0e0e0;
    font-family: "Inter", "Noto Sans", sans-serif;
    font-size: 13px;
}

QScrollArea {
    background-color: transparent;
    border: none;
}

QScrollBar:vertical {
    background: #141414;
    width: 6px;
    margin: 0;
    border-radius: 3px;
}
QScrollBar::handle:vertical {
    background: #3a3a3a;
    min-height: 30px;
    border-radius: 3px;
}
QScrollBar::handle:vertical:hover { background: #00e5ff; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }

QScrollBar:horizontal {
    background: #141414;
    height: 6px;
    margin: 0;
    border-radius: 3px;
}
QScrollBar::handle:horizontal {
    background: #3a3a3a;
    min-width: 30px;
    border-radius: 3px;
}
QScrollBar::handle:horizontal:hover { background: #00e5ff; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

QLabel {
    background: transparent;
    color: #e0e0e0;
}

QPushButton {
    background-color: #1a1a1a;
    color: #e0e0e0;
    border: 1px solid #2a2a2a;
    border-radius: 8px;
    padding: 6px 14px;
    font-size: 13px;
}
QPushButton:hover {
    background-color: #242424;
    border-color: #00e5ff;
    color: #00e5ff;
}
QPushButton:pressed { background-color: #1a2a2d; }
QPushButton:disabled { color: #555555; border-color: #222222; }

QPushButton#accentBtn {
    background-color: #003d4d;
    color: #00e5ff;
    border: 1px solid #00e5ff;
}
QPushButton#accentBtn:hover { background-color: #004d60; }

QLineEdit {
    background-color: #1a1a1a;
    color: #e0e0e0;
    border: 1px solid #2a2a2a;
    border-radius: 8px;
    padding: 6px 10px;
    selection-background-color: #003d4d;
}
QLineEdit:focus { border-color: #00e5ff; }
QLineEdit::placeholder { color: #555555; }

QTableView, QTreeView {
    background-color: #141414;
    alternate-background-color: #161616;
    color: #e0e0e0;
    border: 1px solid #2a2a2a;
    border-radius: 8px;
    gridline-color: #1e1e1e;
    selection-background-color: #1a2a2d;
    selection-color: #00e5ff;
    outline: none;
}
QTableView::item, QTreeView::item {
    padding: 4px 8px;
    border: none;
}
QTableView::item:selected, QTreeView::item:selected {
    background-color: #1a2a2d;
    color: #00e5ff;
}

QHeaderView::section {
    background-color: #1a1a1a;
    color: #9e9e9e;
    border: none;
    border-bottom: 1px solid #2a2a2a;
    padding: 6px 8px;
    font-size: 12px;
    font-weight: 600;
}
QHeaderView::section:hover {
    background-color: #242424;
    color: #e0e0e0;
}
QHeaderView::section:checked { color: #00e5ff; }

QMenu {
    background-color: #1a1a1a;
    color: #e0e0e0;
    border: 1px solid #2a2a2a;
    border-radius: 8px;
    padding: 4px;
}
QMenu::item {
    padding: 6px 20px;
    border-radius: 4px;
}
QMenu::item:selected { background-color: #242424; color: #00e5ff; }
QMenu::separator { height: 1px; background: #2a2a2a; margin: 4px 8px; }

QSplitter::handle { background: #2a2a2a; }

QToolTip {
    background-color: #1a1a1a;
    color: #e0e0e0;
    border: 1px solid #2a2a2a;
    border-radius: 6px;
    padding: 4px 8px;
}

QCheckBox {
    spacing: 8px;
    color: #e0e0e0;
}
QCheckBox::indicator {
    width: 16px; height: 16px;
    background: #1a1a1a;
    border: 1px solid #2a2a2a;
    border-radius: 4px;
}
QCheckBox::indicator:checked {
    background: #00e5ff;
    border-color: #00e5ff;
}

QComboBox {
    background-color: #1a1a1a;
    color: #e0e0e0;
    border: 1px solid #2a2a2a;
    border-radius: 8px;
    padding: 5px 10px;
    min-width: 80px;
}
QComboBox:focus { border-color: #00e5ff; }
QComboBox::drop-down { border: none; width: 20px; }
QComboBox QAbstractItemView {
    background: #1a1a1a;
    border: 1px solid #2a2a2a;
    selection-background-color: #1a2a2d;
    selection-color: #00e5ff;
    outline: none;
}

QProgressBar {
    background-color: #222222;
    border: none;
    border-radius: 4px;
    text-align: center;
    color: transparent;
    height: 6px;
}
QProgressBar::chunk {
    background-color: #00e5ff;
    border-radius: 4px;
}
)");
}

} // namespace Style
