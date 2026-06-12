#include "core/document/Document.h"

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "core/buffer/PixelBuffer.h"

namespace core {

Document::Document(int width, int height, int dpi)
    : m_canvasSize {width, height},
      m_dpi(dpi > 0 ? dpi : 72),
      m_selection(width, height) {
  addRasterLayer("Layer 1");
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

std::size_t Document::addLayer(const std::string& name, LayerKind kind) {
  const std::string finalName = name.empty() ? makeDefaultLayerName(m_layers.size()) : name;
  m_layers.emplace_back(finalName, m_canvasSize.width, m_canvasSize.height, kind);
  m_layers.back().setPaperLayer(false);
  m_layers.back().setBlendMode(BlendMode::Normal);
  m_layers.back().setId(m_nextLayerId++);  // 安定ID を採番
  m_activeLayerIndex = m_layers.size() - 1;
  return m_activeLayerIndex;
}

std::size_t Document::addRasterLayer(const std::string& name) {
  return addLayer(name, LayerKind::Raster);
}

std::size_t Document::addVectorLayer(const std::string& name) {
  return addLayer(name, LayerKind::Vector);
}

std::size_t Document::addFolderLayer(const std::string& name) {
  return addLayer(name, LayerKind::Folder);
}

std::size_t Document::addAdjustmentLayer(const AdjustmentParams& params, const std::string& name) {
  const std::size_t idx = addLayer(name.empty() ? "Adjustment" : name, LayerKind::Adjustment);
  m_layers[idx].setAdjustmentParams(params);
  return idx;
}

std::size_t Document::duplicateLayer(std::size_t index) {
  if (index >= m_layers.size()) {
    return m_activeLayerIndex;
  }

  Layer duplicated = m_layers[index];
  duplicated.setName(duplicated.name() + " Copy");
  duplicated.setId(m_nextLayerId++);  // 複製レイヤーは新しい ID を得る（元と別物）
  const auto insertPos = m_layers.begin() + static_cast<std::ptrdiff_t>(index + 1);
  m_layers.insert(insertPos, duplicated);
  m_activeLayerIndex = index + 1;
  return m_activeLayerIndex;
}

bool Document::removeLayer(std::size_t index) noexcept {
  if (index >= m_layers.size()) {
    return false;
  }

  m_layers.erase(m_layers.begin() + static_cast<std::ptrdiff_t>(index));
  if (m_layers.empty()) {
    m_activeLayerIndex = 0;
  } else if (m_activeLayerIndex == index) {
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

bool Document::moveLayer(std::size_t fromIndex, std::size_t toIndex) noexcept {
  if (fromIndex >= m_layers.size() || toIndex >= m_layers.size() || fromIndex == toIndex) {
    return false;
  }
  if (m_layers[fromIndex].isPaperLayer() || m_layers[toIndex].isPaperLayer()) {
    return false;
  }

  Layer movedLayer = std::move(m_layers[fromIndex]);
  m_layers.erase(m_layers.begin() + static_cast<std::ptrdiff_t>(fromIndex));
  m_layers.insert(m_layers.begin() + static_cast<std::ptrdiff_t>(toIndex), std::move(movedLayer));

  if (m_activeLayerIndex == fromIndex) {
    m_activeLayerIndex = toIndex;
  } else if (fromIndex < m_activeLayerIndex && m_activeLayerIndex <= toIndex) {
    --m_activeLayerIndex;
  } else if (toIndex <= m_activeLayerIndex && m_activeLayerIndex < fromIndex) {
    ++m_activeLayerIndex;
  }

  return true;
}

bool Document::moveLayerUp(std::size_t index) noexcept {
  if (index + 1 >= m_layers.size()) {
    return false;
  }
  return moveLayer(index, index + 1);
}

bool Document::moveLayerDown(std::size_t index) noexcept {
  if (index == 0 || index >= m_layers.size()) {
    return false;
  }
  return moveLayer(index, index - 1);
}

bool Document::resizeCanvas(int newWidth, int newHeight, int offsetX, int offsetY) {
  if (newWidth <= 0 || newHeight <= 0) {
    return false;
  }
  if (newWidth == m_canvasSize.width && newHeight == m_canvasSize.height && offsetX == 0 && offsetY == 0) {
    return false; // no-op
  }

  // 各ラスターレイヤーのバッファをリサイズ
  for (Layer& layer : m_layers) {
    if (layer.kind() != LayerKind::Raster) {
      continue;
    }
    const PixelBuffer old = layer.buffer();
    PixelBuffer newBuf(newWidth, newHeight, Color::Transparent());
    // old のコンテンツを (offsetX, offsetY) に転写
    const int srcW = old.width();
    const int srcH = old.height();
    for (int sy = 0; sy < srcH; ++sy) {
      const int dy = sy + offsetY;
      if (dy < 0 || dy >= newHeight) continue;
      for (int sx = 0; sx < srcW; ++sx) {
        const int dx = sx + offsetX;
        if (dx < 0 || dx >= newWidth) continue;
        newBuf.setPixel(dx, dy, old.pixel(sx, sy));
      }
    }
    layer.buffer() = std::move(newBuf);

    // マスクも同様にリサイズ
    if (layer.hasMask()) {
      const PixelBuffer oldMask = layer.maskBuffer();
      PixelBuffer newMask(newWidth, newHeight, Color::Transparent());
      for (int sy = 0; sy < oldMask.height(); ++sy) {
        const int dy = sy + offsetY;
        if (dy < 0 || dy >= newHeight) continue;
        for (int sx = 0; sx < oldMask.width(); ++sx) {
          const int dx = sx + offsetX;
          if (dx < 0 || dx >= newWidth) continue;
          newMask.setPixel(dx, dy, oldMask.pixel(sx, sy));
        }
      }
      layer.maskBuffer() = std::move(newMask);
    }
  }

  // ベクターレイヤーのパスはオフセットをかける
  for (Layer& layer : m_layers) {
    if (layer.kind() != LayerKind::Vector) {
      continue;
    }
    if (offsetX == 0 && offsetY == 0) {
      continue;
    }
    for (VectorPath& path : layer.vectorPaths()) {
      for (FPoint& pt : path.points) {
        pt.x += static_cast<float>(offsetX);
        pt.y += static_cast<float>(offsetY);
      }
    }
  }

  m_canvasSize = {newWidth, newHeight};
  m_selection = SelectionMask(newWidth, newHeight);
  return true;
}

void Document::clearLayersForLoad() noexcept {
  m_layers.clear();
  m_activeLayerIndex = 0;
}

std::size_t Document::insertLoadedLayer(Layer layer) {
  m_layers.push_back(std::move(layer));
  return m_layers.size() - 1;
}

void Document::insertLayerAt(std::size_t index, Layer layer) {
  if (index > m_layers.size()) {
    index = m_layers.size();
  }
  m_layers.insert(m_layers.begin() + static_cast<std::ptrdiff_t>(index), std::move(layer));
  // activeLayerIndex をシフト（挿入位置以降にあるインデックスを +1）
  if (m_activeLayerIndex >= index && m_layers.size() > 1) {
    ++m_activeLayerIndex;
  }
  m_activeLayerIndex = std::min(m_activeLayerIndex, m_layers.size() - 1);
}

std::string Document::makeDefaultLayerName(std::size_t currentLayerCount) {
  return "Layer " + std::to_string(currentLayerCount + 1);
}

} // namespace core
