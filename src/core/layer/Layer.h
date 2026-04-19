#pragma once

#include <string>

#include "core/buffer/PixelBuffer.h"

namespace core {

class Layer {
public:
  Layer(std::string name, int width, int height);

  const std::string& name() const noexcept { return m_name; }
  void setName(std::string name);

  bool visible() const noexcept { return m_visible; }
  void setVisible(bool visible) noexcept { m_visible = visible; }

  float opacity() const noexcept { return m_opacity; }
  void setOpacity(float opacity) noexcept;

  PixelBuffer& buffer() noexcept { return m_buffer; }
  const PixelBuffer& buffer() const noexcept { return m_buffer; }

private:
  std::string m_name;
  bool m_visible {true};
  float m_opacity {1.0F};
  PixelBuffer m_buffer;
};

} // namespace core
