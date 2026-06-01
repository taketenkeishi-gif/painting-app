#pragma once

#include <map>
#include <vector>

#include <QMainWindow>
#include <QColor>
#include <QByteArray>
#include <QString>
#include <QStringList>

#include "core/color/Color.h"
#include "core/tools/ToolType.h"

class QAction;
class QLabel;
class QMenu;
class QKeySequence;
class QDockWidget;
class QPushButton;
class QGridLayout;
class QSpinBox;
class QSplitter;
class QSlider;
class QTabWidget;
class QToolBar;
class QWidget;
class QResizeEvent;

namespace app::bridge {
class AppController;
}
namespace app::canvasview {
class CanvasWidget;
}
namespace app::panels {
class AiPanel;
class LayerPanel;
class SubToolPanel;
class ToolPanel;
class ToolPropertyPanel;
class ColorWheelWidget;
}

namespace app::mainwindow {

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);

protected:
  void resizeEvent(QResizeEvent* event) override;

private slots:
  void onNewCanvas();
  void onToolStateChanged();
  void onOpenTriggered();
  void onNewFromClipboardTriggered();
  void onImportAsLayerTriggered();
  void onSaveTriggered();
  void onSaveAsTriggered();
  void onExportPngTriggered();
  void onExportFlattenedTriggered();
  void onUndoTriggered();
  void onRedoTriggered();
  void onCutTriggered();
  void onCopyTriggered();
  void onPasteTriggered();
  void onDeletePixelsTriggered();
  void onFillTriggered();
  void onSetToolTriggered();
  void onSelectAllTriggered();
  void onDeselectTriggered();
  void onClearSelectionTriggered();
  void onInvertSelectionTriggered();
  void onToggleLayerVisibilityTriggered();
  void onMoveLayerUpTriggered();
  void onMoveLayerDownTriggered();
  void onAddRasterLayerTriggered();
  void onAddVectorLayerTriggered();
  void onAddFolderLayerTriggered();
  void onDuplicateLayerTriggered();
  void onDeleteLayerTriggered();
  void onMergeDownTriggered();
  void onRasterizeLayerTriggered();
  void onToggleLayerClipTriggered();
  void onToggleLayerMaskTriggered();
  void onRemoveLayerMaskTriggered();
  void onToggleLayerLockTriggered();
  void onToggleLayerAlphaLockTriggered();
  void onToggleLayerPositionLockTriggered();
  void onDecreaseBrushSizeTriggered();
  void onIncreaseBrushSizeTriggered();
  void onZoomInTriggered();
  void onZoomOutTriggered();
  void onResetZoomTriggered();
  void onFitToScreenTriggered();
  void onResetWorkspaceTriggered();
  void onSaveWorkspaceTriggered();
  void onDeleteWorkspaceTriggered();
  void onRestoreLastWorkspaceTriggered();
  void onLoadWorkspaceByName(const QString& name);
  void onShortcutSettingsTriggered();
  void onCommandPaletteTriggered();
  void onChooseForegroundColor();
  void onChooseBackgroundColor();
  void onSwapColors();
  void onResetBlackWhiteColors();
  void onUseTransparentColor();
  void onGenerativeFillTriggered();
  void onConnectComfyUiTriggered();
  void onComfyUiStateChanged(bool connected);
  void onBrightnessContrastTriggered();
  void onHueSatLightTriggered();

private:
  void setupShellLayout();
  void createMenus();
  void createToolBar();
  void applyUiChrome();
  void adjustRightDockLayout();
  void updateUndoRedoState();
  void updateActiveLayerStatus();
  void updateTopToolInfo();
  void updateColorPanel();
  void updateToolActionState();
  void updateNavigatorPreview();
  void pushForegroundColorHistory(const core::Color& color);
  void syncForegroundHsvControlsFromColor(const core::Color& color);
  void syncColorUiFromForeground(const QColor& color);
  void applyForegroundColor(const QColor& color, bool pushHistory = true);
  void applyForegroundFromHsvControls();
  void refreshColorHistoryButtons();
  void relayoutColorHistoryGrid();
  bool openImageFile(const QString& path);
  bool saveImageFile(const QString& path);
  void pushRecentFile(const QString& path);
  void rebuildRecentFilesMenu();
  void rebuildWorkspaceLayoutsMenu();
  void loadWorkspaceLayoutState();
  void saveWorkspaceLayout(const QString& name);
  bool restoreWorkspaceLayout(const QString& name);
  QStringList workspaceLayoutNames() const;
  void loadShortcutOverrides();
  void saveShortcutOverride(const QString& commandId, const QKeySequence& sequence);
  QAction* createToolAction(QMenu* toolMenu, core::ToolKind kind, const QString& text, const QKeySequence& shortcut);

  app::bridge::AppController* m_controller {nullptr};
  app::canvasview::CanvasWidget* m_canvasWidget {nullptr};
  app::panels::AiPanel*    m_aiPanel    {nullptr};
  app::panels::LayerPanel* m_layerPanel {nullptr};
  app::panels::ToolPanel* m_toolPanel {nullptr};
  app::panels::ToolPanel* m_quickSliderPanel {nullptr};
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
  QDockWidget* m_toolSliderDock {nullptr};
  QDockWidget* m_subToolDock {nullptr};
  QDockWidget* m_toolPropertyDock {nullptr};
  QDockWidget* m_colorDock {nullptr};
  QDockWidget* m_aiDock    {nullptr};
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
  QLabel* m_navigatorImageLabel {nullptr};
  QPushButton* m_foregroundColorButton {nullptr};
  QPushButton* m_backgroundColorButton {nullptr};
  QWidget* m_colorPanelWidget {nullptr};
  QSlider* m_hueSlider {nullptr};
  QSlider* m_satSlider {nullptr};
  QSlider* m_valSlider {nullptr};
  QSlider* m_alphaSlider {nullptr};
  QSpinBox* m_hueSpin {nullptr};
  QSpinBox* m_satSpin {nullptr};
  QSpinBox* m_valSpin {nullptr};
  QSpinBox* m_alphaSpin {nullptr};
  app::panels::ColorWheelWidget* m_colorWheelWidget {nullptr};
  std::vector<QPushButton*> m_colorHistoryButtons;
  QWidget* m_colorHistoryGridWidget {nullptr};
  QGridLayout* m_colorHistoryLayout {nullptr};
  int m_colorHistoryColumnCount {0};
  std::vector<core::Color> m_colorHistory;
  core::Color m_backgroundColor {255, 255, 255, 255};
  core::Color m_lastForegroundColor {0, 0, 0, 255};
  bool m_updatingColorControls {false};
  QAction* m_newCanvasAction {nullptr};
  QAction* m_openAction {nullptr};
  QAction* m_newFromClipboardAction {nullptr};
  QAction* m_importAsLayerAction {nullptr};
  QAction* m_saveAction {nullptr};
  QAction* m_saveAsAction {nullptr};
  QAction* m_exportPngAction {nullptr};
  QAction* m_exportFlattenedAction {nullptr};
  QAction* m_exitAction {nullptr};
  QAction* m_undoAction {nullptr};
  QAction* m_redoAction {nullptr};
  QAction* m_cutAction {nullptr};
  QAction* m_copyAction {nullptr};
  QAction* m_pasteAction {nullptr};
  QAction* m_deletePixelsAction {nullptr};
  QAction* m_fillAction {nullptr};
  QAction* m_clearAction {nullptr};
  QAction* m_addLayerAction {nullptr};
  QAction* m_addRasterLayerAction {nullptr};
  QAction* m_addVectorLayerAction {nullptr};
  QAction* m_addFolderLayerAction {nullptr};
  QAction* m_duplicateLayerAction {nullptr};
  QAction* m_deleteLayerAction {nullptr};
  QAction* m_moveLayerUpAction {nullptr};
  QAction* m_moveLayerDownAction {nullptr};
  QAction* m_toggleLayerVisibilityAction {nullptr};
  QAction* m_clearSelectionAction {nullptr};
  QAction* m_selectAllAction {nullptr};
  QAction* m_deselectAction {nullptr};
  QAction* m_invertSelectionAction {nullptr};
  QAction* m_brushSizeDownAction {nullptr};
  QAction* m_brushSizeUpAction {nullptr};
  QAction* m_zoomInAction {nullptr};
  QAction* m_zoomOutAction {nullptr};
  QAction* m_resetZoomAction {nullptr};
  QAction* m_fitToScreenAction {nullptr};
  QAction* m_toggleGridAction {nullptr};
  QAction* m_toggleOverlayAction {nullptr};
  QAction* m_resetWorkspaceAction {nullptr};
  QAction* m_saveWorkspaceAction {nullptr};
  QAction* m_deleteWorkspaceAction {nullptr};
  QAction* m_restoreLastWorkspaceAction {nullptr};
  QAction* m_mergeDownAction {nullptr};
  QAction* m_rasterizeLayerAction {nullptr};
  QAction* m_toggleLayerClipAction {nullptr};
  QAction* m_toggleLayerMaskAction {nullptr};
  QAction* m_removeLayerMaskAction {nullptr};
  QAction* m_toggleLayerLockAction {nullptr};
  QAction* m_toggleLayerAlphaLockAction {nullptr};
  QAction* m_toggleLayerPositionLockAction {nullptr};
  QAction* m_shortcutSummaryAction {nullptr};
  QAction* m_openDocsAction {nullptr};
  QAction* m_shortcutSettingsAction {nullptr};
  QAction* m_commandPaletteAction {nullptr};
  QAction* m_swapColorsAction {nullptr};
  QAction* m_resetColorsAction {nullptr};
  QAction* m_transparentColorAction {nullptr};
  QAction* m_generativeFillAction   {nullptr};
  QAction* m_connectComfyUiAction   {nullptr};
  // 画像調整
  QAction* m_brightnessContrastAction {nullptr};
  QAction* m_hueSatLightAction        {nullptr};
  QLabel*  m_comfyUiStatusLabel    {nullptr};
  QAction* m_clearRecentFilesAction {nullptr};
  QMenu* m_recentFilesMenu {nullptr};
  QMenu* m_workspaceLayoutsMenu {nullptr};
  std::map<core::ToolKind, QAction*> m_toolActions;
  QByteArray m_defaultDockState;
  QString m_currentFilePath;
  QStringList m_recentFiles;
  int m_lastCanvasWidth {800};
  int m_lastCanvasHeight {600};
};

} // namespace app::mainwindow
