#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/buffer/PixelBuffer.h"
#include "core/common/Point.h"

namespace core {

enum class LayerKind {
  Raster,
  Vector
};

struct VectorPath {
  std::vector<Point> points;
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

  bool visible() const noexcept { return m_visible; }
  void setVisible(bool visible) noexcept { m_visible = visible; }

  float opacity() const noexcept { return m_opacity; }
  void setOpacity(float opacity) noexcept;

  PixelBuffer& buffer() noexcept { return m_buffer; }
  const PixelBuffer& buffer() const noexcept { return m_buffer; }

  std::vector<VectorPath>& vectorPaths() noexcept { return m_vectorPaths; }
  const std::vector<VectorPath>& vectorPaths() const noexcept { return m_vectorPaths; }
  void addVectorPath(VectorPath path);
  void clearVectorPaths() noexcept;
  void moveVectorPathsBy(int dx, int dy) noexcept;

private:
  std::string m_name;
  LayerKind m_kind {LayerKind::Raster};
  bool m_visible {true};
  float m_opacity {1.0F};
  PixelBuffer m_buffer;
  std::vector<VectorPath> m_vectorPaths;
};

} // namespace core
