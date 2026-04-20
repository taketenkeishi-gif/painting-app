#pragma once

#include <map>

#include <QMainWindow>
#include <QByteArray>

#include "core/color/Color.h"
#include "core/tools/ToolType.h"

class QAction;
class QLabel;
class QMenu;
class QKeySequence;
class QDockWidget;
class QPushButton;
class QSplitter;
class QTabWidget;
class QToolBar;
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
  void onSetToolTriggered();
  void onClearSelectionTriggered();
  void onInvertSelectionTriggered();
  void onToggleLayerVisibilityTriggered();
  void onMoveLayerUpTriggered();
  void onMoveLayerDownTriggered();
  void onAddRasterLayerTriggered();
  void onAddVectorLayerTriggered();
  void onDuplicateLayerTriggered();
  void onDeleteLayerTriggered();
  void onDecreaseBrushSizeTriggered();
  void onIncreaseBrushSizeTriggered();
  void onZoomInTriggered();
  void onZoomOutTriggered();
  void onResetZoomTriggered();
  void onFitToScreenTriggered();
  void onResetWorkspaceTriggered();
  void onChooseForegroundColor();
  void onChooseBackgroundColor();
  void onSwapColors();

private:
  void setupShellLayout();
  void createMenus();
  void createToolBar();
  void applyUiChrome();
  void updateUndoRedoState();
  void updateActiveLayerStatus();
  void updateTopToolInfo();
  void updateColorPanel();
  void updateToolActionState();
  QAction* createToolAction(QMenu* toolMenu, core::ToolKind kind, const QString& text, const QKeySequence& shortcut);

  app::bridge::AppController* m_controller {nullptr};
  app::canvasview::CanvasWidget* m_canvasWidget {nullptr};
  app::panels::LayerPanel* m_layerPanel {nullptr};
  app::panels::ToolPanel* m_toolPanel {nullptr};
  app::panels::SubToolPanel* m_subToolPanel {nullptr};
  app::panels::ToolPropertyPanel* m_toolPropertyPanel {nullptr};
  QWidget* m_leftToolHost {nullptr};
  QWidget* m_topBar {nullptr};
  QWidget* m_rightPanelHost {nullptr};
  QSplitter* m_mainSplitter {nullptr};
  QSplitter* m_leftSplitter {nullptr};
  QSplitter* m_rightSplitter {nullptr};
  QTabWidget* m_rightTabWidget {nullptr};
  QDockWidget* m_toolDock {nullptr};
  QDockWidget* m_subToolDock {nullptr};
  QDockWidget* m_toolPropertyDock {nullptr};
  QDockWidget* m_colorDock {nullptr};
  QDockWidget* m_layerDock {nullptr};
  QDockWidget* m_infoDock {nullptr};
  QToolBar* m_quickToolBar {nullptr};
  QLabel* m_currentToolLabel {nullptr};
  QLabel* m_currentSubToolLabel {nullptr};
  QLabel* m_toolStatusLabel {nullptr};
  QLabel* m_subToolStatusLabel {nullptr};
  QLabel* m_guideStatusLabel {nullptr};
  QLabel* m_colorStatusLabel {nullptr};
  QLabel* m_sizeStatusLabel {nullptr};
  QLabel* m_zoomStatusLabel {nullptr};
  QLabel* m_activeLayerStatusLabel {nullptr};
  QLabel* m_selectionStatusLabel {nullptr};
  QPushButton* m_foregroundColorButton {nullptr};
  QPushButton* m_backgroundColorButton {nullptr};
  core::Color m_backgroundColor {255, 255, 255, 255};
  QAction* m_newCanvasAction {nullptr};
  QAction* m_undoAction {nullptr};
  QAction* m_redoAction {nullptr};
  QAction* m_addLayerAction {nullptr};
  QAction* m_addRasterLayerAction {nullptr};
  QAction* m_addVectorLayerAction {nullptr};
  QAction* m_duplicateLayerAction {nullptr};
  QAction* m_deleteLayerAction {nullptr};
  QAction* m_moveLayerUpAction {nullptr};
  QAction* m_moveLayerDownAction {nullptr};
  QAction* m_toggleLayerVisibilityAction {nullptr};
  QAction* m_clearSelectionAction {nullptr};
  QAction* m_invertSelectionAction {nullptr};
  QAction* m_brushSizeDownAction {nullptr};
  QAction* m_brushSizeUpAction {nullptr};
  QAction* m_zoomInAction {nullptr};
  QAction* m_zoomOutAction {nullptr};
  QAction* m_resetZoomAction {nullptr};
  QAction* m_fitToScreenAction {nullptr};
  QAction* m_resetWorkspaceAction {nullptr};
  std::map<core::ToolKind, QAction*> m_toolActions;
  QByteArray m_defaultDockState;
  int m_lastCanvasWidth {800};
  int m_lastCanvasHeight {600};
};

} // namespace app::mainwindow
