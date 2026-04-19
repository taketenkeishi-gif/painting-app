#pragma once

#include <map>
#include <memory>

#include "core/tools/ITool.h"

namespace core {

class ToolManager {
public:
  bool registerTool(std::unique_ptr<ITool> tool);
  bool setActiveTool(ToolKind kind) noexcept;

  ToolKind activeToolKind() const noexcept { return m_activeToolKind; }
  bool hasTool(ToolKind kind) const noexcept;

  ITool* activeTool() noexcept;
  const ITool* activeTool() const noexcept;

  ToolResult pointerPress(ToolContext& context, const ToolPointerEvent& event);
  ToolResult pointerMove(ToolContext& context, const ToolPointerEvent& event);
  ToolResult pointerRelease(ToolContext& context, const ToolPointerEvent& event);
  ToolResult cancel(ToolContext& context);
  ToolResult wheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event);

  ToolOverlayState overlay() const;

private:
  std::map<ToolKind, std::unique_ptr<ITool>> m_tools;
  ToolKind m_activeToolKind {ToolKind::Brush};
};

} // namespace core
