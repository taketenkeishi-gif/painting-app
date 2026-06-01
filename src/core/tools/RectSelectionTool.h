#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include <string_view>

#include "core/common/Point.h"
#include "core/tools/ITool.h"

namespace core {

class RectSelectionTool : public ITool {
public:
  enum class Mode {
    Rectangle,
    Lasso,
    AutoSelect
  };

  ToolKind kind() const noexcept override { return ToolKind::RectSelection; }
  std::string_view displayName() const noexcept override { return "Rect Selection"; }

  ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel(ToolContext& context) override;
  ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;
  ToolOverlayState overlay() const override;

  void setMode(Mode mode) noexcept {
    if (m_mode != mode) {
      m_committedLassoPoints.clear();
    }
    m_mode = mode;
  }
  Mode mode() const noexcept { return m_mode; }
  void setAutoSelectThreshold(int threshold) noexcept { m_autoSelectThreshold = std::clamp(threshold, 0, 255); }
  void setAutoSelectContiguous(bool contiguous) noexcept { m_autoSelectContiguous = contiguous; }
  void setAutoSelectReferAllLayers(bool enabled) noexcept { m_autoSelectReferAllLayers = enabled; }

private:
  static Rect normalizeRect(const Point& a, const Point& b);
  static bool pointInPolygon(const std::vector<Point>& polygon, int x, int y);
  static int colorDistance(const Color& a, const Color& b) noexcept;
  bool applyAutoSelect(ToolContext& context, const Point& seed);
  bool applyLassoSelection(ToolContext& context);

  Mode m_mode {Mode::Rectangle};
  bool m_selecting {false};
  Point m_start {0, 0};
  Point m_current {0, 0};
  std::vector<Point> m_lassoPoints;
  std::vector<Point> m_committedLassoPoints;  // shown after commit until next op
  int m_autoSelectThreshold {16};
  bool m_autoSelectContiguous {true};
  bool m_autoSelectReferAllLayers {true};
};

} // namespace core
