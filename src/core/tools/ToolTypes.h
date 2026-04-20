#pragma once

#include "core/color/Color.h"

namespace core {

struct BrushSettings {
  Color color {0, 0, 0, 255};
  int size {8};
  float opacity {1.0F};
  float hardness {1.0F};
  float flow {1.0F};
  float spacing {0.25F};
};

} // namespace core
