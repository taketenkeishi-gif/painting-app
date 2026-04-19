#include "core/layer/Layer.h"

#include <algorithm>
#include <utility>

namespace core {

Layer::Layer(std::string name, int width, int height)
    : m_name(std::move(name)),
      m_buffer(width, height, Color::Transparent()) {}

void Layer::setName(std::string name) {
  m_name = std::move(name);
}

void Layer::setOpacity(float opacity) noexcept {
  m_opacity = std::clamp(opacity, 0.0F, 1.0F);
}

} // namespace core
