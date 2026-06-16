#pragma once

namespace app::ui::system {

// ── Icon sizes (px) ───────────────────────────────────────────────────────────
namespace icon {
  constexpr int kXS     = 12;
  constexpr int kSmall  = 14;
  constexpr int kMedium = 16;
  constexpr int kLarge  = 20;
  constexpr int kXL     = 24;
} // namespace icon

// ── Button sizes (px) ─────────────────────────────────────────────────────────
namespace button {
  constexpr int kHeightS      = 20;
  constexpr int kHeightM      = 24;
  constexpr int kHeightL      = 28;
  constexpr int kMinWidthS    = 40;
  constexpr int kMinWidthM    = 60;
  constexpr int kMinWidthL    = 80;
  constexpr int kIconButton   = 24;  // square icon-only button
  constexpr int kRadius       = 3;
} // namespace button

// ── Spacing (px) ──────────────────────────────────────────────────────────────
namespace spacing {
  constexpr int k1      = 2;
  constexpr int kNarrow = 3;   // tight widget gap (between opacity row items etc.)
  constexpr int k2      = 4;
  constexpr int k3      = 6;
  constexpr int k4      = 8;
  constexpr int k5      = 10;
  constexpr int k6      = 12;
  constexpr int k8      = 16;
  constexpr int k10     = 20;
} // namespace spacing

// ── Margin presets (px) ───────────────────────────────────────────────────────
namespace margin {
  constexpr int kNone   = 0;
  constexpr int kTight  = 2;
  constexpr int kNormal = 4;
  constexpr int kLoose  = 8;
  constexpr int kPanel  = 6;   // standard panel content margin
} // namespace margin

// ── Row / item heights (px) ───────────────────────────────────────────────────
namespace row {
  constexpr int kCompact  = 20;
  constexpr int kNormal   = 24;
  constexpr int kComfy    = 28;
  constexpr int kHeader   = 22;  // dock / section header
  constexpr int kSeparator = 1;
} // namespace row

// ── Misc layout constants ─────────────────────────────────────────────────────
namespace layout {
  constexpr int kSliderTrack  = 3;
  constexpr int kSliderHandle = 12;
  constexpr int kScrollBar    = 6;
  constexpr int kDivider      = 1;
} // namespace layout

// ── Layer panel semantic metrics ──────────────────────────────────────────────
namespace layer {
  // Row / thumbnail  (4:3 ratio: 36×27)
  constexpr int kRowHeight       = 34;   // layer item row height
  constexpr int kThumbW          = 36;   // thumbnail width  (4:3)
  constexpr int kThumbH          = 27;   // thumbnail height (4:3)
  constexpr int kMaskThumbW      = 22;
  constexpr int kMaskThumbH      = 27;   // matches kThumbH for horizontal alignment
  constexpr int kThumbGap        = 2;    // gap between image and mask thumb

  // Left-zone slot widths (eye → expand → typeIcon → thumb)
  constexpr int kVisibilitySlotW = 20;   // eye icon slot
  constexpr int kExpandSlotW     = 12;   // folder expand/collapse chevron slot (always allocated)
  constexpr int kTypeIconSlotW   = 12;   // layer type icon slot
  constexpr int kActiveSlotW     = 10;   // legacy – superseded by full-row highlight

  // Right-zone
  constexpr int kStatusAreaW     = 48;   // 3 × kSmall(14) + 6 padding
  constexpr int kIndentW         = 16;   // per hierarchy depth level

  // Control sizes
  constexpr int kOpacitySliderH  = 22;
  constexpr int kOpacitySpinW    = 48;
  constexpr int kMinListH        = 140;  // minimum list height (normal mode)
  constexpr int kMinListHCompact =  90;  // minimum list height (compact height)
} // namespace layer

} // namespace app::ui::system
