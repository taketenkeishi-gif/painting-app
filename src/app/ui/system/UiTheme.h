#pragma once

// Design-system colour tokens.
// Canonical values are defined here; Theme.h global QSS references these names.

namespace app::ui::system {

namespace theme {

  // ── Background ─────────────────────────────────────────────────────────────
  namespace bg {
    constexpr auto kDeep     = "#0d0f14";
    constexpr auto kPanel    = "#181b22";
    constexpr auto kRaised   = "#1f232e";
    constexpr auto kHover    = "#272c3c";
    constexpr auto kSelected = "#1d3a7a";
    constexpr auto kActive   = "#1e5b94";
    constexpr auto kInput    = "#1f232e";
  } // namespace bg

  // ── Border ─────────────────────────────────────────────────────────────────
  namespace border {
    constexpr auto kDeep   = "#0a0b0e";
    constexpr auto kSubtle = "#1e2230";
    constexpr auto kPanel  = "#2a2e3e";
    constexpr auto kFocus  = "#4e8ef7";
    constexpr auto kHover  = "#3a4252";
  } // namespace border

  // ── Text ───────────────────────────────────────────────────────────────────
  namespace text {
    constexpr auto kPrimary  = "#d4d4d4";
    constexpr auto kSecond   = "#8890a0";
    constexpr auto kDisabled = "#4a5060";
    constexpr auto kAccent   = "#7ab0f0";
    constexpr auto kLabel    = "#c8ccd6";
  } // namespace text

  // ── Accent ─────────────────────────────────────────────────────────────────
  namespace accent {
    constexpr auto kBlue   = "#4e8ef7";
    constexpr auto kGreen  = "#4ec97a";
    constexpr auto kRed    = "#e05555";
    constexpr auto kOrange = "#e09040";
  } // namespace accent

  // ── Surface variants (scrollbar, track, etc.) ──────────────────────────────
  namespace surface {
    constexpr auto kScrollHandle      = "#2e3448";
    constexpr auto kScrollHandleHover = "#4a5068";
    constexpr auto kGroove            = "#16181e";
    constexpr auto kSubPage           = "#4e8ef7";
  } // namespace surface

} // namespace theme

// ── Layer panel semantic colours ──────────────────────────────────────────────
namespace layer {
  // Row backgrounds
  constexpr auto kActiveBg        = "#1e5b94";   // active layer row
  constexpr auto kSelectedBg      = "#153860";   // multi-selected, non-active
  constexpr auto kAlternateBg     = "#222222";   // alternating row (even)
  constexpr auto kRowDivider      = "#1e1e1e";   // 1px bottom border between rows

  // Text
  constexpr auto kNameVisible     = "#d4d4d4";   // layer name when visible
  constexpr auto kNameHidden      = "#787878";   // layer name when hidden

  // Thumbnail / badge decorations
  constexpr auto kChevron         = "#a0afc8";   // folder expand/collapse chevron
  constexpr auto kClipBar         = "#ff8844";   // clipping indicator bar (alpha=210 at use)
  constexpr auto kRasterBadge     = "#4a96e8";   // badge on raster thumb (alpha=200 at use)
  constexpr auto kVectorBadge     = "#70e880";   // badge stroke on vector thumb (alpha=210)
  constexpr auto kThumbBorder     = "#1e1e1e";   // normal thumbnail border
  constexpr auto kFocusBorder     = "#4e8ef7";   // edit-target highlight border
  constexpr auto kMaskDisabled    = "#e05050";   // disabled mask border / X stroke
  constexpr auto kDimOverlay      = "#000000";   // parent-hidden dim overlay (alpha=70)

  // Inline name editor
  constexpr auto kEditorBg        = "#1a2030";
  constexpr auto kEditorText      = "#edf0f9";
  constexpr auto kEditorBorder    = "#4e8ef7";

  // Icon-button strip (lock / clip / mask buttons)
  constexpr auto kIconBtnBg            = "#202a36";
  constexpr auto kIconBtnBorder        = "#3a4658";
  constexpr auto kIconBtnHoverBg       = "#263446";
  constexpr auto kIconBtnHoverBorder   = "#55708f";
  constexpr auto kIconBtnPressedBg     = "#2e4f79";
  constexpr auto kIconBtnPressedBorder = "#7fb3ff";
  constexpr auto kIconBtnCheckedBg     = "#1e3a60";
  constexpr auto kIconBtnCheckedBorder = "#4e8ef7";
  constexpr auto kIconBtnDisabledText  = "#5f6b7a";
  constexpr auto kIconBtnDisabledBg    = "#18212c";
  constexpr auto kIconBtnDisabledBorder= "#2c3542";

  // List widget item QSS colours
  constexpr auto kListItemSelected = "#2e4f79";
  constexpr auto kListItemBorder   = "#252d38";
  constexpr auto kListDropBg       = "#243142";
  constexpr auto kListDropBorder   = "#7fb3ff";
} // namespace layer

} // namespace app::ui::system
