#pragma once

namespace app::ui::system {

// ── Panel width breakpoints (px) ──────────────────────────────────────────────
// Used to switch between compact / normal / expanded panel layouts.
namespace breakpoint {
  constexpr int kPanelCompact  = 160;
  constexpr int kPanelNormal   = 220;
  constexpr int kPanelExpanded = 300;

  constexpr int kWindowS  =  900;
  constexpr int kWindowM  = 1280;
  constexpr int kWindowL  = 1600;
  constexpr int kWindowXL = 1920;
} // namespace breakpoint

// ── Minimum panel / dock sizes (px) ──────────────────────────────────────────
namespace minSize {
  constexpr int kToolPanel     = 40;
  constexpr int kLayerPanel    = 160;
  constexpr int kColorPanel    = 180;
  constexpr int kSubToolPanel  = 180;
  constexpr int kCanvasMinW    = 400;
  constexpr int kCanvasMinH    = 300;
} // namespace minSize

// ── Layer panel responsive breakpoints ───────────────────────────────────────
namespace layerPanel {
  constexpr int kCompactWidth  = 420;   // below → icon-only buttons
  constexpr int kCompactHeight = 560;   // below → shorter list min-height
} // namespace layerPanel

// ── Density modes ─────────────────────────────────────────────────────────────
// Panel can query current density and pick row height from UiMetrics::row.
enum class Density { Compact, Normal, Comfy };

inline constexpr int rowHeightFor(Density d) noexcept {
  switch (d) {
    case Density::Compact: return 20;
    case Density::Comfy:   return 28;
    default:               return 24;
  }
}

} // namespace app::ui::system
