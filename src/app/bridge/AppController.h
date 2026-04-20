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
};

struct SubToolViewModel {
  std::string id;
  std::string name;
  bool active {false};
};

struct ToolStateViewModel {
  core::Color color {0, 0, 0, 255};
  int size {8};
  int opacity {100};
  int hardness {100};
  int flow {100};
  int spacing {25};
  bool antiAlias {true};
  int stabilization {0};
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
  bool duplicateLayer(std::size_t index);
  bool duplicateActiveLayer();
  bool removeLayer(std::size_t index);
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

  bool clearSelection();
  bool invertSelection();

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
  bool currentToolSupportsBlendMode() const noexcept;
  bool currentToolSupportsEraseMode() const noexcept;
  bool currentToolSupportsLockAlphaRespect() const noexcept;

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
  void setBrushBlendMode(core::BlendMode blendMode);
  void setBrushEraseMode(bool eraseMode);
  void setBrushLockAlphaRespect(bool enabled);

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
