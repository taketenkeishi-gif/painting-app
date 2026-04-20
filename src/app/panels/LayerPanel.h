#pragma once

#include <QListWidget>
#include <QPushButton>
#include <QWidget>

class QLabel;
class QSlider;

namespace app::bridge {
class AppController;
}

namespace app::panels {

class LayerPanel : public QWidget {
  Q_OBJECT

public:
  explicit LayerPanel(QWidget* parent = nullptr);
  void setController(app::bridge::AppController* controller);

private slots:
  void refreshLayers();
  void onAddLayerClicked();
  void onDeleteLayerClicked();
  void onCurrentLayerChanged(int row);
  void onLayerItemChanged(QListWidgetItem* item);
  void onOpacityChanged(int value);

private:
  void refreshButtonState();

  app::bridge::AppController* m_controller {nullptr};
  QLabel* m_headerLabel {nullptr};
  QListWidget* m_layerList {nullptr};
  QLabel* m_opacityLabel {nullptr};
  QSlider* m_opacitySlider {nullptr};
  QPushButton* m_addButton {nullptr};
  QPushButton* m_deleteButton {nullptr};
  bool m_isRefreshing {false};
};

} // namespace app::panels
