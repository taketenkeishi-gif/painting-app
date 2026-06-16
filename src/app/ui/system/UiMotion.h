#pragma once

namespace app::ui::system {

// ── Animation durations (ms) ──────────────────────────────────────────────────
namespace duration {
  constexpr int kInstant   =   0;
  constexpr int kFast      =  80;
  constexpr int kNormal    = 150;
  constexpr int kSlow      = 250;
  constexpr int kVerySlow  = 400;
  constexpr int kTooltip   = 500;  // tooltip show delay
} // namespace duration

// ── Easing curve names (match QEasingCurve enum labels) ──────────────────────
// Usage: QPropertyAnimation::setEasingCurve(QEasingCurve::Type(motion::kEnter))
namespace easing {
  // QEasingCurve::OutCubic  = 9
  // QEasingCurve::InOutCubic = 10
  // QEasingCurve::OutQuart  = 14
  // QEasingCurve::Linear    = 1
  constexpr int kEnter  = 9;   // OutCubic  — elements appearing
  constexpr int kExit   = 6;   // InCubic   — elements disappearing
  constexpr int kMove   = 10;  // InOutCubic — repositioning
  constexpr int kLinear = 1;   // Linear     — opacity / color
} // namespace easing

} // namespace app::ui::system
