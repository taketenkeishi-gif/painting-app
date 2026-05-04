#pragma once

#include <optional>
#include <string_view>

#include "core/buffer/PixelBuffer.h"
#include "core/common/Point.h"
#include "core/layer/Layer.h"
#include "core/tools/ToolContext.h"

namespace features::requested_tools {

class RequestedToolsRuntime {
public:
  struct SelectionState {
    std::size_t layerIndex {0};
    std::size_t pathIndex {0};
    core::VectorPath::Kind pathKind {core::VectorPath::Kind::Stroke};
    core::Rect bounds {0, 0, 0, 0};
    bool valid {false};
  };

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
  SelectionState selectionState() const noexcept { return m_selectionState; }
  std::optional<core::Point> cloneSamplePoint() const noexcept {
    if (!m_cloneHasSample) {
      return std::nullopt;
    }
    return m_cloneSamplePoint;
  }

private:
  enum class OperationHandle {
    None,
    Move,
    RulerStart,
    RulerEnd,
    RectTopLeft,
    RectTopRight,
    RectBottomLeft,
    RectBottomRight
  };

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

  SelectionState m_selectionState;
  OperationHandle m_operationHandle {OperationHandle::None};
};

} // namespace features::requested_tools
