#pragma once

#include "core/color/Color.h"

namespace core {

enum class BrushShapeType {
  Circle,
  Square,
  Ellipse
};

// Photoshop互換ブレンドモード（値は後方互換のため固定）
enum class BlendMode {
  // 基本
  Normal      = 0,
  Dissolve    = 3,

  // 暗くする
  Darken      = 4,
  Multiply    = 1,
  ColorBurn   = 5,
  LinearBurn  = 6,
  DarkerColor = 7,

  // 明るくする
  Lighten      = 8,
  Screen       = 9,
  ColorDodge   = 10,
  LinearDodge  = 2,  // 旧 Add と同値で後方互換
  LighterColor = 11,

  // コントラスト
  Overlay     = 12,
  SoftLight   = 13,
  HardLight   = 14,
  VividLight  = 15,
  LinearLight = 16,
  PinLight    = 17,
  HardMix     = 18,

  // 比較
  Difference = 19,
  Exclusion  = 20,
  Subtract   = 21,
  Divide     = 22,

  // カラー成分（HSL）
  Hue        = 23,
  HslSat     = 24,  // Saturation（core::BlendMode::Saturation enum値保護のため）
  HslColor   = 25,  // Color（core::Color との名前衝突回避）
  Luminosity = 26,
};

enum class VectorEraseMode {
  TouchedOnly,
  ToIntersection,
  TrimOutside
};

struct BrushDynamics {
  // 筆圧マッピング (0=なし, 1=リニア, 2=カーブ)
  bool pressureSize     {true};
  bool pressureOpacity  {false};
  bool pressureHardness {false};
  bool pressureFlow     {false};
  float pressureSizeMin {0.1f};     // 筆圧0のときのサイズ比率
  float pressureOpacityMin {0.1f};  // 筆圧0のときのopacity比率
};

struct BrushSettings {
  Color color {0, 0, 0, 255};
  int size {8};
  float opacity {1.0F};
  float hardness {1.0F};
  float flow {1.0F};
  float spacing {0.1F};   // Kritaデフォルト相当（0.1 = 10%）
  bool antiAlias {true};
  float stabilization {0.0F};
  bool postCorrection {false};
  bool velocityBasedCorrection {false};
  BrushShapeType shapeType {BrushShapeType::Circle};
  float angle {0.0F};       // 度数
  float roundness {1.0F};   // 楕円率 0.0-1.0
  float taperStart {0.0F};
  float taperEnd {0.0F};
  BlendMode blendMode {BlendMode::Normal};
  bool eraseMode {false};
  bool lockAlphaRespect {false};
  bool buildupMode {false};  // trueにすると同ストロークでも積み重なる

  BrushDynamics dynamics;
};

} // namespace core
