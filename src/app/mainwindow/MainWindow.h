#pragma once

#include <QMainWindow>

class QPushButton;
class QSpinBox;

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

private:
  void createMenus();
  void updateBrushColorButton();

  app::bridge::AppController* m_controller {nullptr};
  app::canvasview::CanvasWidget* m_canvasWidget {nullptr};
  app::panels::LayerPanel* m_layerPanel {nullptr};
  QPushButton* m_brushColorButton {nullptr};
  QSpinBox* m_brushSizeSpin {nullptr};
  int m_lastCanvasWidth {800};
  int m_lastCanvasHeight {600};
};

} // namespace app::mainwindow
