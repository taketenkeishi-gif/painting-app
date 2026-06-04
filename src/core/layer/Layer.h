#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/buffer/PixelBuffer.h"
#include "core/common/FPoint.h"
#include "core/common/Point.h"
#include "core/tools/ToolTypes.h"

namespace core {

enum class LayerKind {
  Raster,
  Vector,
  Folder,
  Adjustment  ///< 非破壊調整レイヤー（Photoshop Adjustment Layer）
};

// ── Adjustment Layer ──────────────────────────────────────────────────────
enum class AdjustmentKind {
  BrightnessContrast,
  HueSaturation,
  ColorBalance,
  Levels,
  Curves,
  GradientMap,
  Invert,
  Threshold,
  Vibrance,
};

/// 各調整レイヤーのパラメータを一つの struct にまとめる。
/// 使用するフィールドは AdjustmentKind によって異なる。
struct AdjustmentParams {
  AdjustmentKind kind     {AdjustmentKind::BrightnessContrast};
  // BrightnessContrast
  float brightness        {0.0f};   ///< -1.0 〜 +1.0
  float contrast          {0.0f};   ///< -1.0 〜 +1.0
  // HueSaturation / Vibrance
  float hue               {0.0f};   ///< -180 〜 +180 (degrees)
  float saturation        {0.0f};   ///< -1.0 〜 +1.0
  float lightness         {0.0f};   ///< -1.0 〜 +1.0
  float vibrance          {0.0f};   ///< -1.0 〜 +1.0
  // Levels
  float inputBlack        {0.0f};   ///< 0.0 〜 1.0
  float inputWhite        {1.0f};   ///< 0.0 〜 1.0
  float gamma             {1.0f};   ///< 0.1 〜 9.99
  float outputBlack       {0.0f};   ///< 0.0 〜 1.0
  float outputWhite       {1.0f};   ///< 0.0 〜 1.0
  // Threshold
  float threshold         {0.5f};   ///< 0.0 〜 1.0
};

struct VectorPath {
  std::vector<FPoint> points;
  Color color {0, 0, 0, 255};
  int width {1};
  float opacity {1.0F};
};

class Layer {
public:
  Layer(std::string name, int width, int height, LayerKind kind = LayerKind::Raster);

  const std::string& name() const noexcept { return m_name; }
  void setName(std::string name);

  LayerKind kind() const noexcept { return m_kind; }
  bool isRaster() const noexcept { return m_kind == LayerKind::Raster; }
  bool isVector() const noexcept { return m_kind == LayerKind::Vector; }
  bool isFolder() const noexcept { return m_kind == LayerKind::Folder; }
  void setKind(LayerKind kind) noexcept { m_kind = kind; }

  bool visible() const noexcept { return m_visible; }
  void setVisible(bool visible) noexcept { m_visible = visible; }

  float opacity() const noexcept { return m_opacity; }
  void setOpacity(float opacity) noexcept;
  BlendMode blendMode() const noexcept { return m_blendMode; }
  void setBlendMode(BlendMode mode) noexcept { m_blendMode = mode; }

  bool isPaperLayer() const noexcept { return m_isPaperLayer; }
  void setPaperLayer(bool paperLayer) noexcept { m_isPaperLayer = paperLayer; }

  PixelBuffer& buffer() noexcept { return m_buffer; }
  const PixelBuffer& buffer() const noexcept { return m_buffer; }

  std::vector<VectorPath>& vectorPaths() noexcept { return m_vectorPaths; }
  const std::vector<VectorPath>& vectorPaths() const noexcept { return m_vectorPaths; }
  void addVectorPath(VectorPath path);
  void clearVectorPaths() noexcept;
  void moveVectorPathsBy(int dx, int dy) noexcept;
  void resetRasterBuffer(Color fill = Color::Transparent()) noexcept;

  bool clippedToBelow() const noexcept { return m_clippedToBelow; }
  void setClippedToBelow(bool enabled) noexcept { m_clippedToBelow = enabled; }

  bool locked() const noexcept { return m_locked; }
  void setLocked(bool enabled) noexcept { m_locked = enabled; }
  bool alphaLocked() const noexcept { return m_alphaLocked; }
  void setAlphaLocked(bool enabled) noexcept { m_alphaLocked = enabled; }
  bool positionLocked() const noexcept { return m_positionLocked; }
  void setPositionLocked(bool enabled) noexcept { m_positionLocked = enabled; }

  bool hasMask() const noexcept { return m_hasMask; }
  bool maskEnabled() const noexcept { return m_maskEnabled; }
  void setMaskEnabled(bool enabled) noexcept { m_maskEnabled = m_hasMask && enabled; }
  PixelBuffer& maskBuffer() noexcept { return m_maskBuffer; }
  const PixelBuffer& maskBuffer() const noexcept { return m_maskBuffer; }
  void createMask(Color fill = Color::OpaqueWhite()) noexcept;
  void removeMask() noexcept;

  // Adjustment Layer
  bool isAdjustment() const noexcept { return m_kind == LayerKind::Adjustment; }
  const AdjustmentParams& adjustmentParams() const noexcept { return m_adjParams; }
  void setAdjustmentParams(const AdjustmentParams& p) noexcept { m_adjParams = p; }

private:
  std::string m_name;
  LayerKind m_kind {LayerKind::Raster};
  bool m_visible {true};
  float m_opacity {1.0F};
  BlendMode m_blendMode {BlendMode::Normal};
  bool m_isPaperLayer {false};
  PixelBuffer m_buffer;
  bool m_clippedToBelow {false};
  bool m_locked {false};
  bool m_alphaLocked {false};
  bool m_positionLocked {false};
  bool m_hasMask {false};
  bool m_maskEnabled {false};
  PixelBuffer m_maskBuffer;
  std::vector<VectorPath> m_vectorPaths;
  AdjustmentParams m_adjParams;
};

} // namespace core
