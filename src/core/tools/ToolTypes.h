#pragma once

#include "core/color/Color.h"

namespace core {

enum class BrushShapeType {
  Circle,
  Square
};

enum class BlendMode {
  Normal,
  Multiply,
  Add
};

struct BrushSettings {
  Color color {0, 0, 0, 255};
  int size {8};
  float opacity {1.0F};
  float hardness {1.0F};
  float flow {1.0F};
  float spacing {0.25F};
  bool antiAlias {true};
  float stabilization {0.0F};
  bool postCorrection {false};
  bool velocityBasedCorrection {false};
  BrushShapeType shapeType {BrushShapeType::Circle};
  BlendMode blendMode {BlendMode::Normal};
  bool eraseMode {false};
  bool lockAlphaRespect {false};
};

} // namespace core
