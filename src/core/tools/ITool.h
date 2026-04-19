#pragma once

#include <string_view>

#include "core/tools/ToolContext.h"
#include "core/tools/ToolType.h"

namespace core {

class ITool {
public:
  virtual ~ITool() = default;

  virtual ToolKind kind() const noexcept = 0;
  virtual std::string_view displayName() const noexcept = 0;

  virtual ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) = 0;
  virtual ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) = 0;
  virtual ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) = 0;
  virtual ToolResult onCancel(ToolContext& context) = 0;
  virtual ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) = 0;

  virtual ToolOverlayState overlay() const { return {}; }
};

} // namespace core
