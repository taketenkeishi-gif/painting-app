#pragma once

#include <QString>

namespace app::ui {

// ── Color tokens ─────────────────────────────────────────────────────────────
namespace color {
  // Backgrounds
  constexpr auto kBgDeep       = "#0d0f14";   // canvas surround / deep bg
  constexpr auto kBgPanel      = "#181b22";   // panel background
  constexpr auto kBgRaised     = "#1f232e";   // raised elements
  constexpr auto kBgHover      = "#272c3c";   // hover state
  constexpr auto kBgSelected   = "#1d3a7a";   // selected item
  constexpr auto kBgActive     = "#1e5b94";   // active / focused

  // Borders
  constexpr auto kBorderDeep   = "#0a0b0e";
  constexpr auto kBorderSubtle = "#1e2230";
  constexpr auto kBorderPanel  = "#2a2e3e";
  constexpr auto kBorderFocus  = "#4e8ef7";

  // Text
  constexpr auto kTextPrimary  = "#d4d4d4";
  constexpr auto kTextSecond   = "#8890a0";
  constexpr auto kTextDisabled = "#4a5060";
  constexpr auto kTextAccent   = "#7ab0f0";

  // Accent
  constexpr auto kAccentBlue   = "#4e8ef7";
  constexpr auto kAccentGreen  = "#4ec97a";
  constexpr auto kAccentRed    = "#e05555";
  constexpr auto kAccentOrange = "#e09040";
} // namespace color

// ── Size tokens ───────────────────────────────────────────────────────────────
namespace size {
  constexpr int kRadius        = 3;    // border-radius (px)
  constexpr int kPaddingXS     = 2;
  constexpr int kPaddingS      = 4;
  constexpr int kPaddingM      = 6;
  constexpr int kPaddingL      = 8;
  constexpr int kIconSmall     = 14;
  constexpr int kIconMedium    = 16;
  constexpr int kRowHeight     = 24;
  constexpr int kHeaderHeight  = 22;
  constexpr int kSliderTrack   = 4;
  constexpr int kSliderHandle  = 14;
} // namespace size

// ── Global stylesheet fragments ──────────────────────────────────────────────
inline QString globalPanelQss() {
  return QStringLiteral(
    // ── QWidget base ──────────────────────────────────────────────────────
    "QWidget { color: #d4d4d4; }"

    // ── GroupBox ─────────────────────────────────────────────────────────
    "QGroupBox {"
    "  border: 1px solid #2a2e3e;"
    "  border-radius: 3px;"
    "  margin-top: 14px;"
    "  padding: 4px 4px 4px 4px;"
    "  background: #181b22;"
    "}"
    "QGroupBox::title {"
    "  subcontrol-origin: margin;"
    "  subcontrol-position: top left;"
    "  left: 6px;"
    "  top: 2px;"
    "  color: #8890a0;"
    "  font-size: 10px;"
    "}"

    // ── QScrollBar ────────────────────────────────────────────────────────
    "QScrollBar:vertical {"
    "  background: transparent; width: 6px; margin: 0;"
    "}"
    "QScrollBar::handle:vertical {"
    "  background: #2e3448; border-radius: 3px; min-height: 20px;"
    "}"
    "QScrollBar::handle:vertical:hover { background: #4a5068; }"
    "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    "QScrollBar:horizontal {"
    "  background: transparent; height: 6px; margin: 0;"
    "}"
    "QScrollBar::handle:horizontal {"
    "  background: #2e3448; border-radius: 3px; min-width: 20px;"
    "}"
    "QScrollBar::handle:horizontal:hover { background: #4a5068; }"
    "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"

    // ── QSlider (custom Photoshop-style) ──────────────────────────────────
    "QSlider::groove:horizontal {"
    "  height: 4px;"
    "  background: #1e2230;"
    "  border-radius: 2px;"
    "  margin: 0 7px;"
    "}"
    "QSlider::handle:horizontal {"
    "  background: #d4d8e4;"
    "  border: 1px solid #3a4252;"
    "  width: 14px; height: 14px;"
    "  border-radius: 7px;"
    "  margin: -5px -7px;"
    "}"
    "QSlider::handle:horizontal:hover { background: #ffffff; }"
    "QSlider::handle:horizontal:pressed { background: #4e8ef7; }"
    "QSlider::sub-page:horizontal {"
    "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
    "    stop:0 #2a3e6a, stop:1 #4e8ef7);"
    "  border-radius: 2px; margin: 0 7px;"
    "}"
    "QSlider::groove:horizontal:disabled { background: #16181e; }"
    "QSlider::handle:horizontal:disabled { background: #2e3244; border-color: #22263a; }"

    // ── QComboBox ─────────────────────────────────────────────────────────
    "QComboBox {"
    "  background: #1f232e; border: 1px solid #2a2e3e;"
    "  border-radius: 3px; padding: 2px 6px;"
    "  min-height: 22px; color: #c8ccd6;"
    "}"
    "QComboBox:hover { border-color: #3a4252; }"
    "QComboBox:focus { border-color: #4e8ef7; }"
    "QComboBox::drop-down { border: none; width: 18px; }"
    "QComboBox::down-arrow { width: 8px; height: 8px; }"
    "QComboBox QAbstractItemView {"
    "  background: #1a1d26; border: 1px solid #2a2e3e;"
    "  selection-background-color: #1e5b94; color: #d4d4d4;"
    "}"

    // ── QSpinBox / QDoubleSpinBox ─────────────────────────────────────────
    "QSpinBox, QDoubleSpinBox {"
    "  background: #1f232e; border: 1px solid #2a2e3e;"
    "  border-radius: 3px; padding: 1px 4px; color: #c8ccd6;"
    "  min-height: 20px;"
    "}"
    "QSpinBox:focus, QDoubleSpinBox:focus { border-color: #4e8ef7; }"
    "QSpinBox::up-button, QSpinBox::down-button,"
    "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width: 0; }"

    // ── QPushButton ───────────────────────────────────────────────────────
    "QPushButton {"
    "  background: #232736; border: 1px solid #2e3448;"
    "  border-radius: 3px; padding: 3px 8px; color: #c8ccd6; min-height: 20px;"
    "}"
    "QPushButton:hover { background: #2c3245; border-color: #3e4860; color: #e8eaf2; }"
    "QPushButton:pressed { background: #1a1f2e; border-color: #4e8ef7; }"
    "QPushButton:disabled { background: #181b22; color: #4a5060; border-color: #1e2230; }"
    "QPushButton:checked { background: #1d3a7a; border-color: #4e8ef7; color: #a8c8f8; }"

    // ── QLineEdit ─────────────────────────────────────────────────────────
    "QLineEdit {"
    "  background: #1f232e; border: 1px solid #2a2e3e;"
    "  border-radius: 3px; padding: 2px 6px; color: #c8ccd6; min-height: 20px;"
    "}"
    "QLineEdit:focus { border-color: #4e8ef7; }"

    // ── QDockWidget ───────────────────────────────────────────────────────
    "QDockWidget { background: #181b22; }"
    "QDockWidget::title { background: transparent; height: 0px; padding: 0px; margin: 0px; border: none; }"

    // ── QTabBar ───────────────────────────────────────────────────────────
    "QTabBar::tab {"
    "  background: #1a1d26; border: none;"
    "  border-bottom: 2px solid transparent;"
    "  padding: 3px 10px; min-width: 64px;"
    "  color: #8890a0; font-size: 10px;"
    "}"
    "QTabBar::tab:selected {"
    "  background: #1f232e; color: #d4d4d4;"
    "  border-bottom: 2px solid #4e8ef7;"
    "}"
    "QTabBar::tab:hover { background: #222636; color: #b8bcc8; }"

    // ── QMenu ─────────────────────────────────────────────────────────────
    "QMenu {"
    "  background: #1a1d26; border: 1px solid #2a2e3e; padding: 3px 0;"
    "}"
    "QMenu::item { padding: 4px 20px 4px 28px; color: #c8ccd6; }"
    "QMenu::item:selected { background: #1e5b94; color: #e8eaf2; }"
    "QMenu::separator { height: 1px; background: #2a2e3e; margin: 3px 0; }"

    // ── QToolTip ──────────────────────────────────────────────────────────
    "QToolTip {"
    "  background: #1a1d26; border: 1px solid #3a4252;"
    "  color: #d4d4d4; padding: 4px 8px; border-radius: 3px;"
    "}"

    // ── Dock-area QTabBar (tabified docks, not QTabWidget) ─────────────────
    "QTabBar[dockTabBar=\"true\"] {"
    "  background: #181b22;"
    "  border: none;"
    "}"
    "QTabBar[dockTabBar=\"true\"]::tab {"
    "  background: #181b22;"
    "  color: #8890a0;"
    "  border: none;"
    "  border-right: 1px solid #0d0f14;"
    "  padding: 3px 10px;"
    "  min-width: 56px;"
    "  font-size: 10px;"
    "}"
    "QTabBar[dockTabBar=\"true\"]::tab:selected {"
    "  background: #1f232e;"
    "  color: #d4d4d4;"
    "  border-bottom: 2px solid #4e8ef7;"
    "}"
    "QTabBar[dockTabBar=\"true\"]::tab:hover:!selected {"
    "  background: #222636;"
    "  color: #b8bcc8;"
    "}"
    "QTabBar[dockTabBar=\"true\"]::close-button { image: none; width: 0; }"
  );
}

} // namespace app::ui
