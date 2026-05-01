#pragma once

#include <string_view>

#include "core/tools/BrushTool.h"
#include "core/tools/ToolType.h"

namespace core {

class PenTool : public BrushTool {
public:
  ToolKind kind() const noexcept override { return ToolKind::Pen; }
  std::string_view displayName() const noexcept override { return "Pen"; }
};

} // namespace core