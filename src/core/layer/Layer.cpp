#include "core/layer/Layer.h"

#include <algorithm>
#include <utility>

namespace core {

Layer::Layer(std::string name, int width, int height, LayerKind kind)
    : m_name(std::move(name)),
      m_kind(kind),
      m_buffer(width, height, Color::Transparent()),
      m_maskBuffer(width, height, Color::OpaqueWhite()) {}

void Layer::setName(std::string name) {
  m_name = std::move(name);
}

void Layer::setOpacity(float opacity) noexcept {
  m_opacity = std::clamp(opacity, 0.0F, 1.0F);
}

void Layer::addVectorPath(VectorPath path) {
  if (path.points.empty()) {
    return;
  }
  path.width = std::max(1, path.width);
  path.opacity = std::clamp(path.opacity, 0.0F, 1.0F);
  m_vectorPaths.push_back(std::move(path));
}

void Layer::clearVectorPaths() noexcept {
  m_vectorPaths.clear();
}

void Layer::moveVectorPathsBy(int dx, int dy) noexcept {
  if (dx == 0 && dy == 0) {
    return;
  }
  for (VectorPath& path : m_vectorPaths) {
    for (FPoint& point : path.points) {
      point.x += static_cast<float>(dx);
      point.y += static_cast<float>(dy);
    }
  }
}

void Layer::resetRasterBuffer(Color fill) noexcept {
  m_buffer.fill(fill);
}

void Layer::createMask(Color fill) noexcept {
  m_hasMask = true;
  m_maskEnabled = true;
  m_maskBuffer.resize(m_buffer.width(), m_buffer.height(), fill);
}

void Layer::removeMask() noexcept {
  m_hasMask = false;
  m_maskEnabled = false;
  m_maskBuffer.resize(m_buffer.width(), m_buffer.height(), Color::OpaqueWhite());
}

} // namespace core
