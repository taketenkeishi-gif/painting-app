#pragma once

#include <cstddef>

#include <QListWidget>
#include <QPushButton>
#include <QWidget>

class QLabel;
class QModelIndex;
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
  void onAddRasterLayerClicked();
  void onAddVectorLayerClicked();
  void onAddFolderLayerClicked();
  void onDuplicateLayerClicked();
  void onDeleteLayerClicked();
  void onMoveLayerUpClicked();
  void onMoveLayerDownClicked();
  void onToggleClipClicked();
  void onToggleMaskClicked();
  void onRemoveMaskClicked();
  void onCurrentLayerChanged(int row);
  void onLayerItemChanged(QListWidgetItem* item);
  void onLayerRowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row);
  void onOpacityChanged(int value);

private:
  std::size_t layerIndexFromRow(int row) const;
  int rowFromLayerIndex(std::size_t layerIndex) const;
  void refreshButtonState();

  app::bridge::AppController* m_controller {nullptr};
  QLabel* m_headerLabel {nullptr};
  QListWidget* m_layerList {nullptr};
  QLabel* m_opacityLabel {nullptr};
  QSlider* m_opacitySlider {nullptr};
  QPushButton* m_addRasterButton {nullptr};
  QPushButton* m_addVectorButton {nullptr};
  QPushButton* m_addFolderButton {nullptr};
  QPushButton* m_duplicateButton {nullptr};
  QPushButton* m_upButton {nullptr};
  QPushButton* m_downButton {nullptr};
  QPushButton* m_deleteButton {nullptr};
  QPushButton* m_clipButton {nullptr};
  QPushButton* m_maskButton {nullptr};
  QPushButton* m_removeMaskButton {nullptr};
  bool m_isRefreshing {false};
  bool m_isDraggingLayer {false};
};

} // namespace app::panels
