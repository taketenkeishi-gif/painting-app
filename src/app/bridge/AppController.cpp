#include "app/bridge/AppController.h"

#include <algorithm>
#include <utility>

namespace app::bridge {

namespace {

bool pixelBuffersEqual(const core::PixelBuffer& lhs, const core::PixelBuffer& rhs) {
  if (lhs.width() != rhs.width() || lhs.height() != rhs.height()) {
    return false;
  }
  for (int y = 0; y < lhs.height(); ++y) {
    for (int x = 0; x < lhs.width(); ++x) {
      const core::Color a = lhs.pixel(x, y);
      const core::Color b = rhs.pixel(x, y);
      if (a.r != b.r || a.g != b.g || a.b != b.b || a.a != b.a) {
        return false;
      }
    }
  }
  return true;
}

} // namespace

AppController::AppController(QObject* parent)
    : QObject(parent),
      m_document(800, 600) {
  m_brushTool.setColor(core::Color::OpaqueBlack());
  m_brushTool.setSize(8);
  rerender();
}

std::vector<LayerViewModel> AppController::layerViewModels() const {
  std::vector<LayerViewModel> models;
  models.reserve(m_document.layerCount());
  for (std::size_t i = 0; i < m_document.layerCount(); ++i) {
    const core::Layer& layer = m_document.layerAt(i);
    models.push_back(LayerViewModel {
        layer.name(),
        layer.visible(),
        i == m_document.activeLayerIndex()});
  }
  return models;
}

ToolStateViewModel AppController::toolState() const noexcept {
  const core::BrushSettings& brush = m_brushTool.settings();
  return ToolStateViewModel {brush.color, brush.size};
}

void AppController::newDocument(int width, int height) {
  m_document = core::Document(width, height);
  m_layerCounter = 1;
  m_stroking = false;
  m_pendingStroke.reset();
  clearStrokeHistory();
  rerender();
  emit layersChanged();
  emit documentChanged();
}

void AppController::addLayer() {
  ++m_layerCounter;
  m_document.addLayer("Layer " + std::to_string(m_layerCounter));
  m_pendingStroke.reset();
  clearStrokeHistory();
  rerender();
  emit layersChanged();
  emit documentChanged();
}

bool AppController::removeLayer(std::size_t index) {
  if (!m_document.removeLayer(index)) {
    return false;
  }
  m_pendingStroke.reset();
  clearStrokeHistory();
  rerender();
  emit layersChanged();
  emit documentChanged();
  return true;
}

bool AppController::renameLayer(std::size_t index, const std::string& name) {
  if (!m_document.renameLayer(index, name)) {
    return false;
  }
  emit layersChanged();
  return true;
}

void AppController::setActiveLayer(std::size_t index) {
  if (!m_document.setActiveLayer(index)) {
    return;
  }
  emit layersChanged();
}

void AppController::setLayerVisible(std::size_t index, bool visible) {
  if (index >= m_document.layerCount()) {
    return;
  }

  const bool beforeVisible = m_document.layerAt(index).visible();
  if (beforeVisible == visible) {
    return;
  }

  if (!m_document.setLayerVisible(index, visible)) {
    return;
  }

  pushHistoryEntry(StrokeHistoryEntry {
      HistoryKind::LayerVisibility,
      index,
      core::PixelBuffer {},
      core::PixelBuffer {},
      beforeVisible,
      visible});
  rerender();
  emit layersChanged();
  emit documentChanged();
}

void AppController::beginStroke(int x, int y) {
  if (m_stroking) {
    return;
  }

  core::Layer* active = m_document.activeLayer();
  if (active == nullptr) {
    return;
  }

  m_pendingStroke = PendingStrokeState {m_document.activeLayerIndex(), active->buffer()};
  m_stroking = true;
  m_lastPoint = core::Point {x, y};
  m_brushTool.stroke(*active, m_lastPoint, m_lastPoint);
  rerender();
  emit documentChanged();
}

void AppController::continueStroke(int x, int y) {
  if (!m_stroking) {
    return;
  }
  core::Layer* active = m_document.activeLayer();
  if (active == nullptr) {
    return;
  }
  const core::Point nextPoint {x, y};
  m_brushTool.stroke(*active, m_lastPoint, nextPoint);
  m_lastPoint = nextPoint;
  rerender();
  emit documentChanged();
}

void AppController::endStroke() {
  if (!m_stroking) {
    return;
  }

  m_stroking = false;
  if (!m_pendingStroke.has_value()) {
    return;
  }

  const std::size_t layerIndex = m_pendingStroke->layerIndex;
  if (layerIndex >= m_document.layerCount()) {
    m_pendingStroke.reset();
    return;
  }

  const core::PixelBuffer after = m_document.layerAt(layerIndex).buffer();
  if (!pixelBuffersEqual(m_pendingStroke->before, after)) {
    pushHistoryEntry(StrokeHistoryEntry {
        HistoryKind::Stroke,
        layerIndex,
        m_pendingStroke->before,
        after,
        true,
        true});
  }
  m_pendingStroke.reset();
}

bool AppController::undo() {
  if (m_stroking) {
    return false;
  }
  if (m_undoHistory.empty()) {
    return false;
  }

  StrokeHistoryEntry entry = std::move(m_undoHistory.back());
  m_undoHistory.pop_back();

  if (entry.layerIndex >= m_document.layerCount()) {
    clearStrokeHistory();
    return false;
  }

  if (entry.kind == HistoryKind::Stroke) {
    m_document.layerAt(entry.layerIndex).buffer() = entry.before;
  } else {
    m_document.setLayerVisible(entry.layerIndex, entry.beforeVisible);
  }

  if (m_redoHistory.size() >= m_maxStrokeHistory) {
    m_redoHistory.erase(m_redoHistory.begin());
  }
  m_redoHistory.push_back(std::move(entry));
  rerender();
  if (m_redoHistory.back().kind == HistoryKind::LayerVisibility) {
    emit layersChanged();
  }
  emit documentChanged();
  return true;
}

bool AppController::redo() {
  if (m_stroking) {
    return false;
  }
  if (m_redoHistory.empty()) {
    return false;
  }

  StrokeHistoryEntry entry = std::move(m_redoHistory.back());
  m_redoHistory.pop_back();

  if (entry.layerIndex >= m_document.layerCount()) {
    clearStrokeHistory();
    return false;
  }

  if (entry.kind == HistoryKind::Stroke) {
    m_document.layerAt(entry.layerIndex).buffer() = entry.after;
  } else {
    m_document.setLayerVisible(entry.layerIndex, entry.afterVisible);
  }

  if (m_undoHistory.size() >= m_maxStrokeHistory) {
    m_undoHistory.erase(m_undoHistory.begin());
  }
  m_undoHistory.push_back(std::move(entry));
  rerender();
  if (m_undoHistory.back().kind == HistoryKind::LayerVisibility) {
    emit layersChanged();
  }
  emit documentChanged();
  return true;
}

bool AppController::canUndo() const noexcept {
  return !m_undoHistory.empty();
}

bool AppController::canRedo() const noexcept {
  return !m_redoHistory.empty();
}

std::string AppController::nextUndoActionName() const {
  if (m_undoHistory.empty()) {
    return {};
  }
  switch (m_undoHistory.back().kind) {
    case HistoryKind::Stroke:
      return "Stroke";
    case HistoryKind::LayerVisibility:
      return "Visibility";
    default:
      return {};
  }
}

std::string AppController::nextRedoActionName() const {
  if (m_redoHistory.empty()) {
    return {};
  }
  switch (m_redoHistory.back().kind) {
    case HistoryKind::Stroke:
      return "Stroke";
    case HistoryKind::LayerVisibility:
      return "Visibility";
    default:
      return {};
  }
}

void AppController::setBrushColor(const core::Color& color) {
  const core::Color current = m_brushTool.settings().color;
  if (current.r == color.r && current.g == color.g && current.b == color.b && current.a == color.a) {
    return;
  }
  m_brushTool.setColor(color);
  emit toolStateChanged();
}

void AppController::setBrushSize(int size) {
  const int normalized = size < 1 ? 1 : size;
  if (m_brushTool.settings().size == normalized) {
    return;
  }
  m_brushTool.setSize(normalized);
  emit toolStateChanged();
}

void AppController::pushHistoryEntry(StrokeHistoryEntry entry) {
  if (entry.kind == HistoryKind::LayerVisibility && !m_undoHistory.empty()) {
    StrokeHistoryEntry& last = m_undoHistory.back();
    if (last.kind == HistoryKind::LayerVisibility &&
        last.layerIndex == entry.layerIndex &&
        last.afterVisible == entry.beforeVisible) {
      last.afterVisible = entry.afterVisible;
      if (last.beforeVisible == last.afterVisible) {
        m_undoHistory.pop_back();
      }
      m_redoHistory.clear();
      return;
    }
  }

  if (m_undoHistory.size() >= m_maxStrokeHistory) {
    m_undoHistory.erase(m_undoHistory.begin());
  }
  m_undoHistory.push_back(std::move(entry));
  m_redoHistory.clear();
}

void AppController::clearStrokeHistory() noexcept {
  m_undoHistory.clear();
  m_redoHistory.clear();
}

void AppController::rerender() {
  m_composited = m_renderer.composite(m_document);
}

} // namespace app::bridge
