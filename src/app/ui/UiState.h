#pragma once

#include <string>

#include "app/ui/ToolDescriptor.h"
#include "core/tools/ToolTypes.h"
#include "core/tools/ToolType.h"

namespace app::ui {

// UI-level active tool snapshot used by controller and panels.
struct UiState {
  core::ToolKind toolKind {core::ToolKind::Brush};
  std::string subToolId;
  int size {8};
  int opacity {100};
  int hardness {100};
  int flow {100};
  int spacing {25};
  int angle {0};
  int roundness {100};
  int taperStart {0};
  int taperEnd {0};
  bool antiAlias {true};
  int stabilization {0};
  int snapAngle {0};
  int simplifyLevel {0};
  bool postCorrection {false};
  bool velocityBasedCorrection {false};
  core::BrushShapeType shapeType {core::BrushShapeType::Circle};
  core::BlendMode blendMode {core::BlendMode::Normal};
  bool eraseMode {false};
  bool lockAlphaRespect {false};
  VectorEraserMode vectorEraseMode {VectorEraserMode::TouchedOnly};
  bool vectorTrimOutside {false};
  int fillThreshold {0};
  bool fillContiguous {true};
  bool fillReferAllLayers {false};
  int fillGapClose {0};
  SelectionMode selectionMode {SelectionMode::Rectangle};
  int autoSelectThreshold {16};
  bool autoSelectContiguous {true};
  bool autoSelectReferAllLayers {true};
  int selectionFeather {0};
  bool selectionAntiAlias {true};
  bool buildupMode {false};   // trueで積み上げ、falseでKritaスタイル非積み上げ
  bool pressureSize {false};
  float pressureSizeMin {0.0f};
  bool pressureOpacity {false};
  float pressureOpacityMin {0.0f};

  // ── 速度感応 ──────────────────────────────────────────────────────────────
  bool  velocitySize       {false};
  float velocitySizeMin    {0.3f};
  bool  velocityOpacity    {false};
  float velocityOpacityMin {0.3f};

  // ── テクスチャグレイン ──────────────────────────────────────────────────
  bool  textureGrain    {false};
  float textureStrength {0.6f};
  float textureScale    {1.0f};

  // ── ウェットミックス / スメア ────────────────────────────────────────────
  bool  wetMix    {false};
  float wetMixRate{0.5f};
  bool  smear     {false};
  float smearRate {0.9f};

  // ── グラデーション ────────────────────────────────────────────────────────
  int gradientType {0};  ///< 0=Linear, 1=Radial
  int gradientFill {0};  ///< 0=FgToBg, 1=FgToTransparent

  // ── レイヤー編集対象 ─────────────────────────────────────────────────────
  enum class EditTarget { Image, Mask };
  EditTarget editTarget {EditTarget::Image};
};

} // namespace app::ui
