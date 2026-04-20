#include "core/layer/Layer.h"

#include <algorithm>
#include <utility>

namespace core {

Layer::Layer(std::string name, int width, int height, LayerKind kind)
    : m_name(std::move(name)),
      m_kind(kind),
      m_buffer(width, height, Color::Transparent()) {}

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
    for (Point& point : path.points) {
      point.x += dx;
      point.y += dy;
    }
  }
}

void Layer::resetRasterBuffer(Color fill) noexcept {
  m_buffer.fill(fill);
}

} // namespace core
