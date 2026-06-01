#pragma once

#include <algorithm>
#include <string_view>

#include "core/color/Color.h"
#include "core/common/FPoint.h"
#include "core/tools/ITool.h"
#include "core/tools/ToolTypes.h"

namespace core {

/// Photoshop / Krita 互換グラデーションツール
/// ドラッグで方向を指定し、リリース時にアクティブレイヤーへ塗りつぶしを適用する。
class GradientTool : public ITool {
public:
  enum class GradientType {
    Linear,  ///< 直線グラデーション
    Radial,  ///< 放射グラデーション
  };

  enum class GradientFill {
    ForegroundToBackground,  ///< 描画色 → 背景色
    ForegroundToTransparent, ///< 描画色 → 透明
  };

  ToolKind kind() const noexcept override { return ToolKind::Gradient; }
  std::string_view displayName() const noexcept override { return "Gradient"; }

  ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel(ToolContext& context) override;
  ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;
  ToolOverlayState overlay() const override;

  // ── 設定 ──────────────────────────────────────────────────────────────────
  void setGradientType(GradientType type) noexcept { m_type = type; }
  void setGradientFill(GradientFill fill) noexcept { m_fill = fill; }
  void setOpacity(float opacity) noexcept { m_opacity = std::clamp(opacity, 0.0f, 1.0f); }
  void setBlendMode(BlendMode blendMode) noexcept { m_blendMode = blendMode; }
  void setEraseMode(bool erase) noexcept { m_eraseMode = erase; }

  GradientType gradientType() const noexcept { return m_type; }
  GradientFill gradientFill() const noexcept { return m_fill; }

private:
  void applyGradient(ToolContext& context, const FPoint& start, const FPoint& end) const;
  Color sampleGradient(float t, const Color& fgColor, const Color& bgColor) const;
  Color blendWithDst(const Color& dst, const Color& src, int px, int py) const;

  GradientType m_type  {GradientType::Linear};
  GradientFill m_fill  {GradientFill::ForegroundToBackground};
  float        m_opacity    {1.0f};
  BlendMode    m_blendMode  {BlendMode::Normal};
  bool         m_eraseMode  {false};

  bool   m_drawing {false};
  FPoint m_start   {0.0f, 0.0f};
  FPoint m_current {0.0f, 0.0f};
};

} // namespace core
