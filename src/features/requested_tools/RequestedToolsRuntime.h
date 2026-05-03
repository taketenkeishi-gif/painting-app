#pragma once

#include <optional>
#include <string_view>

#include "core/buffer/PixelBuffer.h"
#include "core/common/Point.h"
#include "core/tools/ToolContext.h"

namespace features::requested_tools {

class RequestedToolsRuntime {
public:
  bool handles(std::string_view subToolId) const noexcept;
  core::ToolResult onPointerPress(
      std::string_view subToolId,
      core::ToolContext& context,
      const core::ToolPointerEvent& event,
      const core::Color& color,
      int size,
      int opacityPercent);
  core::ToolResult onPointerMove(
      std::string_view subToolId,
      core::ToolContext& context,
      const core::ToolPointerEvent& event,
      const core::Color& color,
      int size,
      int opacityPercent);
  core::ToolResult onPointerRelease(
      std::string_view subToolId,
      core::ToolContext& context,
      const core::ToolPointerEvent& event,
      const core::Color& color,
      int size,
      int opacityPercent);
  void cancel() noexcept;

private:
  core::ToolResult pressGradient(core::ToolContext& context, const core::ToolPointerEvent& event);
  core::ToolResult moveGradient(core::ToolContext& context, const core::ToolPointerEvent& event);
  core::ToolResult releaseGradient(core::ToolContext& context, const core::ToolPointerEvent& event, const core::Color& color, int opacityPercent);
  core::ToolResult releaseComic(core::ToolContext& context, const core::ToolPointerEvent& event, const core::Color& color, int size, int opacityPercent);
  core::ToolResult pressText(core::ToolContext& context, const core::ToolPointerEvent& event, const core::Color& color, int size, int opacityPercent);
  core::ToolResult releaseRuler(core::ToolContext& context, const core::ToolPointerEvent& event, const core::Color& color, int size, int opacityPercent);
  core::ToolResult releaseLineCorrection(core::ToolContext& context);
  core::ToolResult pressCloneStamp(core::ToolContext& context, const core::ToolPointerEvent& event, int size, int opacityPercent);
  core::ToolResult moveCloneStamp(core::ToolContext& context, const core::ToolPointerEvent& event, int size, int opacityPercent);
  core::ToolResult releaseCloneStamp(core::ToolContext& context);
  core::ToolResult pressBlend(core::ToolContext& context, const core::ToolPointerEvent& event);
  core::ToolResult moveBlend(core::ToolContext& context, const core::ToolPointerEvent& event, int size, int opacityPercent);
  core::ToolResult pressLiquify(core::ToolContext& context, const core::ToolPointerEvent& event);
  core::ToolResult moveLiquify(core::ToolContext& context, const core::ToolPointerEvent& event, int size);
  core::ToolResult pressOperation(core::ToolContext& context, const core::ToolPointerEvent& event);
  core::ToolResult moveOperation(core::ToolContext& context, const core::ToolPointerEvent& event);
  core::ToolResult releaseOperation(core::ToolContext& context);

  core::Point m_dragStart {0, 0};
  core::Point m_lastPoint {0, 0};
  bool m_dragging {false};

  bool m_cloneHasSample {false};
  core::Point m_cloneSamplePoint {0, 0};
  std::optional<core::Point> m_cloneStrokeOrigin;
  std::optional<core::PixelBuffer> m_cloneSnapshot;
  std::optional<core::PixelBuffer> m_cloneWorkingSnapshot;

  std::optional<core::PixelBuffer> m_blendSnapshot;
  std::optional<core::PixelBuffer> m_liquifySnapshot;

  std::optional<std::size_t> m_operationSelectedPathIndex;
};

} // namespace features::requested_tools
