#include "app/bridge/AppController.h"

#include <algorithm>
#include <memory>
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
  auto brush = std::make_unique<core::BrushTool>();
  m_brushTool = brush.get();
  m_toolManager.registerTool(std::move(brush));

  auto eraser = std::make_unique<core::EraserTool>();
  m_eraserTool = eraser.get();
  m_toolManager.registerTool(std::move(eraser));

  m_toolManager.registerTool(std::make_unique<core::EyedropperTool>());
  m_toolManager.registerTool(std::make_unique<core::HandTool>());
  m_toolManager.registerTool(std::make_unique<core::ZoomTool>());
  m_toolManager.registerTool(std::make_unique<core::LineTool>());
  m_toolManager.registerTool(std::make_unique<core::RectSelectionTool>());
  m_toolManager.registerTool(std::make_unique<core::FillTool>());
  m_toolManager.registerTool(std::make_unique<core::MoveLayerTool>());
  m_toolManager.setActiveTool(core::ToolKind::Brush);

  setBrushColor(core::Color::OpaqueBlack());
  setBrushSize(8);
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
  return ToolStateViewModel {m_currentColor, m_brushSize};
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

bool AppController::setCurrentTool(core::ToolKind kind) {
  if (!m_toolManager.setActiveTool(kind)) {
    return false;
  }
  emit toolStateChanged();
  return true;
}

core::ToolKind AppController::currentTool() const noexcept {
  return m_toolManager.activeToolKind();
}

void AppController::beginStroke(int x, int y) {
  if (m_stroking) {
    return;
  }

  const core::ToolKind activeKind = m_toolManager.activeToolKind();
  if (toolWritesPixels(activeKind)) {
    core::Layer* activeLayer = m_document.activeLayer();
    if (activeLayer == nullptr) {
      return;
    }
    m_pendingStroke = PendingStrokeState {m_document.activeLayerIndex(), activeLayer->buffer()};
  }

  m_stroking = true;
  m_lastPointer = core::Point {x, y};
  core::ToolPointerEvent pressEvent;
  pressEvent.point = m_lastPointer;
  core::ToolContext context = makeToolContext();
  const core::ToolResult result = m_toolManager.pointerPress(
      context,
      pressEvent);
  applyToolResult(result);
}

void AppController::continueStroke(int x, int y) {
  if (!m_stroking) {
    return;
  }

  m_lastPointer = core::Point {x, y};
  core::ToolPointerEvent moveEvent;
  moveEvent.point = m_lastPointer;
  core::ToolContext context = makeToolContext();
  const core::ToolResult result = m_toolManager.pointerMove(
      context,
      moveEvent);
  applyToolResult(result);
}

void AppController::endStroke() {
  if (!m_stroking) {
    return;
  }

  m_stroking = false;
  core::ToolPointerEvent releaseEvent;
  releaseEvent.point = m_lastPointer;
  core::ToolContext context = makeToolContext();
  const core::ToolResult result = m_toolManager.pointerRelease(
      context,
      releaseEvent);
  applyToolResult(result);
  finishPendingStrokeHistory();
}

bool AppController::pickColorAt(int x, int y) {
  const core::ToolKind previous = m_toolManager.activeToolKind();
  if (!m_toolManager.setActiveTool(core::ToolKind::Eyedropper)) {
    return false;
  }

  core::ToolPointerEvent event;
  event.point = core::Point {x, y};
  core::ToolContext context = makeToolContext();
  const core::ToolResult result = m_toolManager.pointerPress(
      context, event);
  m_toolManager.pointerRelease(context, event);
  m_toolManager.setActiveTool(previous);

  if (!result.sampledColor.has_value()) {
    return false;
  }
  setBrushColor(*result.sampledColor);
  return true;
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
  if (m_currentColor.r == color.r && m_currentColor.g == color.g &&
      m_currentColor.b == color.b && m_currentColor.a == color.a) {
    return;
  }

  m_currentColor = color;
  if (m_brushTool != nullptr) {
    m_brushTool->setColor(color);
  }
  emit toolStateChanged();
}

void AppController::setBrushSize(int size) {
  const int normalized = size < 1 ? 1 : size;
  if (m_brushSize == normalized) {
    return;
  }

  m_brushSize = normalized;
  if (m_brushTool != nullptr) {
    m_brushTool->setSize(normalized);
  }
  if (m_eraserTool != nullptr) {
    m_eraserTool->setSize(normalized);
  }
  emit toolStateChanged();
}

bool AppController::toolWritesPixels(core::ToolKind kind) noexcept {
  switch (kind) {
    case core::ToolKind::Brush:
    case core::ToolKind::Eraser:
    case core::ToolKind::Line:
    case core::ToolKind::Fill:
    case core::ToolKind::MoveLayer:
      return true;
    default:
      return false;
  }
}

core::ToolContext AppController::makeToolContext() {
  return core::ToolContext {
      m_document,
      m_composited,
      m_currentColor,
      m_brushSize};
}

void AppController::applyToolResult(const core::ToolResult& result) {
  bool changed = false;
  if (result.sampledColor.has_value()) {
    setBrushColor(*result.sampledColor);
  }
  if (result.pixelsChanged) {
    rerender();
    changed = true;
  }
  if (result.selectionChanged) {
    changed = true;
  }
  if (changed || result.viewportChanged) {
    emit documentChanged();
  }
}

void AppController::finishPendingStrokeHistory() {
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
