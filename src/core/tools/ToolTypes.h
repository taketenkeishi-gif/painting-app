#pragma once

#include "core/color/Color.h"

namespace core {

struct BrushSettings {
  Color color {0, 0, 0, 255};
  int size {8};
};

} // namespace core
