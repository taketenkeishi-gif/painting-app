#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <QObject>

#include "core/document/Document.h"
#include "core/render/Renderer.h"
#include "core/tools/BrushTool.h"

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

  void beginStroke(int x, int y);
  void continueStroke(int x, int y);
  void endStroke();
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

  void pushHistoryEntry(StrokeHistoryEntry entry);
  void clearStrokeHistory() noexcept;
  void rerender();

  core::Document m_document;
  core::Renderer m_renderer;
  core::PixelBuffer m_composited;
  core::BrushTool m_brushTool;
  bool m_stroking {false};
  core::Point m_lastPoint {0, 0};
  std::size_t m_layerCounter {1};
  std::optional<PendingStrokeState> m_pendingStroke;
  std::vector<StrokeHistoryEntry> m_undoHistory;
  std::vector<StrokeHistoryEntry> m_redoHistory;
  std::size_t m_maxStrokeHistory {20};
};

} // namespace app::bridge
