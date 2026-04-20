#pragma once

#include <string>

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
  bool antiAlias {true};
  int stabilization {0};
  bool postCorrection {false};
  bool velocityBasedCorrection {false};
  core::BrushShapeType shapeType {core::BrushShapeType::Circle};
  core::BlendMode blendMode {core::BlendMode::Normal};
  bool eraseMode {false};
  bool lockAlphaRespect {false};
};

} // namespace app::ui
