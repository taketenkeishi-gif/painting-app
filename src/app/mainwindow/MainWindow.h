#pragma once

#include <QMainWindow>

class QAction;
class QLabel;
class QWidget;

namespace app::bridge {
class AppController;
}
namespace app::canvasview {
class CanvasWidget;
}
namespace app::panels {
class LayerPanel;
class SubToolPanel;
class ToolPanel;
class ToolPropertyPanel;
}

namespace app::mainwindow {

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);

private slots:
  void onNewCanvas();
  void onToolStateChanged();
  void onUndoTriggered();
  void onRedoTriggered();

private:
  void setupShellLayout();
  void createMenus();
  void updateUndoRedoState();
  void updateActiveLayerStatus();
  void updateTopToolInfo();

  app::bridge::AppController* m_controller {nullptr};
  app::canvasview::CanvasWidget* m_canvasWidget {nullptr};
  app::panels::LayerPanel* m_layerPanel {nullptr};
  app::panels::ToolPanel* m_toolPanel {nullptr};
  app::panels::SubToolPanel* m_subToolPanel {nullptr};
  app::panels::ToolPropertyPanel* m_toolPropertyPanel {nullptr};
  QWidget* m_leftToolHost {nullptr};
  QWidget* m_topBar {nullptr};
  QWidget* m_rightPanelHost {nullptr};
  QLabel* m_currentToolLabel {nullptr};
  QLabel* m_currentSubToolLabel {nullptr};
  QLabel* m_toolStatusLabel {nullptr};
  QLabel* m_guideStatusLabel {nullptr};
  QLabel* m_colorStatusLabel {nullptr};
  QLabel* m_sizeStatusLabel {nullptr};
  QLabel* m_zoomStatusLabel {nullptr};
  QLabel* m_activeLayerStatusLabel {nullptr};
  QAction* m_newCanvasAction {nullptr};
  QAction* m_undoAction {nullptr};
  QAction* m_redoAction {nullptr};
  QAction* m_addLayerAction {nullptr};
  int m_lastCanvasWidth {800};
  int m_lastCanvasHeight {600};
};

} // namespace app::mainwindow
