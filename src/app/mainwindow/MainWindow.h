#pragma once

#include <QMainWindow>

class QAction;
class QLabel;
class QPushButton;
class QSpinBox;
class QWidget;

namespace app::bridge {
class AppController;
}
namespace app::canvasview {
class CanvasWidget;
}
namespace app::panels {
class LayerPanel;
}

namespace app::mainwindow {

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);

private slots:
  void onNewCanvas();
  void onChooseBrushColor();
  void onBrushSizeChanged(int size);
  void onToolStateChanged();
  void onUndoTriggered();
  void onRedoTriggered();

private:
  void setupShellLayout();
  void createMenus();
  void updateUndoRedoState();
  void updateActiveLayerStatus();
  void updateBrushColorButton();

  app::bridge::AppController* m_controller {nullptr};
  app::canvasview::CanvasWidget* m_canvasWidget {nullptr};
  app::panels::LayerPanel* m_layerPanel {nullptr};
  QWidget* m_leftToolHost {nullptr};
  QWidget* m_topBar {nullptr};
  QWidget* m_rightPanelHost {nullptr};
  QLabel* m_subToolPlaceholderLabel {nullptr};
  QLabel* m_propertyPlaceholderLabel {nullptr};
  QLabel* m_activeLayerStatusLabel {nullptr};
  QAction* m_newCanvasAction {nullptr};
  QAction* m_undoAction {nullptr};
  QAction* m_redoAction {nullptr};
  QAction* m_addLayerAction {nullptr};
  QPushButton* m_brushColorButton {nullptr};
  QSpinBox* m_brushSizeSpin {nullptr};
  int m_lastCanvasWidth {800};
  int m_lastCanvasHeight {600};
};

} // namespace app::mainwindow
