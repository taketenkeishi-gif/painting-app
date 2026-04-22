#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <QObject>

#include "app/ui/ToolDescriptor.h"
#include "app/ui/UiState.h"
#include "core/common/Point.h"
#include "core/document/Document.h"
#include "core/render/Renderer.h"
#include "core/tools/BrushTool.h"
#include "core/tools/EraserTool.h"
#include "core/tools/EyedropperTool.h"
#include "core/tools/FillTool.h"
#include "core/tools/HandTool.h"
#include "core/tools/LineTool.h"
#include "core/tools/MoveLayerTool.h"
#include "core/tools/RectSelectionTool.h"
#include "core/tools/ToolManager.h"
#include "core/tools/ZoomTool.h"

namespace app::bridge {

struct LayerViewModel {
  std::string name;
  bool visible {true};
  bool active {false};
  int opacityPercent {100};
  core::LayerKind kind {core::LayerKind::Raster};
  bool clippedToBelow {false};
  bool hasMask {false};
  bool maskEnabled {false};
  bool locked {false};
  bool alphaLocked {false};
  bool positionLocked {false};
};

struct SubToolViewModel {
  std::string id;
  std::string name;
  bool active {false};
  bool enabled {true};
  std::string hint;
};

struct ToolStateViewModel {
  core::Color color {0, 0, 0, 255};
  int size {8};
  int strokeWidth {8};
  int opacity {100};
  int hardness {100};
  int flow {100};
  int spacing {25};
  int angle {0};
  int roundness {100};
  int taperStart {0};
  int taperEnd {0};
  bool antiAlias {true};
  int stabilization {0};
  int snapAngle {0};
  int simplifyLevel {0};
  int fillThreshold {0};
  bool fillContiguous {true};
  bool fillReferAllLayers {false};
  int fillGapClose {0};
  app::ui::SelectionMode selectionMode {app::ui::SelectionMode::Rectangle};
  int autoSelectThreshold {16};
  bool autoSelectContiguous {true};
  bool autoSelectReferAllLayers {true};
  bool postCorrection {false};
  bool velocityBasedCorrection {false};
  core::BrushShapeType shapeType {core::BrushShapeType::Circle};
  core::BlendMode blendMode {core::BlendMode::Normal};
  bool eraseMode {false};
  bool lockAlphaRespect {false};
};

struct CanvasOverlayViewModel {
  core::ToolOverlayState toolOverlay;
  std::optional<core::Rect> selectionRect;
};

class AppController : public QObject {
  Q_OBJECT

public:
  explicit AppController(QObject* parent = nullptr);

  const core::Document& document() const noexcept { return m_document; }
  const core::PixelBuffer& compositedBuffer() const noexcept { return m_composited; }
  CanvasOverlayViewModel canvasOverlay() const;

  std::vector<LayerViewModel> layerViewModels() const;
  std::vector<SubToolViewModel> subToolViewModels() const;
  ToolStateViewModel toolState() const noexcept;

  void newDocument(int width, int height);
  void addLayer();
  void addRasterLayer();
  void addVectorLayer();
  void addFolderLayer();
  bool duplicateLayer(std::size_t index);
  bool duplicateActiveLayer();
  bool removeLayer(std::size_t index);
  bool mergeLayerDown(std::size_t index);
  bool mergeActiveLayerDown();
  bool rasterizeLayer(std::size_t index);
  bool rasterizeActiveLayer();
  bool renameLayer(std::size_t index, const std::string& name);
  bool moveLayer(std::size_t fromIndex, std::size_t toIndex);
  bool moveLayerUp(std::size_t index);
  bool moveLayerDown(std::size_t index);
  bool moveActiveLayerUp();
  bool moveActiveLayerDown();
  void setActiveLayer(std::size_t index);
  void setLayerVisible(std::size_t index, bool visible);
  void setLayerOpacity(std::size_t index, int opacityPercent);
  void setActiveLayerOpacity(int opacityPercent);
  int activeLayerOpacity() const noexcept;
  bool toggleActiveLayerVisible();
  bool toggleActiveLayerClipToBelow();
  bool toggleActiveLayerMask();
  bool removeActiveLayerMask();
  bool toggleActiveLayerLock();
  bool toggleActiveLayerAlphaLock();
  bool toggleActiveLayerPositionLock();

  bool clearSelection();
  bool selectAll();
  bool deselect();
  bool invertSelection();
  bool fillSelectionOrCanvas();
  bool deleteSelectionPixels();
  core::PixelBuffer exportSelectionOrCanvasFromComposite() const;
  void importFlattenedBuffer(const core::PixelBuffer& buffer, const std::string& layerName = "Imported");
  bool pasteBufferAsNewRasterLayer(const core::PixelBuffer& buffer, const std::string& layerName = "Pasted Layer");

  std::vector<core::ToolKind> availableTools() const;
  std::string toolDisplayName(core::ToolKind kind) const;
  bool canUseToolOnActiveLayer(core::ToolKind kind) const;
  bool setCurrentTool(core::ToolKind kind);
  core::ToolKind currentTool() const noexcept;
  bool setCurrentSubTool(const std::string& subToolId);
  std::string currentSubToolId() const;
  bool duplicateCurrentSubTool();
  bool renameCurrentSubTool(const std::string& displayName);
  bool deleteCurrentSubTool();
  bool resetCurrentSubTool();

  std::string currentToolDisplayName() const;
  std::string currentSubToolDisplayName() const;
  std::string currentToolGuide() const;
  std::string activeLayerKindDisplayName() const;
  bool canUseCurrentToolOnActiveLayer() const;
  std::string currentLayerCompatibilityHint() const;

  bool currentToolSupportsColor() const noexcept;
  bool currentToolSupportsSize() const noexcept;
  bool currentToolSupportsOpacity() const noexcept;
  bool currentToolSupportsHardness() const noexcept;
  bool currentToolSupportsFlow() const noexcept;
  bool currentToolSupportsSpacing() const noexcept;
  bool currentToolSupportsAntiAlias() const noexcept;
  bool currentToolSupportsStabilization() const noexcept;
  bool currentToolSupportsPostCorrection() const noexcept;
  bool currentToolSupportsVelocityCorrection() const noexcept;
  bool currentToolSupportsShapeType() const noexcept;
  bool currentToolSupportsAngle() const noexcept;
  bool currentToolSupportsRoundness() const noexcept;
  bool currentToolSupportsTaperStart() const noexcept;
  bool currentToolSupportsTaperEnd() const noexcept;
  bool currentToolSupportsBlendMode() const noexcept;
  bool currentToolSupportsEraseMode() const noexcept;
  bool currentToolSupportsLockAlphaRespect() const noexcept;
  bool currentToolSupportsSnapAngle() const noexcept;
  bool currentToolSupportsSimplifyLevel() const noexcept;
  bool currentToolSupportsFillThreshold() const noexcept;
  bool currentToolSupportsFillContiguous() const noexcept;
  bool currentToolSupportsFillReferAllLayers() const noexcept;
  bool currentToolSupportsFillGapClose() const noexcept;
  bool currentToolSupportsSelectionMode() const noexcept;
  bool currentToolSupportsAutoSelectThreshold() const noexcept;
  bool currentToolSupportsAutoSelectContiguous() const noexcept;
  bool currentToolSupportsAutoSelectReferAllLayers() const noexcept;

  void beginStroke(int x, int y);
  void continueStroke(int x, int y);
  void endStroke();
  bool pickColorAt(int x, int y);

  bool undo();
  bool redo();
  bool canUndo() const noexcept;
  bool canRedo() const noexcept;
  std::string nextUndoActionName() const;
  std::string nextRedoActionName() const;

  void setBrushColor(const core::Color& color);
  void setBrushSize(int size);
  void adjustBrushSize(int delta);
  void setBrushOpacity(int opacity);
  void setBrushHardness(int hardness);
  void setBrushFlow(int flow);
  void setBrushSpacing(int spacing);
  void setBrushAntiAlias(bool antiAlias);
  void setBrushStabilization(int stabilization);
  void setBrushPostCorrection(bool enabled);
  void setBrushVelocityBasedCorrection(bool enabled);
  void setBrushShapeType(core::BrushShapeType shapeType);
  void setBrushAngle(int angle);
  void setBrushRoundness(int roundness);
  void setBrushTaperStart(int taperStart);
  void setBrushTaperEnd(int taperEnd);
  void setBrushBlendMode(core::BlendMode blendMode);
  void setBrushEraseMode(bool eraseMode);
  void setBrushLockAlphaRespect(bool enabled);
  void setLineSnapAngle(int snapAngle);
  void setLineSimplifyLevel(int simplifyLevel);
  void setFillThreshold(int threshold);
  void setFillContiguous(bool contiguous);
  void setFillReferAllLayers(bool enabled);
  void setFillGapClose(int gapClose);
  void setSelectionMode(app::ui::SelectionMode mode);
  void setAutoSelectThreshold(int threshold);
  void setAutoSelectContiguous(bool contiguous);
  void setAutoSelectReferAllLayers(bool enabled);

signals:
  void documentChanged();
  void layersChanged();
  void toolStateChanged();

private:
  enum class HistoryKind {
    Stroke,
    LayerVisibility,
    LayerOrder,
    Selection
  };

  struct StrokeHistoryEntry {
    HistoryKind kind {HistoryKind::Stroke};
    std::string actionName {"Stroke"};
    std::size_t layerIndex {0};
    std::optional<core::Layer> beforeLayer;
    std::optional<core::Layer> afterLayer;
    bool beforeVisible {true};
    bool afterVisible {true};
    std::size_t beforeIndex {0};
    std::size_t afterIndex {0};
    core::SelectionMask beforeSelection;
    core::SelectionMask afterSelection;
  };

  struct PendingStrokeState {
    bool trackPixels {false};
    bool trackSelection {false};
    std::string actionName {"Stroke"};
    std::size_t layerIndex {0};
    std::optional<core::Layer> beforeLayer;
    core::SelectionMask beforeSelection;
  };

  static bool toolWritesPixels(core::ToolKind kind) noexcept;
  static bool toolWritesSelection(core::ToolKind kind) noexcept;
  static std::string actionNameForTool(core::ToolKind kind);
  bool isSubToolCompatibleWithLayerKind(const app::ui::SubToolDescriptor& subTool, core::LayerKind layerKind) const noexcept;
  bool isCurrentSubToolCompatibleWithActiveLayer() const noexcept;
  const app::ui::SubToolDescriptor* firstCompatibleSubTool(core::ToolKind kind, core::LayerKind layerKind) const noexcept;
  void ensureCurrentSubToolCompatibility();

  const app::ui::ToolDescriptor* currentToolDescriptor() const noexcept;
  const app::ui::SubToolDescriptor* currentSubToolDescriptor() const noexcept;
  bool selectSubToolInternal(std::string_view subToolId, bool emitSignal);
  void applyUiStateToTools();
  void resetToolStateFromDescriptor(const app::ui::SubToolDescriptor& subTool);

  core::ToolContext makeToolContext();
  void applyToolResult(const core::ToolResult& result);
  void finishPendingStrokeHistory();
  void pushHistoryEntry(StrokeHistoryEntry entry);
  void pushSelectionHistoryIfChanged(const core::SelectionMask& before, const std::string& actionName);
  void clearStrokeHistory() noexcept;
  void rerender();

  core::Document m_document;
  core::Renderer m_renderer;
  core::PixelBuffer m_composited;

  core::ToolManager m_toolManager;
  core::BrushTool* m_brushTool {nullptr};
  core::EraserTool* m_eraserTool {nullptr};
  core::LineTool* m_lineTool {nullptr};
  core::RectSelectionTool* m_rectSelectionTool {nullptr};
  core::FillTool* m_fillTool {nullptr};

  app::ui::ToolCatalog m_toolCatalog;
  app::ui::UiState m_uiState;
  std::unordered_map<core::ToolKind, std::string> m_selectedSubToolByTool;

  bool m_stroking {false};
  std::size_t m_layerCounter {1};
  std::optional<PendingStrokeState> m_pendingStroke;
  core::Point m_lastPointer {0, 0};
  core::Color m_currentColor {0, 0, 0, 255};

  std::vector<StrokeHistoryEntry> m_undoHistory;
  std::vector<StrokeHistoryEntry> m_redoHistory;
  std::size_t m_maxStrokeHistory {20};
};

} // namespace app::bridge
