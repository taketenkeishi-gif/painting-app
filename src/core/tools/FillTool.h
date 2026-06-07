#pragma once

#include <algorithm>
#include <string_view>

#include "core/tools/ITool.h"

namespace core {

class FillTool : public ITool {
public:
  struct Settings {
    int threshold {0};
    bool contiguous {true};
    bool referAllLayers {false};
    int gapClose {0};
    bool eraseMode {false};   // 透明色で塗りつぶし（消去）
  };

  ToolKind kind() const noexcept override { return ToolKind::Fill; }
  std::string_view displayName() const noexcept override { return "Fill"; }

  ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel(ToolContext& context) override;
  ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;

  void setThreshold(int threshold) noexcept { m_settings.threshold = std::clamp(threshold, 0, 255); }
  void setContiguous(bool contiguous) noexcept { m_settings.contiguous = contiguous; }
  void setReferAllLayers(bool enabled) noexcept { m_settings.referAllLayers = enabled; }
  void setGapClose(int gapClose) noexcept { m_settings.gapClose = std::clamp(gapClose, 0, 8); }
  void setEraseMode(bool erase) noexcept { m_settings.eraseMode = erase; }
  const Settings& settings() const noexcept { return m_settings; }

private:
  static bool  isSameColor(const Color& a, const Color& b) noexcept;
  static float colorDistance(const Color& a, const Color& b) noexcept;  // 知覚的ユークリッド距離
  bool matchesTarget(const PixelBuffer& source, const Color& target, int x, int y) const noexcept;
  bool hasBridge(const PixelBuffer& source, const Color& target, int x, int y) const noexcept;

  Settings m_settings;
};

} // namespace core
