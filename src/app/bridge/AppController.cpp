#include "app/bridge/AppController.h"

#include <algorithm>
#include <cmath>
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

bool vectorPathsEqual(const std::vector<core::VectorPath>& lhs, const std::vector<core::VectorPath>& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    const core::VectorPath& a = lhs[i];
    const core::VectorPath& b = rhs[i];
    if (a.width != b.width || std::abs(a.opacity - b.opacity) > 0.0001F ||
        a.color.r != b.color.r || a.color.g != b.color.g || a.color.b != b.color.b || a.color.a != b.color.a ||
        a.points.size() != b.points.size()) {
      return false;
    }
    for (std::size_t j = 0; j < a.points.size(); ++j) {
      if (a.points[j].x != b.points[j].x || a.points[j].y != b.points[j].y) {
        return false;
      }
    }
  }
  return true;
}

bool layersEqual(const core::Layer& lhs, const core::Layer& rhs) {
  if (lhs.kind() != rhs.kind()) {
    return false;
  }
  if (lhs.clippedToBelow() != rhs.clippedToBelow()) {
    return false;
  }
  if (lhs.locked() != rhs.locked() || lhs.alphaLocked() != rhs.alphaLocked() ||
      lhs.positionLocked() != rhs.positionLocked()) {
    return false;
  }
  if (lhs.hasMask() != rhs.hasMask() || lhs.maskEnabled() != rhs.maskEnabled()) {
    return false;
  }
  if (lhs.hasMask() && !pixelBuffersEqual(lhs.maskBuffer(), rhs.maskBuffer())) {
    return false;
  }
  if (!pixelBuffersEqual(lhs.buffer(), rhs.buffer())) {
    return false;
  }
  return vectorPathsEqual(lhs.vectorPaths(), rhs.vectorPaths());
}

bool containsProperty(const std::vector<app::ui::ToolPropertyKey>& properties, app::ui::ToolPropertyKey key) {
  return std::find(properties.begin(), properties.end(), key) != properties.end();
}

bool containsProperty(
    const app::ui::ToolDescriptor* descriptor,
    const app::ui::SubToolDescriptor* subTool,
    app::ui::ToolPropertyKey key) {
  if (subTool != nullptr && !subTool->editableProperties.empty()) {
    return containsProperty(subTool->editableProperties, key);
  }
  if (descriptor == nullptr) {
    return false;
  }
  return containsProperty(descriptor->availableProperties, key);
}

int clampPercent(int value) {
  return std::clamp(value, 0, 100);
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
  auto line = std::make_unique<core::LineTool>();
  m_lineTool = line.get();
  m_toolManager.registerTool(std::move(line));
  auto rectSelection = std::make_unique<core::RectSelectionTool>();
  m_rectSelectionTool = rectSelection.get();
  m_toolManager.registerTool(std::move(rectSelection));
  auto fill = std::make_unique<core::FillTool>();
  m_fillTool = fill.get();
  m_toolManager.registerTool(std::move(fill));
  m_toolManager.registerTool(std::make_unique<core::MoveLayerTool>());

  for (const app::ui::ToolDescriptor& tool : m_toolCatalog.tools()) {
    if (!tool.subTools.empty()) {
      m_selectedSubToolByTool[tool.kind] = tool.subTools.front().id;
    }
  }

  m_uiState.toolKind = core::ToolKind::Brush;
  m_toolManager.setActiveTool(core::ToolKind::Brush);
  const app::ui::SubToolDescriptor* defaultSubTool = m_toolCatalog.defaultSubTool(core::ToolKind::Brush);
  if (defaultSubTool != nullptr) {
    resetToolStateFromDescriptor(*defaultSubTool);
  }

  setBrushColor(core::Color::OpaqueBlack());
  applyUiStateToTools();
  rerender();
}

CanvasOverlayViewModel AppController::canvasOverlay() const {
  CanvasOverlayViewModel view;
  view.toolOverlay = m_toolManager.overlay();
  view.selectionRect = m_document.selection().boundingRect();
  return view;
}

std::vector<LayerViewModel> AppController::layerViewModels() const {
  std::vector<LayerViewModel> models;
  models.reserve(m_document.layerCount());
  for (std::size_t i = 0; i < m_document.layerCount(); ++i) {
    const core::Layer& layer = m_document.layerAt(i);
    models.push_back(LayerViewModel {
        layer.name(),
        layer.visible(),
        i == m_document.activeLayerIndex(),
        static_cast<int>(std::lround(std::clamp(layer.opacity(), 0.0F, 1.0F) * 100.0F)),
        layer.kind(),
        layer.clippedToBelow(),
        layer.hasMask(),
        layer.maskEnabled(),
        layer.locked(),
        layer.alphaLocked(),
        layer.positionLocked()});
  }
  return models;
}

std::vector<SubToolViewModel> AppController::subToolViewModels() const {
  std::vector<SubToolViewModel> models;
  const app::ui::ToolDescriptor* descriptor = currentToolDescriptor();
  if (descriptor == nullptr) {
    return models;
  }

  const std::string selectedId = currentSubToolId();
  const core::Layer* active = m_document.activeLayer();
  const core::LayerKind activeKind = active == nullptr ? core::LayerKind::Raster : active->kind();
  models.reserve(descriptor->subTools.size());
  for (const app::ui::SubToolDescriptor& sub : descriptor->subTools) {
    const bool enabled = isSubToolCompatibleWithLayerKind(sub, activeKind);
    models.push_back(SubToolViewModel {
        sub.id,
        sub.displayName,
        sub.id == selectedId,
        enabled,
        enabled ? std::string {} : std::string {"Layer kind mismatch"}});
  }
  return models;
}

ToolStateViewModel AppController::toolState() const noexcept {
  return ToolStateViewModel {
      m_currentColor,
      m_uiState.size,
      m_uiState.size,
      m_uiState.opacity,
      m_uiState.hardness,
      m_uiState.flow,
      m_uiState.spacing,
      m_uiState.angle,
      m_uiState.roundness,
      m_uiState.taperStart,
      m_uiState.taperEnd,
      m_uiState.antiAlias,
      m_uiState.stabilization,
      m_uiState.snapAngle,
      m_uiState.simplifyLevel,
      m_uiState.fillThreshold,
      m_uiState.fillContiguous,
      m_uiState.fillReferAllLayers,
      m_uiState.fillGapClose,
      m_uiState.selectionMode,
      m_uiState.autoSelectThreshold,
      m_uiState.autoSelectContiguous,
      m_uiState.autoSelectReferAllLayers,
      m_uiState.postCorrection,
      m_uiState.velocityBasedCorrection,
      m_uiState.shapeType,
      m_uiState.blendMode,
      m_uiState.eraseMode,
      m_uiState.lockAlphaRespect};
}

void AppController::newDocument(int width, int height) {
  m_document = core::Document(width, height);
  m_layerCounter = 1;
  ensureCurrentSubToolCompatibility();
  m_stroking = false;
  m_pendingStroke.reset();
  clearStrokeHistory();
  rerender();
  emit toolStateChanged();
  emit layersChanged();
  emit documentChanged();
}

void AppController::addLayer() {
  addRasterLayer();
}

void AppController::addRasterLayer() {
  ++m_layerCounter;
  m_document.addRasterLayer("Layer " + std::to_string(m_layerCounter));
  ensureCurrentSubToolCompatibility();
  m_pendingStroke.reset();
  clearStrokeHistory();
  rerender();
  emit layersChanged();
  emit documentChanged();
}

void AppController::addVectorLayer() {
  ++m_layerCounter;
  m_document.addVectorLayer("Vector " + std::to_string(m_layerCounter));
  ensureCurrentSubToolCompatibility();
  m_pendingStroke.reset();
  clearStrokeHistory();
  rerender();
  emit layersChanged();
  emit documentChanged();
}

void AppController::addFolderLayer() {
  ++m_layerCounter;
  m_document.addFolderLayer("Folder " + std::to_string(m_layerCounter));
  ensureCurrentSubToolCompatibility();
  m_pendingStroke.reset();
  clearStrokeHistory();
  rerender();
  emit layersChanged();
  emit documentChanged();
}

bool AppController::duplicateLayer(std::size_t index) {
  if (index >= m_document.layerCount()) {
    return false;
  }
  m_document.duplicateLayer(index);
  ensureCurrentSubToolCompatibility();
  m_pendingStroke.reset();
  clearStrokeHistory();
  rerender();
  emit layersChanged();
  emit documentChanged();
  return true;
}

bool AppController::duplicateActiveLayer() {
  if (m_document.layerCount() == 0) {
    return false;
  }
  return duplicateLayer(m_document.activeLayerIndex());
}

bool AppController::mergeLayerDown(std::size_t index) {
  if (index == 0 || index >= m_document.layerCount()) {
    return false;
  }

  const core::Size size = m_document.canvasSize();
  core::Document temp(size.width, size.height);
  temp.layerAt(0) = m_document.layerAt(index - 1);
  temp.layerAt(0).setVisible(true);
  temp.addLayer("MergeTop", m_document.layerAt(index).kind());
  temp.layerAt(1) = m_document.layerAt(index);
  temp.layerAt(1).setVisible(true);

  core::PixelBuffer merged = m_renderer.composite(temp);
  core::Layer& dst = m_document.layerAt(index - 1);
  dst.setKind(core::LayerKind::Raster);
  dst.clearVectorPaths();
  dst.buffer() = std::move(merged);
  dst.setOpacity(1.0F);
  dst.setVisible(true);

  m_document.removeLayer(index);
  m_document.setActiveLayer(index - 1);
  ensureCurrentSubToolCompatibility();
  m_pendingStroke.reset();
  clearStrokeHistory();
  rerender();
  emit toolStateChanged();
  emit layersChanged();
  emit documentChanged();
  return true;
}

bool AppController::mergeActiveLayerDown() {
  if (m_document.layerCount() == 0) {
    return false;
  }
  return mergeLayerDown(m_document.activeLayerIndex());
}

bool AppController::rasterizeLayer(std::size_t index) {
  if (index >= m_document.layerCount()) {
    return false;
  }
  core::Layer& layer = m_document.layerAt(index);
  if (layer.kind() == core::LayerKind::Raster) {
    return false;
  }

  const core::Layer before = layer;
  const core::Size size = m_document.canvasSize();
  core::Document temp(size.width, size.height);
  temp.layerAt(0) = layer;
  temp.layerAt(0).setVisible(true);
  temp.layerAt(0).setOpacity(1.0F);
  const core::PixelBuffer raster = m_renderer.composite(temp);

  layer.setKind(core::LayerKind::Raster);
  layer.clearVectorPaths();
  layer.buffer() = raster;

  const core::Layer after = layer;
  if (!layersEqual(before, after)) {
    StrokeHistoryEntry entry;
    entry.kind = HistoryKind::Stroke;
    entry.actionName = "Rasterize";
    entry.layerIndex = index;
    entry.beforeLayer = before;
    entry.afterLayer = after;
    pushHistoryEntry(std::move(entry));
  }

  ensureCurrentSubToolCompatibility();
  rerender();
  emit toolStateChanged();
  emit layersChanged();
  emit documentChanged();
  return true;
}

bool AppController::rasterizeActiveLayer() {
  if (m_document.layerCount() == 0) {
    return false;
  }
  return rasterizeLayer(m_document.activeLayerIndex());
}

bool AppController::removeLayer(std::size_t index) {
  if (!m_document.removeLayer(index)) {
    return false;
  }
  ensureCurrentSubToolCompatibility();
  m_pendingStroke.reset();
  clearStrokeHistory();
  rerender();
  emit toolStateChanged();
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

bool AppController::moveLayer(std::size_t fromIndex, std::size_t toIndex) {
  if (!m_document.moveLayer(fromIndex, toIndex)) {
    return false;
  }

  StrokeHistoryEntry entry;
  entry.kind = HistoryKind::LayerOrder;
  entry.actionName = "Layer Order";
  entry.beforeIndex = fromIndex;
  entry.afterIndex = toIndex;
  pushHistoryEntry(std::move(entry));

  rerender();
  emit layersChanged();
  emit documentChanged();
  return true;
}

bool AppController::moveLayerUp(std::size_t index) {
  return moveLayer(index, index + 1);
}

bool AppController::moveLayerDown(std::size_t index) {
  if (index == 0) {
    return false;
  }
  return moveLayer(index, index - 1);
}

bool AppController::moveActiveLayerUp() {
  if (m_document.layerCount() == 0) {
    return false;
  }
  return moveLayerUp(m_document.activeLayerIndex());
}

bool AppController::moveActiveLayerDown() {
  if (m_document.layerCount() == 0) {
    return false;
  }
  return moveLayerDown(m_document.activeLayerIndex());
}

void AppController::setActiveLayer(std::size_t index) {
  if (!m_document.setActiveLayer(index)) {
    return;
  }
  ensureCurrentSubToolCompatibility();
  emit toolStateChanged();
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

  StrokeHistoryEntry entry;
  entry.kind = HistoryKind::LayerVisibility;
  entry.actionName = "Visibility";
  entry.layerIndex = index;
  entry.beforeVisible = beforeVisible;
  entry.afterVisible = visible;
  pushHistoryEntry(std::move(entry));
  rerender();
  emit layersChanged();
  emit documentChanged();
}

void AppController::setLayerOpacity(std::size_t index, int opacityPercent) {
  if (index >= m_document.layerCount()) {
    return;
  }
  const float normalized = static_cast<float>(clampPercent(opacityPercent)) / 100.0F;
  core::Layer& layer = m_document.layerAt(index);
  if (std::abs(layer.opacity() - normalized) < 0.0001F) {
    return;
  }
  layer.setOpacity(normalized);
  rerender();
  emit layersChanged();
  emit documentChanged();
}

void AppController::setActiveLayerOpacity(int opacityPercent) {
  if (m_document.layerCount() == 0) {
    return;
  }
  setLayerOpacity(m_document.activeLayerIndex(), opacityPercent);
}

int AppController::activeLayerOpacity() const noexcept {
  if (m_document.layerCount() == 0) {
    return 100;
  }
  const float opacity = m_document.layerAt(m_document.activeLayerIndex()).opacity();
  return static_cast<int>(std::lround(std::clamp(opacity, 0.0F, 1.0F) * 100.0F));
}

bool AppController::toggleActiveLayerVisible() {
  if (m_document.layerCount() == 0) {
    return false;
  }
  const std::size_t active = m_document.activeLayerIndex();
  const bool nextVisible = !m_document.layerAt(active).visible();
  setLayerVisible(active, nextVisible);
  return true;
}

bool AppController::toggleActiveLayerClipToBelow() {
  if (m_document.layerCount() == 0) {
    return false;
  }
  const std::size_t activeIndex = m_document.activeLayerIndex();
  core::Layer& active = m_document.layerAt(activeIndex);
  if (active.kind() == core::LayerKind::Folder) {
    return false;
  }
  const core::Layer before = active;
  active.setClippedToBelow(!active.clippedToBelow());
  const core::Layer after = active;
  if (layersEqual(before, after)) {
    return false;
  }
  StrokeHistoryEntry entry;
  entry.kind = HistoryKind::Stroke;
  entry.actionName = "Clipping";
  entry.layerIndex = activeIndex;
  entry.beforeLayer = before;
  entry.afterLayer = after;
  pushHistoryEntry(std::move(entry));
  rerender();
  emit layersChanged();
  emit documentChanged();
  return true;
}

bool AppController::toggleActiveLayerMask() {
  if (m_document.layerCount() == 0) {
    return false;
  }
  const std::size_t activeIndex = m_document.activeLayerIndex();
  core::Layer& active = m_document.layerAt(activeIndex);
  if (active.kind() == core::LayerKind::Folder) {
    return false;
  }
  const core::Layer before = active;
  if (!active.hasMask()) {
    active.createMask(core::Color::OpaqueWhite());
  } else {
    active.setMaskEnabled(!active.maskEnabled());
  }
  const core::Layer after = active;
  if (layersEqual(before, after)) {
    return false;
  }
  StrokeHistoryEntry entry;
  entry.kind = HistoryKind::Stroke;
  entry.actionName = "Mask";
  entry.layerIndex = activeIndex;
  entry.beforeLayer = before;
  entry.afterLayer = after;
  pushHistoryEntry(std::move(entry));
  rerender();
  emit layersChanged();
  emit documentChanged();
  return true;
}

bool AppController::removeActiveLayerMask() {
  if (m_document.layerCount() == 0) {
    return false;
  }
  const std::size_t activeIndex = m_document.activeLayerIndex();
  core::Layer& active = m_document.layerAt(activeIndex);
  if (!active.hasMask()) {
    return false;
  }
  const core::Layer before = active;
  active.removeMask();
  const core::Layer after = active;
  if (layersEqual(before, after)) {
    return false;
  }
  StrokeHistoryEntry entry;
  entry.kind = HistoryKind::Stroke;
  entry.actionName = "Mask Remove";
  entry.layerIndex = activeIndex;
  entry.beforeLayer = before;
  entry.afterLayer = after;
  pushHistoryEntry(std::move(entry));
  rerender();
  emit layersChanged();
  emit documentChanged();
  return true;
}

bool AppController::toggleActiveLayerLock() {
  if (m_document.layerCount() == 0) {
    return false;
  }
  const std::size_t activeIndex = m_document.activeLayerIndex();
  core::Layer& active = m_document.layerAt(activeIndex);
  const core::Layer before = active;
  active.setLocked(!active.locked());
  const core::Layer after = active;
  if (layersEqual(before, after)) {
    return false;
  }
  StrokeHistoryEntry entry;
  entry.kind = HistoryKind::Stroke;
  entry.actionName = "Layer Lock";
  entry.layerIndex = activeIndex;
  entry.beforeLayer = before;
  entry.afterLayer = after;
  pushHistoryEntry(std::move(entry));
  rerender();
  emit layersChanged();
  emit documentChanged();
  return true;
}

bool AppController::toggleActiveLayerAlphaLock() {
  if (m_document.layerCount() == 0) {
    return false;
  }
  const std::size_t activeIndex = m_document.activeLayerIndex();
  core::Layer& active = m_document.layerAt(activeIndex);
  if (active.kind() != core::LayerKind::Raster) {
    return false;
  }
  const core::Layer before = active;
  active.setAlphaLocked(!active.alphaLocked());
  const core::Layer after = active;
  if (layersEqual(before, after)) {
    return false;
  }
  StrokeHistoryEntry entry;
  entry.kind = HistoryKind::Stroke;
  entry.actionName = "Alpha Lock";
  entry.layerIndex = activeIndex;
  entry.beforeLayer = before;
  entry.afterLayer = after;
  pushHistoryEntry(std::move(entry));
  rerender();
  emit layersChanged();
  emit documentChanged();
  return true;
}

bool AppController::toggleActiveLayerPositionLock() {
  if (m_document.layerCount() == 0) {
    return false;
  }
  const std::size_t activeIndex = m_document.activeLayerIndex();
  core::Layer& active = m_document.layerAt(activeIndex);
  if (active.kind() == core::LayerKind::Folder) {
    return false;
  }
  const core::Layer before = active;
  active.setPositionLocked(!active.positionLocked());
  const core::Layer after = active;
  if (layersEqual(before, after)) {
    return false;
  }
  StrokeHistoryEntry entry;
  entry.kind = HistoryKind::Stroke;
  entry.actionName = "Position Lock";
  entry.layerIndex = activeIndex;
  entry.beforeLayer = before;
  entry.afterLayer = after;
  pushHistoryEntry(std::move(entry));
  rerender();
  emit layersChanged();
  emit documentChanged();
  return true;
}

bool AppController::clearSelection() {
  const core::SelectionMask before = m_document.selection();
  if (!before.hasSelection()) {
    return false;
  }
  m_document.clearSelection();
  pushSelectionHistoryIfChanged(before, "Selection");
  emit documentChanged();
  return true;
}

bool AppController::selectAll() {
  const core::SelectionMask before = m_document.selection();
  const core::Size size = m_document.canvasSize();
  const bool changed = m_document.selection().setRect(core::Rect {0, 0, size.width, size.height});
  if (!changed) {
    return false;
  }
  pushSelectionHistoryIfChanged(before, "Selection");
  emit documentChanged();
  return true;
}

bool AppController::deselect() {
  return clearSelection();
}

bool AppController::invertSelection() {
  const core::SelectionMask before = m_document.selection();
  if (!m_document.selection().invert()) {
    return false;
  }
  pushSelectionHistoryIfChanged(before, "Selection");
  emit documentChanged();
  return true;
}

bool AppController::fillSelectionOrCanvas() {
  core::Layer* active = m_document.activeLayer();
  if (active == nullptr || active->kind() != core::LayerKind::Raster) {
    return false;
  }

  const core::Layer before = *active;
  const core::SelectionMask& selection = m_document.selection();
  if (selection.hasSelection()) {
    for (int y = 0; y < active->buffer().height(); ++y) {
      for (int x = 0; x < active->buffer().width(); ++x) {
        if (selection.contains(x, y)) {
          active->buffer().setPixel(x, y, m_currentColor);
        }
      }
    }
  } else {
    active->buffer().fill(m_currentColor);
  }

  const core::Layer after = *active;
  if (layersEqual(before, after)) {
    return false;
  }

  StrokeHistoryEntry entry;
  entry.kind = HistoryKind::Stroke;
  entry.actionName = "Fill";
  entry.layerIndex = m_document.activeLayerIndex();
  entry.beforeLayer = before;
  entry.afterLayer = after;
  pushHistoryEntry(std::move(entry));
  rerender();
  emit documentChanged();
  return true;
}

bool AppController::deleteSelectionPixels() {
  core::Layer* active = m_document.activeLayer();
  if (active == nullptr || active->kind() != core::LayerKind::Raster) {
    return false;
  }

  const core::Layer before = *active;
  const core::SelectionMask& selection = m_document.selection();
  if (selection.hasSelection()) {
    for (int y = 0; y < active->buffer().height(); ++y) {
      for (int x = 0; x < active->buffer().width(); ++x) {
        if (selection.contains(x, y)) {
          active->buffer().setPixel(x, y, core::Color::Transparent());
        }
      }
    }
  } else {
    active->buffer().fill(core::Color::Transparent());
  }

  const core::Layer after = *active;
  if (layersEqual(before, after)) {
    return false;
  }

  StrokeHistoryEntry entry;
  entry.kind = HistoryKind::Stroke;
  entry.actionName = "Delete";
  entry.layerIndex = m_document.activeLayerIndex();
  entry.beforeLayer = before;
  entry.afterLayer = after;
  pushHistoryEntry(std::move(entry));
  rerender();
  emit documentChanged();
  return true;
}

core::PixelBuffer AppController::exportSelectionOrCanvasFromComposite() const {
  const core::SelectionMask& selection = m_document.selection();
  if (!selection.hasSelection()) {
    return m_composited;
  }

  const std::optional<core::Rect> bounds = selection.boundingRect();
  if (!bounds.has_value()) {
    return m_composited;
  }
  const core::Rect rect = *bounds;
  core::PixelBuffer cropped(rect.width, rect.height, core::Color::Transparent());
  for (int y = 0; y < rect.height; ++y) {
    for (int x = 0; x < rect.width; ++x) {
      const int sx = rect.x + x;
      const int sy = rect.y + y;
      if (!m_composited.inBounds(sx, sy) || !selection.contains(sx, sy)) {
        continue;
      }
      cropped.setPixel(x, y, m_composited.pixel(sx, sy));
    }
  }
  return cropped;
}

void AppController::importFlattenedBuffer(const core::PixelBuffer& buffer, const std::string& layerName) {
  if (buffer.width() <= 0 || buffer.height() <= 0) {
    return;
  }
  m_document = core::Document(buffer.width(), buffer.height());
  core::Layer& base = m_document.layerAt(0);
  base.buffer() = buffer;
  base.setKind(core::LayerKind::Raster);
  base.clearVectorPaths();
  if (!layerName.empty()) {
    base.setName(layerName);
  }
  m_layerCounter = 1;
  ensureCurrentSubToolCompatibility();
  m_stroking = false;
  m_pendingStroke.reset();
  clearStrokeHistory();
  rerender();
  emit toolStateChanged();
  emit layersChanged();
  emit documentChanged();
}

bool AppController::pasteBufferAsNewRasterLayer(const core::PixelBuffer& buffer, const std::string& layerName) {
  if (buffer.width() <= 0 || buffer.height() <= 0) {
    return false;
  }
  ++m_layerCounter;
  const std::string finalName = layerName.empty() ? ("Layer " + std::to_string(m_layerCounter)) : layerName;
  const std::size_t index = m_document.addRasterLayer(finalName);
  core::Layer& layer = m_document.layerAt(index);
  layer.buffer().fill(core::Color::Transparent());
  for (int y = 0; y < buffer.height(); ++y) {
    for (int x = 0; x < buffer.width(); ++x) {
      if (!layer.buffer().inBounds(x, y)) {
        continue;
      }
      layer.buffer().setPixel(x, y, buffer.pixel(x, y));
    }
  }
  m_document.setActiveLayer(index);
  ensureCurrentSubToolCompatibility();
  m_pendingStroke.reset();
  clearStrokeHistory();
  rerender();
  emit toolStateChanged();
  emit layersChanged();
  emit documentChanged();
  return true;
}

std::vector<core::ToolKind> AppController::availableTools() const {
  std::vector<core::ToolKind> tools;
  tools.reserve(m_toolCatalog.tools().size());
  for (const app::ui::ToolDescriptor& descriptor : m_toolCatalog.tools()) {
    tools.push_back(descriptor.kind);
  }
  return tools;
}

std::string AppController::toolDisplayName(core::ToolKind kind) const {
  const app::ui::ToolDescriptor* descriptor = m_toolCatalog.findTool(kind);
  if (descriptor != nullptr && !descriptor->displayName.empty()) {
    return descriptor->displayName;
  }
  return std::string {core::toolKindDisplayName(kind)};
}

bool AppController::canUseToolOnActiveLayer(core::ToolKind kind) const {
  const core::Layer* active = m_document.activeLayer();
  if (active == nullptr) {
    return true;
  }
  if (active->kind() == core::LayerKind::Folder) {
    return kind == core::ToolKind::Hand || kind == core::ToolKind::Zoom;
  }
  return firstCompatibleSubTool(kind, active->kind()) != nullptr;
}

bool AppController::setCurrentTool(core::ToolKind kind) {
  if (!m_toolManager.setActiveTool(kind)) {
    return false;
  }

  m_uiState.toolKind = kind;
  if (m_selectedSubToolByTool.find(kind) == m_selectedSubToolByTool.end()) {
    const app::ui::SubToolDescriptor* defaultSub = m_toolCatalog.defaultSubTool(kind);
    if (defaultSub != nullptr) {
      m_selectedSubToolByTool[kind] = defaultSub->id;
    }
  }

  ensureCurrentSubToolCompatibility();
  selectSubToolInternal(currentSubToolId(), false);
  emit toolStateChanged();
  emit documentChanged();
  return true;
}

core::ToolKind AppController::currentTool() const noexcept {
  return m_toolManager.activeToolKind();
}

bool AppController::setCurrentSubTool(const std::string& subToolId) {
  if (!selectSubToolInternal(subToolId, true)) {
    return false;
  }
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    ensureCurrentSubToolCompatibility();
    emit toolStateChanged();
  }
  emit documentChanged();
  return true;
}

std::string AppController::currentSubToolId() const {
  const auto it = m_selectedSubToolByTool.find(currentTool());
  if (it != m_selectedSubToolByTool.end()) {
    return it->second;
  }
  const app::ui::SubToolDescriptor* sub = m_toolCatalog.defaultSubTool(currentTool());
  return sub == nullptr ? std::string {} : sub->id;
}

bool AppController::duplicateCurrentSubTool() {
  const std::string sourceId = currentSubToolId();
  const app::ui::SubToolDescriptor* source = currentSubToolDescriptor();
  if (source == nullptr) {
    return false;
  }
  if (!m_toolCatalog.duplicateSubTool(currentTool(), sourceId, source->displayName + " Copy")) {
    return false;
  }
  const app::ui::ToolDescriptor* tool = currentToolDescriptor();
  if (tool == nullptr || tool->subTools.empty()) {
    return false;
  }
  const app::ui::SubToolDescriptor& created = tool->subTools.back();
  return setCurrentSubTool(created.id);
}

bool AppController::renameCurrentSubTool(const std::string& displayName) {
  if (!m_toolCatalog.renameSubTool(currentTool(), currentSubToolId(), displayName)) {
    return false;
  }
  emit toolStateChanged();
  return true;
}

bool AppController::deleteCurrentSubTool() {
  const std::string deletingId = currentSubToolId();
  if (!m_toolCatalog.removeSubTool(currentTool(), deletingId)) {
    return false;
  }
  const app::ui::SubToolDescriptor* fallback = m_toolCatalog.defaultSubTool(currentTool());
  if (fallback != nullptr) {
    m_selectedSubToolByTool[currentTool()] = fallback->id;
    selectSubToolInternal(fallback->id, true);
  }
  emit documentChanged();
  return true;
}

bool AppController::resetCurrentSubTool() {
  const std::string id = currentSubToolId();
  if (!m_toolCatalog.resetSubTool(currentTool(), id)) {
    return false;
  }
  selectSubToolInternal(id, true);
  emit documentChanged();
  return true;
}

std::string AppController::currentToolDisplayName() const {
  const app::ui::ToolDescriptor* descriptor = currentToolDescriptor();
  if (descriptor != nullptr) {
    return descriptor->displayName;
  }
  return std::string {core::toolKindDisplayName(currentTool())};
}

std::string AppController::currentSubToolDisplayName() const {
  const app::ui::SubToolDescriptor* sub = currentSubToolDescriptor();
  return sub == nullptr ? std::string {"-"} : sub->displayName;
}

std::string AppController::currentToolGuide() const {
  std::string guide;
  switch (currentTool()) {
    case core::ToolKind::Brush:
      guide = u8"\u5DE6\u30C9\u30E9\u30C3\u30B0\u3067\u63CF\u753B\u3002\u30DB\u30A4\u30FC\u30EB/[]\u3067\u30B5\u30A4\u30BA\u5909\u66F4\u3002";
      break;
    case core::ToolKind::Eraser:
      guide = u8"\u5DE6\u30C9\u30E9\u30C3\u30B0\u3067\u6D88\u53BB\u3002\u30DB\u30A4\u30FC\u30EB/[]\u3067\u30B5\u30A4\u30BA\u5909\u66F4\u3002";
      break;
    case core::ToolKind::Eyedropper:
      guide = u8"\u30AF\u30EA\u30C3\u30AF\u3057\u3066\u8272\u3092\u53D6\u5F97\u3002";
      break;
    case core::ToolKind::Fill:
      guide = u8"\u30AF\u30EA\u30C3\u30AF\u3057\u3066\u5857\u308A\u3064\u3076\u3057\u3002";
      break;
    case core::ToolKind::Line:
      guide = u8"\u30C9\u30E9\u30C3\u30B0\u3057\u3066\u76F4\u7DDA\u3092\u63CF\u753B\u3002";
      break;
    case core::ToolKind::RectSelection:
      guide = u8"\u30C9\u30E9\u30C3\u30B0\u3057\u3066\u9078\u629E\u7BC4\u56F2\u3092\u4F5C\u6210\u3002";
      break;
    case core::ToolKind::MoveLayer:
      guide = u8"\u30C9\u30E9\u30C3\u30B0\u3057\u3066\u30EC\u30A4\u30E4\u30FC\u5185\u5BB9\u3092\u79FB\u52D5\u3002";
      break;
    case core::ToolKind::Hand:
      guide = u8"\u30C9\u30E9\u30C3\u30B0\u3057\u3066\u30AD\u30E3\u30F3\u30D0\u30B9\u3092\u79FB\u52D5\u3002";
      break;
    case core::ToolKind::Zoom:
      guide = u8"Ctrl+\u30DB\u30A4\u30FC\u30EB\u3067\u30BA\u30FC\u30E0\u3002";
      break;
    default:
      break;
  }
  const std::string hint = currentLayerCompatibilityHint();
  return hint.empty() ? guide : (guide.empty() ? hint : guide + "  " + hint);
}

std::string AppController::activeLayerKindDisplayName() const {
  const core::Layer* layer = m_document.activeLayer();
  if (layer == nullptr) {
    return u8"\u306A\u3057";
  }
  switch (layer->kind()) {
    case core::LayerKind::Raster:
      return u8"\u30E9\u30B9\u30BF";
    case core::LayerKind::Vector:
      return u8"\u30D9\u30AF\u30BF\u30FC";
    case core::LayerKind::Folder:
      return u8"\u30D5\u30A9\u30EB\u30C0";
    default:
      return u8"\u4E0D\u660E";
  }
}

bool AppController::canUseCurrentToolOnActiveLayer() const {
  return isCurrentSubToolCompatibleWithActiveLayer();
}

std::string AppController::currentLayerCompatibilityHint() const {
  if (isCurrentSubToolCompatibleWithActiveLayer()) {
    return {};
  }
  return u8"\uFF08\u73FE\u5728\u306E\u30EC\u30A4\u30E4\u30FC\u7A2E\u5225\u3067\u306F\u4E00\u90E8\u64CD\u4F5C\u304C\u7121\u52B9\u3067\u3059\uFF09";
}

bool AppController::currentToolSupportsColor() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return !m_uiState.eraseMode &&
         containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::Color);
}

bool AppController::currentToolSupportsSize() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::Size) ||
         containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::StrokeWidth);
}

bool AppController::currentToolSupportsOpacity() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::Opacity);
}

bool AppController::currentToolSupportsHardness() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::Hardness);
}

bool AppController::currentToolSupportsFlow() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::Flow);
}

bool AppController::currentToolSupportsSpacing() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::Spacing);
}

bool AppController::currentToolSupportsAntiAlias() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::AntiAlias);
}

bool AppController::currentToolSupportsStabilization() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::Stabilization);
}

bool AppController::currentToolSupportsPostCorrection() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::PostCorrection);
}

bool AppController::currentToolSupportsVelocityCorrection() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::VelocityCorrection);
}

bool AppController::currentToolSupportsShapeType() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::ShapeType);
}

bool AppController::currentToolSupportsAngle() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::Angle);
}

bool AppController::currentToolSupportsRoundness() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::Roundness);
}

bool AppController::currentToolSupportsTaperStart() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::TaperStart);
}

bool AppController::currentToolSupportsTaperEnd() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::TaperEnd);
}

bool AppController::currentToolSupportsBlendMode() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return !m_uiState.eraseMode &&
         containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::BlendMode);
}

bool AppController::currentToolSupportsEraseMode() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::EraseMode);
}

bool AppController::currentToolSupportsLockAlphaRespect() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return !m_uiState.eraseMode &&
         containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::LockAlphaRespect);
}

bool AppController::currentToolSupportsSnapAngle() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::SnapAngle);
}

bool AppController::currentToolSupportsSimplifyLevel() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::SimplifyLevel);
}

bool AppController::currentToolSupportsFillThreshold() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::FillThreshold);
}

bool AppController::currentToolSupportsFillContiguous() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::FillContiguous);
}

bool AppController::currentToolSupportsFillReferAllLayers() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(
      currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::FillReferAllLayers);
}

bool AppController::currentToolSupportsFillGapClose() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::FillGapClose);
}

bool AppController::currentToolSupportsSelectionMode() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::SelectionMode);
}

bool AppController::currentToolSupportsAutoSelectThreshold() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(
      currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::AutoSelectThreshold);
}

bool AppController::currentToolSupportsAutoSelectContiguous() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(
      currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::AutoSelectContiguous);
}

bool AppController::currentToolSupportsAutoSelectReferAllLayers() const noexcept {
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return false;
  }
  return containsProperty(
      currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::AutoSelectReferAllLayers);
}

void AppController::beginStroke(int x, int y) {
  if (m_stroking) {
    return;
  }
  if (!isCurrentSubToolCompatibleWithActiveLayer()) {
    return;
  }

  const core::ToolKind activeKind = m_toolManager.activeToolKind();
  PendingStrokeState pending;
  pending.actionName = actionNameForTool(activeKind);
  if (toolWritesPixels(activeKind)) {
    core::Layer* activeLayer = m_document.activeLayer();
    if (activeLayer == nullptr) {
      return;
    }
    pending.trackPixels = true;
    pending.layerIndex = m_document.activeLayerIndex();
    pending.beforeLayer = *activeLayer;
  }
  if (toolWritesSelection(activeKind)) {
    pending.trackSelection = true;
    pending.beforeSelection = m_document.selection();
  }
  if (pending.trackPixels || pending.trackSelection) {
    m_pendingStroke = std::move(pending);
  } else {
    m_pendingStroke.reset();
  }

  m_stroking = true;
  m_lastPointer = core::Point {x, y};
  core::ToolPointerEvent pressEvent;
  pressEvent.point = m_lastPointer;
  core::ToolContext context = makeToolContext();
  const core::ToolResult result = m_toolManager.pointerPress(context, pressEvent);
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
  const core::ToolResult result = m_toolManager.pointerMove(context, moveEvent);
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
  const core::ToolResult result = m_toolManager.pointerRelease(context, releaseEvent);
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
  const core::ToolResult result = m_toolManager.pointerPress(context, event);
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

  if ((entry.kind == HistoryKind::Stroke || entry.kind == HistoryKind::LayerVisibility) &&
      entry.layerIndex >= m_document.layerCount()) {
    clearStrokeHistory();
    return false;
  }
  if (entry.kind == HistoryKind::LayerOrder &&
      (entry.beforeIndex >= m_document.layerCount() || entry.afterIndex >= m_document.layerCount())) {
    clearStrokeHistory();
    return false;
  }
  if (entry.kind == HistoryKind::LayerOrder &&
      (entry.beforeIndex >= m_document.layerCount() || entry.afterIndex >= m_document.layerCount())) {
    clearStrokeHistory();
    return false;
  }

  switch (entry.kind) {
    case HistoryKind::Stroke:
      if (!entry.beforeLayer.has_value()) {
        clearStrokeHistory();
        return false;
      }
      m_document.layerAt(entry.layerIndex) = *entry.beforeLayer;
      break;
    case HistoryKind::LayerVisibility:
      m_document.setLayerVisible(entry.layerIndex, entry.beforeVisible);
      break;
    case HistoryKind::LayerOrder:
      m_document.moveLayer(entry.afterIndex, entry.beforeIndex);
      break;
    case HistoryKind::Selection:
      m_document.selection() = entry.beforeSelection;
      break;
    default:
      break;
  }

  if (m_redoHistory.size() >= m_maxStrokeHistory) {
    m_redoHistory.erase(m_redoHistory.begin());
  }
  m_redoHistory.push_back(std::move(entry));
  rerender();
  if (m_redoHistory.back().kind == HistoryKind::LayerVisibility ||
      m_redoHistory.back().kind == HistoryKind::LayerOrder) {
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

  if ((entry.kind == HistoryKind::Stroke || entry.kind == HistoryKind::LayerVisibility) &&
      entry.layerIndex >= m_document.layerCount()) {
    clearStrokeHistory();
    return false;
  }

  switch (entry.kind) {
    case HistoryKind::Stroke:
      if (!entry.afterLayer.has_value()) {
        clearStrokeHistory();
        return false;
      }
      m_document.layerAt(entry.layerIndex) = *entry.afterLayer;
      break;
    case HistoryKind::LayerVisibility:
      m_document.setLayerVisible(entry.layerIndex, entry.afterVisible);
      break;
    case HistoryKind::LayerOrder:
      m_document.moveLayer(entry.beforeIndex, entry.afterIndex);
      break;
    case HistoryKind::Selection:
      m_document.selection() = entry.afterSelection;
      break;
    default:
      break;
  }

  if (m_undoHistory.size() >= m_maxStrokeHistory) {
    m_undoHistory.erase(m_undoHistory.begin());
  }
  m_undoHistory.push_back(std::move(entry));
  rerender();
  if (m_undoHistory.back().kind == HistoryKind::LayerVisibility ||
      m_undoHistory.back().kind == HistoryKind::LayerOrder) {
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
  return m_undoHistory.back().actionName;
}

std::string AppController::nextRedoActionName() const {
  if (m_redoHistory.empty()) {
    return {};
  }
  return m_redoHistory.back().actionName;
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
  const int normalized = std::max(1, size);
  if (m_uiState.size == normalized) {
    return;
  }

  m_uiState.size = normalized;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::adjustBrushSize(int delta) {
  setBrushSize(m_uiState.size + delta);
}

void AppController::setBrushOpacity(int opacity) {
  const int normalized = clampPercent(opacity);
  if (m_uiState.opacity == normalized) {
    return;
  }

  m_uiState.opacity = normalized;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setBrushHardness(int hardness) {
  const int normalized = clampPercent(hardness);
  if (m_uiState.hardness == normalized) {
    return;
  }

  m_uiState.hardness = normalized;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setBrushFlow(int flow) {
  const int normalized = clampPercent(flow);
  if (m_uiState.flow == normalized) {
    return;
  }

  m_uiState.flow = normalized;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setBrushSpacing(int spacing) {
  const int normalized = std::clamp(spacing, 1, 300);
  if (m_uiState.spacing == normalized) {
    return;
  }

  m_uiState.spacing = normalized;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setBrushAntiAlias(bool antiAlias) {
  if (m_uiState.antiAlias == antiAlias) {
    return;
  }
  m_uiState.antiAlias = antiAlias;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setBrushStabilization(int stabilization) {
  const int normalized = clampPercent(stabilization);
  if (m_uiState.stabilization == normalized) {
    return;
  }
  m_uiState.stabilization = normalized;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setBrushPostCorrection(bool enabled) {
  if (m_uiState.postCorrection == enabled) {
    return;
  }
  m_uiState.postCorrection = enabled;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setBrushVelocityBasedCorrection(bool enabled) {
  if (m_uiState.velocityBasedCorrection == enabled) {
    return;
  }
  m_uiState.velocityBasedCorrection = enabled;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setBrushShapeType(core::BrushShapeType shapeType) {
  if (m_uiState.shapeType == shapeType) {
    return;
  }
  m_uiState.shapeType = shapeType;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setBrushAngle(int angle) {
  const int normalized = std::clamp(angle, -180, 180);
  if (m_uiState.angle == normalized) {
    return;
  }
  m_uiState.angle = normalized;
  emit toolStateChanged();
}

void AppController::setBrushRoundness(int roundness) {
  const int normalized = clampPercent(roundness);
  if (m_uiState.roundness == normalized) {
    return;
  }
  m_uiState.roundness = normalized;
  emit toolStateChanged();
}

void AppController::setBrushTaperStart(int taperStart) {
  const int normalized = clampPercent(taperStart);
  if (m_uiState.taperStart == normalized) {
    return;
  }
  m_uiState.taperStart = normalized;
  emit toolStateChanged();
}

void AppController::setBrushTaperEnd(int taperEnd) {
  const int normalized = clampPercent(taperEnd);
  if (m_uiState.taperEnd == normalized) {
    return;
  }
  m_uiState.taperEnd = normalized;
  emit toolStateChanged();
}

void AppController::setBrushBlendMode(core::BlendMode blendMode) {
  if (m_uiState.blendMode == blendMode) {
    return;
  }
  m_uiState.blendMode = blendMode;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setBrushEraseMode(bool eraseMode) {
  if (m_uiState.eraseMode == eraseMode) {
    return;
  }
  m_uiState.eraseMode = eraseMode;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setBrushLockAlphaRespect(bool enabled) {
  if (m_uiState.lockAlphaRespect == enabled) {
    return;
  }
  m_uiState.lockAlphaRespect = enabled;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setLineSnapAngle(int snapAngle) {
  const int normalized = std::clamp(snapAngle, 0, 180);
  if (m_uiState.snapAngle == normalized) {
    return;
  }
  m_uiState.snapAngle = normalized;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setLineSimplifyLevel(int simplifyLevel) {
  const int normalized = clampPercent(simplifyLevel);
  if (m_uiState.simplifyLevel == normalized) {
    return;
  }
  m_uiState.simplifyLevel = normalized;
  emit toolStateChanged();
}

void AppController::setFillThreshold(int threshold) {
  const int normalized = std::clamp(threshold, 0, 255);
  if (m_uiState.fillThreshold == normalized) {
    return;
  }
  m_uiState.fillThreshold = normalized;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setFillContiguous(bool contiguous) {
  if (m_uiState.fillContiguous == contiguous) {
    return;
  }
  m_uiState.fillContiguous = contiguous;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setFillReferAllLayers(bool enabled) {
  if (m_uiState.fillReferAllLayers == enabled) {
    return;
  }
  m_uiState.fillReferAllLayers = enabled;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setFillGapClose(int gapClose) {
  const int normalized = std::clamp(gapClose, 0, 8);
  if (m_uiState.fillGapClose == normalized) {
    return;
  }
  m_uiState.fillGapClose = normalized;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setSelectionMode(app::ui::SelectionMode mode) {
  if (m_uiState.selectionMode == mode) {
    return;
  }
  m_uiState.selectionMode = mode;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setAutoSelectThreshold(int threshold) {
  const int normalized = std::clamp(threshold, 0, 255);
  if (m_uiState.autoSelectThreshold == normalized) {
    return;
  }
  m_uiState.autoSelectThreshold = normalized;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setAutoSelectContiguous(bool contiguous) {
  if (m_uiState.autoSelectContiguous == contiguous) {
    return;
  }
  m_uiState.autoSelectContiguous = contiguous;
  applyUiStateToTools();
  emit toolStateChanged();
}

void AppController::setAutoSelectReferAllLayers(bool enabled) {
  if (m_uiState.autoSelectReferAllLayers == enabled) {
    return;
  }
  m_uiState.autoSelectReferAllLayers = enabled;
  applyUiStateToTools();
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

bool AppController::toolWritesSelection(core::ToolKind kind) noexcept {
  return kind == core::ToolKind::RectSelection;
}

std::string AppController::actionNameForTool(core::ToolKind kind) {
  switch (kind) {
    case core::ToolKind::Brush:
      return u8"\u63CF\u753B";
    case core::ToolKind::Eraser:
      return u8"\u6D88\u53BB";
    case core::ToolKind::Line:
      return u8"\u76F4\u7DDA";
    case core::ToolKind::Fill:
      return u8"\u5857\u308A\u3064\u3076\u3057";
    case core::ToolKind::MoveLayer:
      return u8"\u30EC\u30A4\u30E4\u30FC\u79FB\u52D5";
    case core::ToolKind::RectSelection:
      return u8"\u9078\u629E";
    case core::ToolKind::Eyedropper:
      return u8"\u8272\u53D6\u5F97";
    case core::ToolKind::Hand:
      return u8"\u624B\u306E\u3072\u3089";
    case core::ToolKind::Zoom:
      return u8"\u30BA\u30FC\u30E0";
    default:
      return u8"\u64CD\u4F5C";
  }
}

bool AppController::isSubToolCompatibleWithLayerKind(
    const app::ui::SubToolDescriptor& subTool,
    core::LayerKind layerKind) const noexcept {
  if (layerKind == core::LayerKind::Folder) {
    return false;
  }
  const app::ui::TargetLayerKind target = subTool.profile.targetLayerKind;
  if (target == app::ui::TargetLayerKind::Both) {
    return true;
  }
  if (target == app::ui::TargetLayerKind::Raster) {
    return layerKind == core::LayerKind::Raster;
  }
  return layerKind == core::LayerKind::Vector;
}

bool AppController::isCurrentSubToolCompatibleWithActiveLayer() const noexcept {
  const app::ui::SubToolDescriptor* sub = currentSubToolDescriptor();
  const core::Layer* layer = m_document.activeLayer();
  if (sub == nullptr || layer == nullptr) {
    return true;
  }
  return isSubToolCompatibleWithLayerKind(*sub, layer->kind());
}

const app::ui::SubToolDescriptor* AppController::firstCompatibleSubTool(
    core::ToolKind kind,
    core::LayerKind layerKind) const noexcept {
  const app::ui::ToolDescriptor* descriptor = m_toolCatalog.findTool(kind);
  if (descriptor == nullptr) {
    return nullptr;
  }
  for (const app::ui::SubToolDescriptor& sub : descriptor->subTools) {
    if (isSubToolCompatibleWithLayerKind(sub, layerKind)) {
      return &sub;
    }
  }
  return nullptr;
}

void AppController::ensureCurrentSubToolCompatibility() {
  const core::Layer* active = m_document.activeLayer();
  if (active == nullptr) {
    return;
  }
  const std::string currentId = currentSubToolId();
  const app::ui::SubToolDescriptor* current = m_toolCatalog.findSubTool(currentTool(), currentId);
  if (current != nullptr && isSubToolCompatibleWithLayerKind(*current, active->kind())) {
    return;
  }
  const app::ui::SubToolDescriptor* compatible = firstCompatibleSubTool(currentTool(), active->kind());
  if (compatible != nullptr) {
    m_selectedSubToolByTool[currentTool()] = compatible->id;
  }
}

const app::ui::ToolDescriptor* AppController::currentToolDescriptor() const noexcept {
  return m_toolCatalog.findTool(currentTool());
}

const app::ui::SubToolDescriptor* AppController::currentSubToolDescriptor() const noexcept {
  return m_toolCatalog.findSubTool(currentTool(), currentSubToolId());
}

bool AppController::selectSubToolInternal(std::string_view subToolId, bool emitSignal) {
  const app::ui::SubToolDescriptor* sub = m_toolCatalog.findSubTool(currentTool(), subToolId);
  if (sub == nullptr) {
    return false;
  }

  m_selectedSubToolByTool[currentTool()] = sub->id;
  resetToolStateFromDescriptor(*sub);
  applyUiStateToTools();

  if (emitSignal) {
    emit toolStateChanged();
  }
  return true;
}

void AppController::applyUiStateToTools() {
  const float opacity = static_cast<float>(m_uiState.opacity) / 100.0F;
  const float hardness = static_cast<float>(m_uiState.hardness) / 100.0F;
  const float flow = static_cast<float>(m_uiState.flow) / 100.0F;
  const float spacing = static_cast<float>(std::max(1, m_uiState.spacing)) / 100.0F;
  const float stabilization = static_cast<float>(m_uiState.stabilization) / 100.0F;

  if (m_brushTool != nullptr) {
    m_brushTool->setSize(m_uiState.size);
    m_brushTool->setOpacity(opacity);
    m_brushTool->setHardness(hardness);
    m_brushTool->setFlow(flow);
    m_brushTool->setSpacing(spacing);
    m_brushTool->setAntiAlias(m_uiState.antiAlias);
    m_brushTool->setStabilization(stabilization);
    m_brushTool->setPostCorrection(m_uiState.postCorrection);
    m_brushTool->setVelocityBasedCorrection(m_uiState.velocityBasedCorrection);
    m_brushTool->setShapeType(m_uiState.shapeType);
    m_brushTool->setBlendMode(m_uiState.blendMode);
    m_brushTool->setEraseMode(m_uiState.eraseMode);
    m_brushTool->setLockAlphaRespect(m_uiState.lockAlphaRespect);
    m_brushTool->setColor(m_currentColor);
  }

  if (m_eraserTool != nullptr) {
    m_eraserTool->setSize(m_uiState.size);
    m_eraserTool->setOpacity(opacity);
    m_eraserTool->setFlow(flow);
    m_eraserTool->setHardness(hardness);
    m_eraserTool->setSpacing(spacing);
    m_eraserTool->setAntiAlias(m_uiState.antiAlias);
    m_eraserTool->setStabilization(stabilization);
    m_eraserTool->setPostCorrection(m_uiState.postCorrection);
    m_eraserTool->setVelocityBasedCorrection(m_uiState.velocityBasedCorrection);
    m_eraserTool->setShapeType(m_uiState.shapeType);
  }

  if (m_lineTool != nullptr) {
    m_lineTool->setSnapAngleDegrees(m_uiState.snapAngle);
  }
  if (m_fillTool != nullptr) {
    m_fillTool->setThreshold(m_uiState.fillThreshold);
    m_fillTool->setContiguous(m_uiState.fillContiguous);
    m_fillTool->setReferAllLayers(m_uiState.fillReferAllLayers);
    m_fillTool->setGapClose(m_uiState.fillGapClose);
  }
  if (m_rectSelectionTool != nullptr) {
    core::RectSelectionTool::Mode mode = core::RectSelectionTool::Mode::Rectangle;
    if (m_uiState.selectionMode == app::ui::SelectionMode::Lasso) {
      mode = core::RectSelectionTool::Mode::Lasso;
    } else if (m_uiState.selectionMode == app::ui::SelectionMode::AutoSelect) {
      mode = core::RectSelectionTool::Mode::AutoSelect;
    }
    m_rectSelectionTool->setMode(mode);
    m_rectSelectionTool->setAutoSelectThreshold(m_uiState.autoSelectThreshold);
    m_rectSelectionTool->setAutoSelectContiguous(m_uiState.autoSelectContiguous);
    m_rectSelectionTool->setAutoSelectReferAllLayers(m_uiState.autoSelectReferAllLayers);
  }
}

void AppController::resetToolStateFromDescriptor(const app::ui::SubToolDescriptor& subTool) {
  const app::ui::ToolBehaviorProfile& profile = subTool.profile;
  m_uiState.subToolId = subTool.id;
  m_uiState.size = std::max(1, profile.stroke.size);
  m_uiState.opacity = clampPercent(profile.stroke.opacity);
  m_uiState.hardness = clampPercent(profile.shape.hardness);
  m_uiState.flow = clampPercent(profile.stroke.flow);
  m_uiState.spacing = std::clamp(profile.stroke.spacing, 1, 300);
  m_uiState.angle = std::clamp(profile.shape.angle, -180, 180);
  m_uiState.roundness = clampPercent(profile.shape.roundness);
  m_uiState.taperStart = clampPercent(profile.shape.taperStart);
  m_uiState.taperEnd = clampPercent(profile.shape.taperEnd);
  m_uiState.antiAlias = profile.stroke.antiAlias;
  m_uiState.stabilization = clampPercent(profile.stabilizer.stabilization);
  m_uiState.snapAngle = std::clamp(profile.vector.snapAngle, 0, 180);
  m_uiState.simplifyLevel = clampPercent(profile.vector.simplifyLevel);
  m_uiState.fillThreshold = std::clamp(profile.fill.threshold, 0, 255);
  m_uiState.fillContiguous = profile.fill.contiguous;
  m_uiState.fillReferAllLayers = profile.fill.referAllLayers;
  m_uiState.fillGapClose = std::clamp(profile.fill.gapClose, 0, 8);
  m_uiState.selectionMode = profile.selection.mode;
  m_uiState.autoSelectThreshold = std::clamp(profile.selection.autoSelectThreshold, 0, 255);
  m_uiState.autoSelectContiguous = profile.selection.autoSelectContiguous;
  m_uiState.autoSelectReferAllLayers = profile.selection.autoSelectReferAllLayers;
  m_uiState.postCorrection = profile.stabilizer.postCorrection;
  m_uiState.velocityBasedCorrection = profile.stabilizer.velocityBasedCorrection;
  m_uiState.shapeType = profile.shape.shapeType;
  m_uiState.blendMode = profile.blendMode;
  m_uiState.eraseMode = profile.eraseMode;
  m_uiState.lockAlphaRespect = profile.lockAlphaRespect;
}

core::ToolContext AppController::makeToolContext() {
  return core::ToolContext {
      m_document,
      m_composited,
      m_currentColor,
      m_uiState.size};
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

  const PendingStrokeState pending = *m_pendingStroke;
  m_pendingStroke.reset();

  if (pending.trackPixels) {
    const std::size_t layerIndex = pending.layerIndex;
    if (layerIndex >= m_document.layerCount()) {
      return;
    }

    if (!pending.beforeLayer.has_value()) {
      return;
    }
    const core::Layer after = m_document.layerAt(layerIndex);
    if (!layersEqual(*pending.beforeLayer, after)) {
      StrokeHistoryEntry entry;
      entry.kind = HistoryKind::Stroke;
      entry.actionName = pending.actionName;
      entry.layerIndex = layerIndex;
      entry.beforeLayer = *pending.beforeLayer;
      entry.afterLayer = after;
      pushHistoryEntry(std::move(entry));
    }
  }

  if (pending.trackSelection) {
    const core::SelectionMask afterSelection = m_document.selection();
    if (pending.beforeSelection != afterSelection) {
      StrokeHistoryEntry entry;
      entry.kind = HistoryKind::Selection;
      entry.actionName = "Selection";
      entry.beforeSelection = pending.beforeSelection;
      entry.afterSelection = afterSelection;
      pushHistoryEntry(std::move(entry));
    }
  }
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

void AppController::pushSelectionHistoryIfChanged(const core::SelectionMask& before, const std::string& actionName) {
  const core::SelectionMask after = m_document.selection();
  if (before == after) {
    return;
  }
  StrokeHistoryEntry entry;
  entry.kind = HistoryKind::Selection;
  entry.actionName = actionName;
  entry.beforeSelection = before;
  entry.afterSelection = after;
  pushHistoryEntry(std::move(entry));
}

void AppController::clearStrokeHistory() noexcept {
  m_undoHistory.clear();
  m_redoHistory.clear();
}

void AppController::rerender() {
  m_composited = m_renderer.composite(m_document);
}

} // namespace app::bridge
