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
  m_toolManager.registerTool(std::make_unique<core::LineTool>());
  m_toolManager.registerTool(std::make_unique<core::RectSelectionTool>());
  m_toolManager.registerTool(std::make_unique<core::FillTool>());
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
        static_cast<int>(std::lround(std::clamp(layer.opacity(), 0.0F, 1.0F) * 100.0F))});
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
  models.reserve(descriptor->subTools.size());
  for (const app::ui::SubToolDescriptor& sub : descriptor->subTools) {
    models.push_back(SubToolViewModel {sub.id, sub.displayName, sub.id == selectedId});
  }
  return models;
}

ToolStateViewModel AppController::toolState() const noexcept {
  return ToolStateViewModel {
      m_currentColor,
      m_uiState.size,
      m_uiState.opacity,
      m_uiState.hardness,
      m_uiState.flow,
      m_uiState.spacing,
      m_uiState.antiAlias,
      m_uiState.stabilization,
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

bool AppController::invertSelection() {
  const core::SelectionMask before = m_document.selection();
  if (!m_document.selection().invert()) {
    return false;
  }
  pushSelectionHistoryIfChanged(before, "Selection");
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
  const app::ui::SubToolDescriptor* sub = currentSubToolDescriptor();
  if (sub != nullptr && !sub->guide.empty()) {
    return sub->guide;
  }
  const app::ui::ToolDescriptor* descriptor = currentToolDescriptor();
  if (descriptor != nullptr) {
    return descriptor->guide;
  }
  return "";
}

bool AppController::currentToolSupportsColor() const noexcept {
  return !m_uiState.eraseMode && containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::Color);
}

bool AppController::currentToolSupportsSize() const noexcept {
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::Size);
}

bool AppController::currentToolSupportsOpacity() const noexcept {
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::Opacity);
}

bool AppController::currentToolSupportsHardness() const noexcept {
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::Hardness);
}

bool AppController::currentToolSupportsFlow() const noexcept {
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::Flow);
}

bool AppController::currentToolSupportsSpacing() const noexcept {
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::Spacing);
}

bool AppController::currentToolSupportsAntiAlias() const noexcept {
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::AntiAlias);
}

bool AppController::currentToolSupportsStabilization() const noexcept {
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::Stabilization);
}

bool AppController::currentToolSupportsPostCorrection() const noexcept {
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::PostCorrection);
}

bool AppController::currentToolSupportsVelocityCorrection() const noexcept {
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::VelocityCorrection);
}

bool AppController::currentToolSupportsShapeType() const noexcept {
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::ShapeType);
}

bool AppController::currentToolSupportsBlendMode() const noexcept {
  return !m_uiState.eraseMode &&
         containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::BlendMode);
}

bool AppController::currentToolSupportsEraseMode() const noexcept {
  return containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::EraseMode);
}

bool AppController::currentToolSupportsLockAlphaRespect() const noexcept {
  return !m_uiState.eraseMode &&
         containsProperty(currentToolDescriptor(), currentSubToolDescriptor(), app::ui::ToolPropertyKey::LockAlphaRespect);
}

void AppController::beginStroke(int x, int y) {
  if (m_stroking) {
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
    pending.before = activeLayer->buffer();
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

  switch (entry.kind) {
    case HistoryKind::Stroke:
      m_document.layerAt(entry.layerIndex).buffer() = entry.before;
      break;
    case HistoryKind::LayerVisibility:
      m_document.setLayerVisible(entry.layerIndex, entry.beforeVisible);
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

  if ((entry.kind == HistoryKind::Stroke || entry.kind == HistoryKind::LayerVisibility) &&
      entry.layerIndex >= m_document.layerCount()) {
    clearStrokeHistory();
    return false;
  }

  switch (entry.kind) {
    case HistoryKind::Stroke:
      m_document.layerAt(entry.layerIndex).buffer() = entry.after;
      break;
    case HistoryKind::LayerVisibility:
      m_document.setLayerVisible(entry.layerIndex, entry.afterVisible);
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
      return "Stroke";
    case core::ToolKind::Eraser:
      return "Eraser";
    case core::ToolKind::Line:
      return "Line";
    case core::ToolKind::Fill:
      return "Fill";
    case core::ToolKind::MoveLayer:
      return "Move Layer";
    case core::ToolKind::RectSelection:
      return "Selection";
    case core::ToolKind::Eyedropper:
      return "Eyedropper";
    case core::ToolKind::Hand:
      return "Hand";
    case core::ToolKind::Zoom:
      return "Zoom";
    default:
      return "Action";
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
}

void AppController::resetToolStateFromDescriptor(const app::ui::SubToolDescriptor& subTool) {
  m_uiState.subToolId = subTool.id;
  m_uiState.size = std::max(1, subTool.preset.size);
  m_uiState.opacity = clampPercent(subTool.preset.opacity);
  m_uiState.hardness = clampPercent(subTool.preset.hardness);
  m_uiState.flow = clampPercent(subTool.preset.flow);
  m_uiState.spacing = std::clamp(subTool.preset.spacing, 1, 300);
  m_uiState.antiAlias = subTool.preset.antiAlias;
  m_uiState.stabilization = clampPercent(subTool.preset.stabilization);
  m_uiState.postCorrection = subTool.preset.postCorrection;
  m_uiState.velocityBasedCorrection = subTool.preset.velocityBasedCorrection;
  m_uiState.shapeType = subTool.preset.shapeType;
  m_uiState.blendMode = subTool.preset.blendMode;
  m_uiState.eraseMode = subTool.preset.eraseMode;
  m_uiState.lockAlphaRespect = subTool.preset.lockAlphaRespect;
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

    const core::PixelBuffer after = m_document.layerAt(layerIndex).buffer();
    if (!pixelBuffersEqual(pending.before, after)) {
      StrokeHistoryEntry entry;
      entry.kind = HistoryKind::Stroke;
      entry.actionName = pending.actionName;
      entry.layerIndex = layerIndex;
      entry.before = pending.before;
      entry.after = after;
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
