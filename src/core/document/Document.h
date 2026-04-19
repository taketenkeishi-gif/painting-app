#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/common/Size.h"
#include "core/layer/Layer.h"

namespace core {

class Document {
public:
  Document(int width = 800, int height = 600);

  Size canvasSize() const noexcept { return m_canvasSize; }

  std::size_t layerCount() const noexcept { return m_layers.size(); }
  Layer& layerAt(std::size_t index);
  const Layer& layerAt(std::size_t index) const;

  std::size_t activeLayerIndex() const noexcept { return m_activeLayerIndex; }
  bool setActiveLayer(std::size_t index) noexcept;

  Layer* activeLayer() noexcept;
  const Layer* activeLayer() const noexcept;

  std::size_t addLayer(const std::string& name = {});
  bool removeLayer(std::size_t index) noexcept;
  bool renameLayer(std::size_t index, const std::string& newName);
  bool setLayerVisible(std::size_t index, bool visible) noexcept;
  bool setLayerOpacity(std::size_t index, float opacity) noexcept;

private:
  static std::string makeDefaultLayerName(std::size_t currentLayerCount);

  Size m_canvasSize;
  std::vector<Layer> m_layers;
  std::size_t m_activeLayerIndex {0};
};

} // namespace core
