#include "core/document/Document.h"

#include <cstddef>
#include <stdexcept>

namespace core {

Document::Document(int width, int height)
    : m_canvasSize {width, height},
      m_selection(width, height) {
  addLayer("Layer 1");
}

Layer& Document::layerAt(std::size_t index) {
  if (index >= m_layers.size()) {
    throw std::out_of_range("Layer index out of range.");
  }
  return m_layers[index];
}

const Layer& Document::layerAt(std::size_t index) const {
  if (index >= m_layers.size()) {
    throw std::out_of_range("Layer index out of range.");
  }
  return m_layers[index];
}

bool Document::setActiveLayer(std::size_t index) noexcept {
  if (index >= m_layers.size()) {
    return false;
  }
  m_activeLayerIndex = index;
  return true;
}

Layer* Document::activeLayer() noexcept {
  if (m_layers.empty()) {
    return nullptr;
  }
  return &m_layers[m_activeLayerIndex];
}

const Layer* Document::activeLayer() const noexcept {
  if (m_layers.empty()) {
    return nullptr;
  }
  return &m_layers[m_activeLayerIndex];
}

std::size_t Document::addLayer(const std::string& name) {
  const std::string finalName = name.empty() ? makeDefaultLayerName(m_layers.size()) : name;
  m_layers.emplace_back(finalName, m_canvasSize.width, m_canvasSize.height);
  m_activeLayerIndex = m_layers.size() - 1;
  return m_activeLayerIndex;
}

bool Document::removeLayer(std::size_t index) noexcept {
  if (index >= m_layers.size() || m_layers.size() <= 1) {
    return false;
  }

  m_layers.erase(m_layers.begin() + static_cast<std::ptrdiff_t>(index));
  if (m_activeLayerIndex == index) {
    m_activeLayerIndex = index < m_layers.size() ? index : m_layers.size() - 1;
  } else if (m_activeLayerIndex > index) {
    --m_activeLayerIndex;
  }

  return true;
}

bool Document::renameLayer(std::size_t index, const std::string& newName) {
  if (index >= m_layers.size() || newName.empty()) {
    return false;
  }

  m_layers[index].setName(newName);
  return true;
}

bool Document::setLayerVisible(std::size_t index, bool visible) noexcept {
  if (index >= m_layers.size()) {
    return false;
  }
  m_layers[index].setVisible(visible);
  return true;
}

bool Document::setLayerOpacity(std::size_t index, float opacity) noexcept {
  if (index >= m_layers.size()) {
    return false;
  }
  m_layers[index].setOpacity(opacity);
  return true;
}

std::string Document::makeDefaultLayerName(std::size_t currentLayerCount) {
  return "Layer " + std::to_string(currentLayerCount + 1);
}

} // namespace core
