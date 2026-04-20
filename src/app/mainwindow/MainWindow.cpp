#include "app/mainwindow/MainWindow.h"

#include <cstdint>

#include <QAction>
#include <QActionGroup>
#include <QClipboard>
#include <QColorDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGuiApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSplitter>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include "app/bridge/AppController.h"
#include "app/canvasview/CanvasWidget.h"
#include "app/panels/LayerPanel.h"
#include "app/panels/SubToolPanel.h"
#include "app/panels/ToolPanel.h"
#include "app/panels/ToolPropertyPanel.h"
#include "platform/qt/QtImageConverter.h"

namespace app::mainwindow {

namespace {

QColor toQColor(const core::Color& color) {
  return QColor(color.r, color.g, color.b, color.a);
}

core::Color toCoreColor(const QColor& color) {
  return core::Color {
      static_cast<std::uint8_t>(color.red()),
      static_cast<std::uint8_t>(color.green()),
      static_cast<std::uint8_t>(color.blue()),
      static_cast<std::uint8_t>(color.alpha())};
}

QGroupBox* makePanelGroup(const QString& title, QWidget* child, QWidget* parent) {
  auto* box = new QGroupBox(title, parent);
  auto* layout = new QVBoxLayout(box);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->addWidget(child);
  return box;
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_controller(new app::bridge::AppController(this)),
      m_canvasWidget(new app::canvasview::CanvasWidget(this)),
      m_layerPanel(new app::panels::LayerPanel(this)),
      m_toolPanel(new app::panels::ToolPanel(this)),
      m_subToolPanel(new app::panels::SubToolPanel(this)),
      m_toolPropertyPanel(new app::panels::ToolPropertyPanel(this)) {
  setWindowTitle("Layered Paint App");
  resize(1400, 860);

  m_canvasWidget->setController(m_controller);
  m_layerPanel->setController(m_controller);
  m_toolPanel->setController(m_controller);
  m_subToolPanel->setController(m_controller);
  m_toolPropertyPanel->setController(m_controller);

  setupShellLayout();
  createMenus();
  createToolBar();
  applyUiChrome();

  connect(m_controller, &app::bridge::AppController::toolStateChanged, this, &MainWindow::onToolStateChanged);
  connect(m_controller, &app::bridge::AppController::layersChanged, this, &MainWindow::updateActiveLayerStatus);
  connect(m_controller, &app::bridge::AppController::documentChanged, this, &MainWindow::updateActiveLayerStatus);
  connect(m_controller, &app::bridge::AppController::documentChanged, this, &MainWindow::updateUndoRedoState);
  connect(m_controller, &app::bridge::AppController::layersChanged, this, &MainWindow::updateUndoRedoState);

  onToolStateChanged();
  updateUndoRedoState();
  updateActiveLayerStatus();
  updateTopToolInfo();
  updateColorPanel();
}

void MainWindow::setupShellLayout() {
  setCentralWidget(m_canvasWidget);
  setDockNestingEnabled(true);

  auto* colorPanel = new QWidget(this);
  auto* colorLayout = new QVBoxLayout(colorPanel);
  colorLayout->setContentsMargins(8, 8, 8, 8);
  colorLayout->setSpacing(6);
  auto* colorTitle = new QLabel("Color", colorPanel);
  colorTitle->setStyleSheet("font-weight: 700;");
  m_foregroundColorButton = new QPushButton("FG", colorPanel);
  m_backgroundColorButton = new QPushButton("BG", colorPanel);
  auto* swapColorButton = new QPushButton("Swap", colorPanel);
  auto* resetColorButton = new QPushButton("Reset B/W", colorPanel);
  auto* colorButtons = new QHBoxLayout();
  colorButtons->setContentsMargins(0, 0, 0, 0);
  colorButtons->setSpacing(6);
  colorButtons->addWidget(m_foregroundColorButton);
  colorButtons->addWidget(m_backgroundColorButton);
  colorLayout->addWidget(colorTitle);
  colorLayout->addLayout(colorButtons);
  colorLayout->addWidget(swapColorButton);
  colorLayout->addWidget(resetColorButton);
  colorLayout->addStretch(1);
  connect(m_foregroundColorButton, &QPushButton::clicked, this, &MainWindow::onChooseForegroundColor);
  connect(m_backgroundColorButton, &QPushButton::clicked, this, &MainWindow::onChooseBackgroundColor);
  connect(swapColorButton, &QPushButton::clicked, this, &MainWindow::onSwapColors);
  connect(resetColorButton, &QPushButton::clicked, this, [this]() {
    m_controller->setBrushColor(core::Color::OpaqueBlack());
    m_backgroundColor = core::Color {255, 255, 255, 255};
    updateColorPanel();
  });

  auto* infoPanel = new QWidget(this);
  auto* infoLayout = new QVBoxLayout(infoPanel);
  infoLayout->setContentsMargins(8, 8, 8, 8);
  infoLayout->setSpacing(6);
  auto* infoTitle = new QLabel("Info", infoPanel);
  infoTitle->setStyleSheet("font-weight: 700;");
  auto* infoText = new QLabel("Tool, sub tool, and layer constraints are shown in status bar.", infoPanel);
  infoText->setWordWrap(true);
  infoLayout->addWidget(infoTitle);
  infoLayout->addWidget(infoText);
  infoLayout->addStretch(1);

  auto makeDock = [this](const QString& title, QWidget* widget, const char* name) {
    auto* dock = new QDockWidget(title, this);
    dock->setObjectName(name);
    dock->setWidget(widget);
    dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    return dock;
  };

  m_toolDock = makeDock("Tools", m_toolPanel, "ToolDock");
  m_subToolDock = makeDock("Sub Tool", m_subToolPanel, "SubToolDock");
  m_toolPropertyDock = makeDock("Tool Property", m_toolPropertyPanel, "ToolPropertyDock");
  m_colorDock = makeDock("Color", colorPanel, "ColorDock");
  m_layerDock = makeDock("Layer", m_layerPanel, "LayerDock");
  m_infoDock = makeDock("Info", infoPanel, "InfoDock");

  addDockWidget(Qt::LeftDockWidgetArea, m_toolDock);
  splitDockWidget(m_toolDock, m_subToolDock, Qt::Vertical);
  splitDockWidget(m_subToolDock, m_toolPropertyDock, Qt::Vertical);
  splitDockWidget(m_toolPropertyDock, m_colorDock, Qt::Vertical);

  addDockWidget(Qt::RightDockWidgetArea, m_layerDock);
  splitDockWidget(m_layerDock, m_infoDock, Qt::Vertical);

  m_toolDock->raise();
  m_layerDock->raise();
  m_defaultDockState = saveState();
}

void MainWindow::createMenus() {
  auto* fileMenu = menuBar()->addMenu("&File");
  auto* editMenu = menuBar()->addMenu("&Edit");
  auto* toolMenu = menuBar()->addMenu("&Tool");
  auto* selectMenu = menuBar()->addMenu("&Select");
  auto* layerMenu = menuBar()->addMenu("&Layer");
  auto* viewMenu = menuBar()->addMenu("&View");
  auto* windowMenu = menuBar()->addMenu("&Window");
  auto* helpMenu = menuBar()->addMenu("&Help");

  m_newCanvasAction = new QAction("&New Canvas", this);
  m_openAction = new QAction("&Open...", this);
  m_saveAction = new QAction("&Save", this);
  m_saveAsAction = new QAction("Save &As...", this);
  m_exportPngAction = new QAction("Export &PNG...", this);
  auto* closeAction = new QAction("&Close", this);
  m_undoAction = new QAction("&Undo", this);
  m_redoAction = new QAction("&Redo", this);
  m_cutAction = new QAction("Cu&t", this);
  m_copyAction = new QAction("&Copy", this);
  m_pasteAction = new QAction("&Paste", this);
  m_deletePixelsAction = new QAction("&Delete Pixels", this);
  m_fillAction = new QAction("&Fill", this);
  m_addLayerAction = new QAction("&New Raster Layer", this);
  m_addRasterLayerAction = m_addLayerAction;
  m_addVectorLayerAction = new QAction("New &Vector Layer", this);
  m_duplicateLayerAction = new QAction("&Duplicate Layer", this);
  m_deleteLayerAction = new QAction("&Delete Layer", this);
  m_moveLayerUpAction = new QAction("Move Layer &Up", this);
  m_moveLayerDownAction = new QAction("Move Layer &Down", this);
  m_toggleLayerVisibilityAction = new QAction("&Toggle Visibility", this);
  m_mergeDownAction = new QAction("&Merge Down", this);
  m_rasterizeLayerAction = new QAction("&Rasterize Layer", this);

  m_selectAllAction = new QAction("Select &All", this);
  m_deselectAction = new QAction("&Deselect", this);
  m_clearSelectionAction = new QAction("&Clear Selection", this);
  m_invertSelectionAction = new QAction("&Invert Selection", this);
  m_brushSizeDownAction = new QAction("Brush Size &Down", this);
  m_brushSizeUpAction = new QAction("Brush Size &Up", this);
  m_zoomInAction = new QAction("Zoom &In", this);
  m_zoomOutAction = new QAction("Zoom &Out", this);
  m_resetZoomAction = new QAction("&Reset Zoom", this);
  m_fitToScreenAction = new QAction("&Fit To Screen", this);
  m_resetWorkspaceAction = new QAction("&Reset Workspace", this);
  auto* aboutAction = new QAction("&About", this);

  m_recentFilesMenu = fileMenu->addMenu("Recent Files");

  m_newCanvasAction->setShortcut(QKeySequence::New);
  m_openAction->setShortcut(QKeySequence::Open);
  m_saveAction->setShortcut(QKeySequence::Save);
  m_saveAsAction->setShortcut(QKeySequence::SaveAs);
  m_exportPngAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E));
  closeAction->setShortcut(QKeySequence::Close);
  m_undoAction->setShortcut(QKeySequence::Undo);
  m_redoAction->setShortcuts({QKeySequence::Redo, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z)});
  m_cutAction->setShortcut(QKeySequence::Cut);
  m_copyAction->setShortcut(QKeySequence::Copy);
  m_pasteAction->setShortcut(QKeySequence::Paste);
  m_deletePixelsAction->setShortcut(QKeySequence(Qt::Key_Delete));
  m_fillAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Backspace));
  m_addLayerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
  m_addVectorLayerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_N));
  m_duplicateLayerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
  m_deleteLayerAction->setShortcut(QKeySequence::Delete);
  m_moveLayerUpAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Up));
  m_moveLayerDownAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Down));
  m_toggleLayerVisibilityAction->setShortcut(QKeySequence(Qt::Key_V));
  m_mergeDownAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
  m_rasterizeLayerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
  m_selectAllAction->setShortcut(QKeySequence::SelectAll);
  m_deselectAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
  m_clearSelectionAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
  m_invertSelectionAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I));
  m_brushSizeDownAction->setShortcut(QKeySequence(Qt::Key_BracketLeft));
  m_brushSizeUpAction->setShortcut(QKeySequence(Qt::Key_BracketRight));
  m_zoomInAction->setShortcut(QKeySequence::ZoomIn);
  m_zoomOutAction->setShortcut(QKeySequence::ZoomOut);
  m_resetZoomAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
  m_fitToScreenAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_9));
  m_resetWorkspaceAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W));

  fileMenu->addAction(m_newCanvasAction);
  fileMenu->addAction(m_openAction);
  fileMenu->addAction(m_saveAction);
  fileMenu->addAction(m_saveAsAction);
  fileMenu->addSeparator();
  fileMenu->addAction(m_exportPngAction);
  if (m_recentFilesMenu != nullptr) {
    rebuildRecentFilesMenu();
    fileMenu->addMenu(m_recentFilesMenu);
  }
  fileMenu->addSeparator();
  fileMenu->addAction(closeAction);
  editMenu->addAction(m_undoAction);
  editMenu->addAction(m_redoAction);
  editMenu->addSeparator();
  editMenu->addAction(m_cutAction);
  editMenu->addAction(m_copyAction);
  editMenu->addAction(m_pasteAction);
  editMenu->addAction(m_deletePixelsAction);
  editMenu->addAction(m_fillAction);
  editMenu->addSeparator();
  editMenu->addAction(m_brushSizeDownAction);
  editMenu->addAction(m_brushSizeUpAction);

  auto* toolGroup = new QActionGroup(this);
  toolGroup->setExclusive(true);
  auto bindTool = [&](core::ToolKind kind, const QString& text, const QKeySequence& shortcut) {
    QAction* action = createToolAction(toolMenu, kind, text, shortcut);
    action->setActionGroup(toolGroup);
  };
  bindTool(core::ToolKind::Brush, "&Brush", QKeySequence(Qt::Key_B));
  bindTool(core::ToolKind::Eraser, "&Eraser", QKeySequence(Qt::Key_E));
  bindTool(core::ToolKind::Eyedropper, "&Eyedropper", QKeySequence(Qt::Key_I));
  bindTool(core::ToolKind::Fill, "&Fill", QKeySequence(Qt::Key_G));
  bindTool(core::ToolKind::Line, "&Line", QKeySequence(Qt::Key_U));
  bindTool(core::ToolKind::RectSelection, "&Rect Selection", QKeySequence(Qt::Key_R));
  bindTool(core::ToolKind::MoveLayer, "&Move Layer", QKeySequence(Qt::Key_M));
  bindTool(core::ToolKind::Hand, "&Hand", QKeySequence(Qt::Key_H));
  bindTool(core::ToolKind::Zoom, "&Zoom", QKeySequence(Qt::Key_Z));

  selectMenu->addAction(m_clearSelectionAction);
  selectMenu->addAction(m_selectAllAction);
  selectMenu->addAction(m_deselectAction);
  selectMenu->addAction(m_invertSelectionAction);

  layerMenu->addAction(m_addRasterLayerAction);
  layerMenu->addAction(m_addVectorLayerAction);
  layerMenu->addAction(m_duplicateLayerAction);
  layerMenu->addAction(m_deleteLayerAction);
  layerMenu->addAction(m_mergeDownAction);
  layerMenu->addAction(m_rasterizeLayerAction);
  layerMenu->addSeparator();
  layerMenu->addAction(m_moveLayerUpAction);
  layerMenu->addAction(m_moveLayerDownAction);
  layerMenu->addAction(m_toggleLayerVisibilityAction);

  viewMenu->addAction(m_zoomInAction);
  viewMenu->addAction(m_zoomOutAction);
  viewMenu->addAction(m_resetZoomAction);
  viewMenu->addAction(m_fitToScreenAction);

  if (m_toolDock != nullptr) {
    windowMenu->addAction(m_toolDock->toggleViewAction());
  }
  if (m_subToolDock != nullptr) {
    windowMenu->addAction(m_subToolDock->toggleViewAction());
  }
  if (m_toolPropertyDock != nullptr) {
    windowMenu->addAction(m_toolPropertyDock->toggleViewAction());
  }
  if (m_colorDock != nullptr) {
    windowMenu->addAction(m_colorDock->toggleViewAction());
  }
  if (m_layerDock != nullptr) {
    windowMenu->addAction(m_layerDock->toggleViewAction());
  }
  if (m_infoDock != nullptr) {
    windowMenu->addAction(m_infoDock->toggleViewAction());
  }
  windowMenu->addSeparator();
  windowMenu->addAction(m_resetWorkspaceAction);

  helpMenu->addAction(aboutAction);

  connect(m_newCanvasAction, &QAction::triggered, this, &MainWindow::onNewCanvas);
  connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpenTriggered);
  connect(m_saveAction, &QAction::triggered, this, &MainWindow::onSaveTriggered);
  connect(m_saveAsAction, &QAction::triggered, this, &MainWindow::onSaveAsTriggered);
  connect(m_exportPngAction, &QAction::triggered, this, &MainWindow::onExportPngTriggered);
  connect(closeAction, &QAction::triggered, this, &QWidget::close);
  connect(m_undoAction, &QAction::triggered, this, &MainWindow::onUndoTriggered);
  connect(m_redoAction, &QAction::triggered, this, &MainWindow::onRedoTriggered);
  connect(m_cutAction, &QAction::triggered, this, &MainWindow::onCutTriggered);
  connect(m_copyAction, &QAction::triggered, this, &MainWindow::onCopyTriggered);
  connect(m_pasteAction, &QAction::triggered, this, &MainWindow::onPasteTriggered);
  connect(m_deletePixelsAction, &QAction::triggered, this, &MainWindow::onDeletePixelsTriggered);
  connect(m_fillAction, &QAction::triggered, this, &MainWindow::onFillTriggered);
  connect(m_clearSelectionAction, &QAction::triggered, this, &MainWindow::onClearSelectionTriggered);
  connect(m_selectAllAction, &QAction::triggered, this, &MainWindow::onSelectAllTriggered);
  connect(m_deselectAction, &QAction::triggered, this, &MainWindow::onDeselectTriggered);
  connect(m_invertSelectionAction, &QAction::triggered, this, &MainWindow::onInvertSelectionTriggered);
  connect(m_toggleLayerVisibilityAction, &QAction::triggered, this, &MainWindow::onToggleLayerVisibilityTriggered);
  connect(m_moveLayerUpAction, &QAction::triggered, this, &MainWindow::onMoveLayerUpTriggered);
  connect(m_moveLayerDownAction, &QAction::triggered, this, &MainWindow::onMoveLayerDownTriggered);
  connect(m_brushSizeDownAction, &QAction::triggered, this, &MainWindow::onDecreaseBrushSizeTriggered);
  connect(m_brushSizeUpAction, &QAction::triggered, this, &MainWindow::onIncreaseBrushSizeTriggered);
  connect(m_addRasterLayerAction, &QAction::triggered, this, &MainWindow::onAddRasterLayerTriggered);
  connect(m_addVectorLayerAction, &QAction::triggered, this, &MainWindow::onAddVectorLayerTriggered);
  connect(m_duplicateLayerAction, &QAction::triggered, this, &MainWindow::onDuplicateLayerTriggered);
  connect(m_deleteLayerAction, &QAction::triggered, this, &MainWindow::onDeleteLayerTriggered);
  connect(m_mergeDownAction, &QAction::triggered, this, &MainWindow::onMergeDownTriggered);
  connect(m_rasterizeLayerAction, &QAction::triggered, this, &MainWindow::onRasterizeLayerTriggered);
  connect(m_zoomInAction, &QAction::triggered, this, &MainWindow::onZoomInTriggered);
  connect(m_zoomOutAction, &QAction::triggered, this, &MainWindow::onZoomOutTriggered);
  connect(m_resetZoomAction, &QAction::triggered, this, &MainWindow::onResetZoomTriggered);
  connect(m_fitToScreenAction, &QAction::triggered, this, &MainWindow::onFitToScreenTriggered);
  connect(m_resetWorkspaceAction, &QAction::triggered, this, &MainWindow::onResetWorkspaceTriggered);
  connect(aboutAction, &QAction::triggered, this, [this]() {
    statusBar()->showMessage("Layered Paint App - UI shell + raster/vector base", 4000);
  });

  m_toolStatusLabel = new QLabel("Tool: Brush", this);
  m_toolStatusLabel->setObjectName("ToolStatusLabel");
  m_subToolStatusLabel = new QLabel("Sub: Normal", this);
  m_subToolStatusLabel->setObjectName("SubToolStatusLabel");
  m_guideStatusLabel = new QLabel("Guide: LMB drag to paint. Wheel adjusts size.", this);
  m_guideStatusLabel->setObjectName("ToolGuideStatusLabel");
  m_colorStatusLabel = new QLabel("Color: #000000", this);
  m_colorStatusLabel->setObjectName("BrushColorStatusLabel");
  m_sizeStatusLabel = new QLabel("Size: 8", this);
  m_sizeStatusLabel->setObjectName("BrushSizeStatusLabel");
  m_activeLayerStatusLabel = new QLabel("Layer: Layer 1", this);
  m_activeLayerStatusLabel->setObjectName("ActiveLayerStatusLabel");
  m_zoomStatusLabel = new QLabel("Zoom: 100%", this);
  m_zoomStatusLabel->setObjectName("ZoomStatusLabel");
  m_selectionStatusLabel = new QLabel("Selection: Off", this);
  m_selectionStatusLabel->setObjectName("SelectionStatusLabel");

  statusBar()->addWidget(m_toolStatusLabel);
  statusBar()->addWidget(m_subToolStatusLabel);
  statusBar()->addWidget(m_guideStatusLabel, 1);
  statusBar()->addPermanentWidget(m_colorStatusLabel);
  statusBar()->addPermanentWidget(m_sizeStatusLabel);
  statusBar()->addPermanentWidget(m_zoomStatusLabel);
  statusBar()->addPermanentWidget(m_selectionStatusLabel);
  statusBar()->addPermanentWidget(m_activeLayerStatusLabel);
}

void MainWindow::createToolBar() {
  m_quickToolBar = addToolBar("Quick Tools");
  m_quickToolBar->setMovable(false);
  m_quickToolBar->setIconSize(QSize(18, 18));

  m_newCanvasAction->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
  m_openAction->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
  m_saveAction->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
  m_undoAction->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
  m_redoAction->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
  m_addRasterLayerAction->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
  m_addVectorLayerAction->setIcon(style()->standardIcon(QStyle::SP_DriveNetIcon));
  m_duplicateLayerAction->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
  m_deleteLayerAction->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
  m_toggleLayerVisibilityAction->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
  m_moveLayerUpAction->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
  m_moveLayerDownAction->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
  m_zoomInAction->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
  m_zoomOutAction->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));

  m_quickToolBar->addAction(m_newCanvasAction);
  m_quickToolBar->addAction(m_openAction);
  m_quickToolBar->addAction(m_saveAction);
  m_quickToolBar->addSeparator();
  m_quickToolBar->addAction(m_undoAction);
  m_quickToolBar->addAction(m_redoAction);
  m_quickToolBar->addSeparator();
  m_quickToolBar->addAction(m_addRasterLayerAction);
  m_quickToolBar->addAction(m_addVectorLayerAction);
  m_quickToolBar->addAction(m_duplicateLayerAction);
  m_quickToolBar->addAction(m_deleteLayerAction);
  m_quickToolBar->addAction(m_moveLayerUpAction);
  m_quickToolBar->addAction(m_moveLayerDownAction);
  m_quickToolBar->addAction(m_toggleLayerVisibilityAction);
  m_quickToolBar->addSeparator();
  m_quickToolBar->addAction(m_zoomInAction);
  m_quickToolBar->addAction(m_zoomOutAction);
  m_quickToolBar->addAction(m_resetZoomAction);
}

void MainWindow::applyUiChrome() {
  setStyleSheet(
      "QMainWindow { background: #1f2227; color: #e8e8e8; }"
      "QDockWidget { color: #d9dde5; }"
      "QDockWidget::title { background: #2b3038; border: 1px solid #3a414d; padding: 4px 8px; }"
      "QDockWidget > QWidget { background: #262a31; }"
      "QGroupBox { border: 1px solid #3f4652; border-radius: 4px; margin-top: 12px; padding-top: 10px; }"
      "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; color: #d9dde5; font-weight: 700; }"
      "QListWidget { background: #1f2328; border: 1px solid #353b44; color: #dfe5ef; }"
      "QListWidget::item:selected { background: #32507a; color: #ffffff; }"
      "QPushButton, QToolButton { background: #2d323a; border: 1px solid #4b5464; border-radius: 3px; padding: 4px 8px; color: #e2e6ee; }"
      "QPushButton:hover { background: #39404a; }"
      "QMenuBar, QToolBar { background: #2a2f36; color: #e2e6ee; }"
      "QLineEdit, QSpinBox, QComboBox { background: #1e2228; border: 1px solid #3e4654; color: #e6ebf3; }"
      "QSlider::groove:horizontal { background: #20252d; height: 6px; border-radius: 3px; }"
      "QSlider::handle:horizontal { background: #6f92c2; width: 12px; border-radius: 6px; margin: -3px 0; }"
      "QStatusBar { background: #2a2f36; border-top: 1px solid #3a3f48; }");
}

void MainWindow::updateUndoRedoState() {
  if (m_undoAction == nullptr || m_redoAction == nullptr) {
    return;
  }

  const bool canUndo = m_controller->canUndo();
  const bool canRedo = m_controller->canRedo();

  if (canUndo) {
    const QString label = QString::fromStdString(m_controller->nextUndoActionName());
    m_undoAction->setText(label.isEmpty() ? "&Undo" : QString("&Undo %1").arg(label));
  } else {
    m_undoAction->setText("&Undo");
  }

  if (canRedo) {
    const QString label = QString::fromStdString(m_controller->nextRedoActionName());
    m_redoAction->setText(label.isEmpty() ? "&Redo" : QString("&Redo %1").arg(label));
  } else {
    m_redoAction->setText("&Redo");
  }

  m_undoAction->setEnabled(canUndo);
  m_redoAction->setEnabled(canRedo);
}

void MainWindow::updateActiveLayerStatus() {
  if (m_activeLayerStatusLabel == nullptr) {
    return;
  }

  const core::Document& doc = m_controller->document();
  if (doc.layerCount() == 0) {
    m_activeLayerStatusLabel->setText("Layer: (none)");
    return;
  }
  const std::size_t active = doc.activeLayerIndex();
  const core::Layer& layer = doc.layerAt(active);
  const QString kind = layer.kind() == core::LayerKind::Vector ? "Vector" : "Raster";
  m_activeLayerStatusLabel->setText(QString("Layer: %1 (%2)").arg(QString::fromStdString(layer.name()), kind));
  if (m_selectionStatusLabel != nullptr) {
    m_selectionStatusLabel->setText(QString("Selection: %1").arg(doc.selection().hasSelection() ? "On" : "Off"));
  }
}

void MainWindow::updateTopToolInfo() {
  if (m_currentToolLabel == nullptr || m_currentSubToolLabel == nullptr) {
    return;
  }

  m_currentToolLabel->setText(QString("Tool: %1").arg(QString::fromStdString(m_controller->currentToolDisplayName())));
  m_currentSubToolLabel->setText(QString("Sub Tool: %1").arg(QString::fromStdString(m_controller->currentSubToolDisplayName())));
  updateToolActionState();
}

void MainWindow::onUndoTriggered() {
  m_controller->undo();
  updateUndoRedoState();
}

void MainWindow::onRedoTriggered() {
  m_controller->redo();
  updateUndoRedoState();
}

void MainWindow::onNewCanvas() {
  QDialog dialog(this);
  dialog.setWindowTitle("New Canvas");
  auto* form = new QFormLayout(&dialog);
  auto* widthSpin = new QSpinBox(&dialog);
  auto* heightSpin = new QSpinBox(&dialog);
  widthSpin->setRange(1, 8192);
  heightSpin->setRange(1, 8192);
  widthSpin->setValue(m_lastCanvasWidth);
  heightSpin->setValue(m_lastCanvasHeight);
  form->addRow("Width", widthSpin);
  form->addRow("Height", heightSpin);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  form->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  const int width = widthSpin->value();
  const int height = heightSpin->value();
  m_lastCanvasWidth = width;
  m_lastCanvasHeight = height;
  m_controller->newDocument(width, height);
}

void MainWindow::onToolStateChanged() {
  const app::bridge::ToolStateViewModel state = m_controller->toolState();

  if (m_sizeStatusLabel != nullptr) {
    m_sizeStatusLabel->setText(QString("Size: %1").arg(state.size));
  }

  if (m_colorStatusLabel != nullptr) {
    const QColor color(state.color.r, state.color.g, state.color.b, state.color.a);
    m_colorStatusLabel->setText(QString("Color: %1").arg(color.name(QColor::HexRgb).toUpper()));
  }

  if (m_toolStatusLabel != nullptr) {
    m_toolStatusLabel->setText(QString("Tool: %1").arg(QString::fromStdString(m_controller->currentToolDisplayName())));
  }
  if (m_subToolStatusLabel != nullptr) {
    m_subToolStatusLabel->setText(QString("Sub: %1").arg(QString::fromStdString(m_controller->currentSubToolDisplayName())));
  }

  if (m_guideStatusLabel != nullptr) {
    m_guideStatusLabel->setText(QString("Guide: %1").arg(QString::fromStdString(m_controller->currentToolGuide())));
  }

  updateColorPanel();
  updateTopToolInfo();
}

void MainWindow::onOpenTriggered() {
  const QString path = QFileDialog::getOpenFileName(
      this,
      "Open Image",
      m_currentFilePath.isEmpty() ? QString() : QFileInfo(m_currentFilePath).absolutePath(),
      "Image Files (*.png *.jpg *.jpeg *.bmp)");
  if (path.isEmpty()) {
    return;
  }
  openImageFile(path);
}

void MainWindow::onSaveTriggered() {
  if (m_currentFilePath.isEmpty()) {
    onSaveAsTriggered();
    return;
  }
  if (saveImageFile(m_currentFilePath)) {
    statusBar()->showMessage(QString("Saved: %1").arg(m_currentFilePath), 2500);
  }
}

void MainWindow::onSaveAsTriggered() {
  const QString path = QFileDialog::getSaveFileName(
      this,
      "Save Image",
      m_currentFilePath,
      "PNG Image (*.png);;JPEG Image (*.jpg *.jpeg);;BMP Image (*.bmp)");
  if (path.isEmpty()) {
    return;
  }
  if (saveImageFile(path)) {
    m_currentFilePath = path;
    pushRecentFile(path);
    statusBar()->showMessage(QString("Saved: %1").arg(path), 2500);
  }
}

void MainWindow::onExportPngTriggered() {
  const QString path = QFileDialog::getSaveFileName(this, "Export PNG", QString(), "PNG Image (*.png)");
  if (path.isEmpty()) {
    return;
  }
  if (saveImageFile(path)) {
    statusBar()->showMessage(QString("Exported: %1").arg(path), 2500);
  }
}

void MainWindow::onSetToolTriggered() {
  auto* action = qobject_cast<QAction*>(sender());
  if (action == nullptr || m_controller == nullptr) {
    return;
  }

  const QVariant kindValue = action->data();
  if (!kindValue.isValid()) {
    return;
  }
  m_controller->setCurrentTool(static_cast<core::ToolKind>(kindValue.toInt()));
}

void MainWindow::onCutTriggered() {
  onCopyTriggered();
  onDeletePixelsTriggered();
}

void MainWindow::onCopyTriggered() {
  const core::PixelBuffer copied = m_controller->exportSelectionOrCanvasFromComposite();
  if (copied.width() <= 0 || copied.height() <= 0) {
    return;
  }
  QGuiApplication::clipboard()->setImage(platform::qt::QtImageConverter::toQImage(copied));
  statusBar()->showMessage("Copied to clipboard", 1500);
}

void MainWindow::onPasteTriggered() {
  const QImage image = QGuiApplication::clipboard()->image();
  if (image.isNull()) {
    statusBar()->showMessage("Clipboard has no image", 1500);
    return;
  }
  const core::PixelBuffer buffer = platform::qt::QtImageConverter::fromQImage(image);
  if (m_controller->pasteBufferAsNewRasterLayer(buffer, "Pasted Layer")) {
    statusBar()->showMessage("Pasted as new raster layer", 1500);
  }
}

void MainWindow::onDeletePixelsTriggered() {
  if (m_controller->deleteSelectionPixels()) {
    updateUndoRedoState();
  }
}

void MainWindow::onFillTriggered() {
  if (m_controller->fillSelectionOrCanvas()) {
    updateUndoRedoState();
  }
}

void MainWindow::onSelectAllTriggered() {
  if (m_controller->selectAll()) {
    updateUndoRedoState();
  }
}

void MainWindow::onDeselectTriggered() {
  if (m_controller->deselect()) {
    updateUndoRedoState();
  }
}

void MainWindow::onClearSelectionTriggered() {
  if (m_controller->clearSelection()) {
    updateUndoRedoState();
  }
}

void MainWindow::onInvertSelectionTriggered() {
  if (m_controller->invertSelection()) {
    updateUndoRedoState();
  }
}

void MainWindow::onToggleLayerVisibilityTriggered() {
  if (m_controller->toggleActiveLayerVisible()) {
    updateUndoRedoState();
  }
}

void MainWindow::onMoveLayerUpTriggered() {
  if (m_controller->moveActiveLayerUp()) {
    updateUndoRedoState();
  }
}

void MainWindow::onMoveLayerDownTriggered() {
  if (m_controller->moveActiveLayerDown()) {
    updateUndoRedoState();
  }
}

void MainWindow::onAddRasterLayerTriggered() {
  m_controller->addRasterLayer();
}

void MainWindow::onAddVectorLayerTriggered() {
  m_controller->addVectorLayer();
}

void MainWindow::onDuplicateLayerTriggered() {
  m_controller->duplicateActiveLayer();
}

void MainWindow::onDeleteLayerTriggered() {
  if (m_controller->document().layerCount() == 0) {
    return;
  }
  m_controller->removeLayer(m_controller->document().activeLayerIndex());
}

void MainWindow::onMergeDownTriggered() {
  if (m_controller->mergeActiveLayerDown()) {
    updateUndoRedoState();
  }
}

void MainWindow::onRasterizeLayerTriggered() {
  if (m_controller->rasterizeActiveLayer()) {
    updateUndoRedoState();
  }
}

void MainWindow::onDecreaseBrushSizeTriggered() {
  m_controller->adjustBrushSize(-1);
}

void MainWindow::onIncreaseBrushSizeTriggered() {
  m_controller->adjustBrushSize(1);
}

void MainWindow::onZoomInTriggered() {
  m_canvasWidget->zoomIn();
}

void MainWindow::onZoomOutTriggered() {
  m_canvasWidget->zoomOut();
}

void MainWindow::onResetZoomTriggered() {
  m_canvasWidget->resetZoom();
}

void MainWindow::onFitToScreenTriggered() {
  m_canvasWidget->fitToScreen();
}

void MainWindow::onResetWorkspaceTriggered() {
  if (m_defaultDockState.isEmpty()) {
    return;
  }
  restoreState(m_defaultDockState);
}

void MainWindow::onChooseForegroundColor() {
  const QColor current = toQColor(m_controller->toolState().color);
  const QColor picked = QColorDialog::getColor(current, this, "Foreground Color", QColorDialog::ShowAlphaChannel);
  if (!picked.isValid()) {
    return;
  }
  m_controller->setBrushColor(toCoreColor(picked));
}

void MainWindow::onChooseBackgroundColor() {
  const QColor current = toQColor(m_backgroundColor);
  const QColor picked = QColorDialog::getColor(current, this, "Background Color", QColorDialog::ShowAlphaChannel);
  if (!picked.isValid()) {
    return;
  }
  m_backgroundColor = toCoreColor(picked);
  updateColorPanel();
}

void MainWindow::onSwapColors() {
  const core::Color foreground = m_controller->toolState().color;
  m_controller->setBrushColor(m_backgroundColor);
  m_backgroundColor = foreground;
  updateColorPanel();
}

void MainWindow::updateColorPanel() {
  if (m_foregroundColorButton == nullptr || m_backgroundColorButton == nullptr) {
    return;
  }

  const QColor fg = toQColor(m_controller->toolState().color);
  const QColor bg = toQColor(m_backgroundColor);
  const auto makeStyle = [](const QColor& color) {
    const int luminance = (299 * color.red() + 587 * color.green() + 114 * color.blue()) / 1000;
    const QString textColor = luminance > 128 ? "#111111" : "#f5f5f5";
    return QString("QPushButton { background-color: rgba(%1,%2,%3,%4); color:%5; border:1px solid #505866; min-height:24px; }")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(color.alpha())
        .arg(textColor);
  };

  m_foregroundColorButton->setText(QString("FG %1").arg(fg.name(QColor::HexRgb).toUpper()));
  m_backgroundColorButton->setText(QString("BG %1").arg(bg.name(QColor::HexRgb).toUpper()));
  m_foregroundColorButton->setStyleSheet(makeStyle(fg));
  m_backgroundColorButton->setStyleSheet(makeStyle(bg));
}

void MainWindow::updateToolActionState() {
  const core::ToolKind current = m_controller->currentTool();
  for (const auto& [kind, action] : m_toolActions) {
    if (action != nullptr) {
      action->setChecked(kind == current);
      action->setEnabled(m_controller->canUseToolOnActiveLayer(kind));
    }
  }
}

bool MainWindow::openImageFile(const QString& path) {
  QImage image(path);
  if (image.isNull()) {
    statusBar()->showMessage(QString("Open failed: %1").arg(path), 2500);
    return false;
  }
  const core::PixelBuffer buffer = platform::qt::QtImageConverter::fromQImage(image);
  const QFileInfo info(path);
  m_controller->importFlattenedBuffer(buffer, info.completeBaseName().toStdString());
  m_currentFilePath = path;
  pushRecentFile(path);
  statusBar()->showMessage(QString("Opened: %1").arg(path), 2500);
  return true;
}

bool MainWindow::saveImageFile(const QString& path) {
  if (path.isEmpty()) {
    return false;
  }
  const QImage image = platform::qt::QtImageConverter::toQImage(m_controller->compositedBuffer());
  const bool ok = image.save(path);
  if (!ok) {
    statusBar()->showMessage(QString("Save failed: %1").arg(path), 2500);
    return false;
  }
  return true;
}

void MainWindow::pushRecentFile(const QString& path) {
  if (path.isEmpty()) {
    return;
  }
  m_recentFiles.removeAll(path);
  m_recentFiles.prepend(path);
  while (m_recentFiles.size() > 8) {
    m_recentFiles.removeLast();
  }
  rebuildRecentFilesMenu();
}

void MainWindow::rebuildRecentFilesMenu() {
  if (m_recentFilesMenu == nullptr) {
    return;
  }
  m_recentFilesMenu->clear();
  if (m_recentFiles.isEmpty()) {
    QAction* empty = m_recentFilesMenu->addAction("(No recent files)");
    empty->setEnabled(false);
    return;
  }
  for (const QString& path : m_recentFiles) {
    QAction* action = m_recentFilesMenu->addAction(path);
    connect(action, &QAction::triggered, this, [this, path]() {
      openImageFile(path);
    });
  }
}

QAction* MainWindow::createToolAction(QMenu* toolMenu, core::ToolKind kind, const QString& text, const QKeySequence& shortcut) {
  auto* action = new QAction(text, this);
  action->setCheckable(true);
  action->setShortcut(shortcut);
  action->setData(static_cast<int>(kind));
  action->setToolTip(QString::fromLatin1(core::toolKindDisplayName(kind)));
  connect(action, &QAction::triggered, this, &MainWindow::onSetToolTriggered);
  toolMenu->addAction(action);
  m_toolActions[kind] = action;
  return action;
}

} // namespace app::mainwindow
