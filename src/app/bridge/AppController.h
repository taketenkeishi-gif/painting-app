#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <QObject>

#include "core/common/Point.h"
#include "core/document/Document.h"
#include "core/render/Renderer.h"
#include "core/tools/BrushTool.h"
#include "core/tools/EraserTool.h"
#include "core/tools/EyedropperTool.h"
#include "core/tools/HandTool.h"
#include "core/tools/ToolManager.h"
#include "core/tools/ZoomTool.h"

namespace app::bridge {

struct LayerViewModel {
  std::string name;
  bool visible {true};
  bool active {false};
};

struct ToolStateViewModel {
  core::Color color {0, 0, 0, 255};
  int size {8};
};

class AppController : public QObject {
  Q_OBJECT

public:
  explicit AppController(QObject* parent = nullptr);

  const core::Document& document() const noexcept { return m_document; }
  const core::PixelBuffer& compositedBuffer() const noexcept { return m_composited; }

  std::vector<LayerViewModel> layerViewModels() const;
  ToolStateViewModel toolState() const noexcept;

  void newDocument(int width, int height);
  void addLayer();
  bool removeLayer(std::size_t index);
  bool renameLayer(std::size_t index, const std::string& name);
  void setActiveLayer(std::size_t index);
  void setLayerVisible(std::size_t index, bool visible);

  bool setCurrentTool(core::ToolKind kind);
  core::ToolKind currentTool() const noexcept;

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

signals:
  void documentChanged();
  void layersChanged();
  void toolStateChanged();

private:
  enum class HistoryKind {
    Stroke,
    LayerVisibility
  };

  struct StrokeHistoryEntry {
    HistoryKind kind {HistoryKind::Stroke};
    std::size_t layerIndex {0};
    core::PixelBuffer before;
    core::PixelBuffer after;
    bool beforeVisible {true};
    bool afterVisible {true};
  };

  struct PendingStrokeState {
    std::size_t layerIndex {0};
    core::PixelBuffer before;
  };

  static bool toolWritesPixels(core::ToolKind kind) noexcept;

  core::ToolContext makeToolContext();
  void applyToolResult(const core::ToolResult& result);
  void finishPendingStrokeHistory();
  void pushHistoryEntry(StrokeHistoryEntry entry);
  void clearStrokeHistory() noexcept;
  void rerender();

  core::Document m_document;
  core::Renderer m_renderer;
  core::PixelBuffer m_composited;

  core::ToolManager m_toolManager;
  core::BrushTool* m_brushTool {nullptr};
  core::EraserTool* m_eraserTool {nullptr};

  bool m_stroking {false};
  std::size_t m_layerCounter {1};
  std::optional<PendingStrokeState> m_pendingStroke;
  core::Point m_lastPointer {0, 0};
  core::Color m_currentColor {0, 0, 0, 255};
  int m_brushSize {8};

  std::vector<StrokeHistoryEntry> m_undoHistory;
  std::vector<StrokeHistoryEntry> m_redoHistory;
  std::size_t m_maxStrokeHistory {20};
};

} // namespace app::bridge
