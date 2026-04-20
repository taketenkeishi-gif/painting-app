#pragma once

#include <string>

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
  bool eraseMode {false};
};

} // namespace app::ui
