#pragma once

#include <cstddef>

#include <QListWidget>
#include <QPushButton>
#include <QString>
#include <QWidget>

class QLabel;
class QModelIndex;
class QSlider;
class QSpinBox;
class QLineEdit;
class QComboBox;
class QPoint;
class QResizeEvent;
class QGridLayout;
class QGroupBox;

namespace app::bridge {
class AppController;
}

namespace app::panels {

class LayerPanel : public QWidget {
  Q_OBJECT

public:
  explicit LayerPanel(QWidget* parent = nullptr);
  void setController(app::bridge::AppController* controller);

protected:
  void resizeEvent(QResizeEvent* event) override;

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
  void onToggleLockClicked();
  void onToggleAlphaLockClicked();
  void onTogglePositionLockClicked();
  void onCurrentLayerChanged(int row);
  void onLayerItemChanged(QListWidgetItem* item);
  void onLayerRowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row);
  void onOpacityChanged(int value);
  void onBlendModeChanged(int index);
  void onFilterTextChanged(const QString& text);
  void onLayerContextMenuRequested(const QPoint& pos);
  void onQuickAddClicked();
  void onQuickRemoveClicked();

private:
  std::size_t layerIndexFromRow(int row) const;
  int rowFromLayerIndex(std::size_t layerIndex) const;
  void refreshButtonState();
  void applyResponsiveMode();
  void applyButtonCompactMode(bool compact);

  app::bridge::AppController* m_controller {nullptr};
  QLabel* m_headerLabel {nullptr};
  QLineEdit* m_filterEdit {nullptr};
  QListWidget* m_layerList {nullptr};
  QLabel* m_opacityLabel {nullptr};
  QSlider* m_opacitySlider {nullptr};
  QSpinBox* m_opacitySpin {nullptr};
  QComboBox* m_blendModeCombo {nullptr};
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
  QPushButton* m_lockButton {nullptr};
  QPushButton* m_lockAlphaButton {nullptr};
  QPushButton* m_lockPositionButton {nullptr};
  QGroupBox* m_primaryGroup {nullptr};
  QGroupBox* m_stateGroup {nullptr};
  QGridLayout* m_primaryGrid {nullptr};
  QGridLayout* m_stateGrid {nullptr};
  QPushButton* m_quickAddButton {nullptr};
  QPushButton* m_quickRemoveButton {nullptr};
  bool m_compactButtons {false};
  bool m_isRefreshing {false};
  bool m_isDraggingLayer {false};
};

} // namespace app::panels
