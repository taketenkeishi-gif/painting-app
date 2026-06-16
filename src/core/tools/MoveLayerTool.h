#pragma once

#include <string_view>

#include "core/common/Point.h"
#include "core/tools/ITool.h"

namespace core {

class MoveLayerTool : public ITool {
public:
  ToolKind kind() const noexcept override { return ToolKind::MoveLayer; }
  std::string_view displayName() const noexcept override { return "Move Layer"; }

  ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel(ToolContext& context) override;
  ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;
  ToolOverlayState overlay() const override;

private:
  bool m_dragging    {false};
  Point m_start      {0, 0};
  Point m_current    {0, 0};
  /// ドラッグ開始時点のレイヤーオフセット（ラスターレイヤー移動で使用）
  int m_baseOffsetX  {0};
  int m_baseOffsetY  {0};
};

} // namespace core
