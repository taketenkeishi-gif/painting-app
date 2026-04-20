#include "app/mainwindow/MainWindow.h"

#include <cstdint>

#include <QAction>
#include <QActionGroup>
#include <QColorDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
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
  auto* root = new QWidget(this);
  auto* rootLayout = new QVBoxLayout(root);
  rootLayout->setContentsMargins(4, 4, 4, 4);
  rootLayout->setSpacing(6);

  m_topBar = new QFrame(root);
  m_topBar->setObjectName("TopBarHost");
  auto* topLayout = new QHBoxLayout(m_topBar);
  topLayout->setContentsMargins(10, 6, 10, 6);
  topLayout->setSpacing(12);
  m_currentToolLabel = new QLabel("Tool: Brush", m_topBar);
  m_currentSubToolLabel = new QLabel("Sub Tool: Normal", m_topBar);
  topLayout->addWidget(m_currentToolLabel);
  topLayout->addWidget(m_currentSubToolLabel);
  topLayout->addStretch(1);
  rootLayout->addWidget(m_topBar);

  m_mainSplitter = new QSplitter(Qt::Horizontal, root);

  m_leftToolHost = new QFrame(m_mainSplitter);
  m_leftToolHost->setObjectName("LeftToolHost");
  auto* leftLayout = new QVBoxLayout(m_leftToolHost);
  leftLayout->setContentsMargins(4, 4, 4, 4);
  leftLayout->setSpacing(6);
  m_leftSplitter = new QSplitter(Qt::Vertical, m_leftToolHost);
  leftLayout->addWidget(m_leftSplitter, 1);

  auto* toolGroup = makePanelGroup("Tools", m_toolPanel, m_leftSplitter);
  auto* subToolGroup = makePanelGroup("Sub Tool", m_subToolPanel, m_leftSplitter);
  auto* propGroup = makePanelGroup("Tool Property", m_toolPropertyPanel, m_leftSplitter);

  auto* colorPanel = new QWidget(m_leftSplitter);
  auto* colorLayout = new QVBoxLayout(colorPanel);
  colorLayout->setContentsMargins(8, 8, 8, 8);
  colorLayout->setSpacing(6);
  auto* colorTitle = new QLabel("Color", colorPanel);
  colorTitle->setStyleSheet("font-weight: 700;");
  m_foregroundColorButton = new QPushButton("FG", colorPanel);
  m_backgroundColorButton = new QPushButton("BG", colorPanel);
  auto* swapColorButton = new QPushButton("Swap", colorPanel);
  auto* colorButtons = new QHBoxLayout();
  colorButtons->setContentsMargins(0, 0, 0, 0);
  colorButtons->setSpacing(6);
  colorButtons->addWidget(m_foregroundColorButton);
  colorButtons->addWidget(m_backgroundColorButton);
  colorLayout->addWidget(colorTitle);
  colorLayout->addLayout(colorButtons);
  colorLayout->addWidget(swapColorButton);
  colorLayout->addStretch(1);
  connect(m_foregroundColorButton, &QPushButton::clicked, this, &MainWindow::onChooseForegroundColor);
  connect(m_backgroundColorButton, &QPushButton::clicked, this, &MainWindow::onChooseBackgroundColor);
  connect(swapColorButton, &QPushButton::clicked, this, &MainWindow::onSwapColors);

  m_leftSplitter->addWidget(toolGroup);
  m_leftSplitter->addWidget(subToolGroup);
  m_leftSplitter->addWidget(propGroup);
  m_leftSplitter->addWidget(colorPanel);
  m_leftSplitter->setStretchFactor(0, 0);
  m_leftSplitter->setStretchFactor(1, 2);
  m_leftSplitter->setStretchFactor(2, 3);
  m_leftSplitter->setStretchFactor(3, 1);

  auto* centerHost = new QFrame(m_mainSplitter);
  centerHost->setObjectName("CenterCanvasHost");
  auto* centerLayout = new QVBoxLayout(centerHost);
  centerLayout->setContentsMargins(0, 0, 0, 0);
  centerLayout->setSpacing(0);
  centerLayout->addWidget(m_canvasWidget, 1);

  m_rightPanelHost = new QFrame(m_mainSplitter);
  m_rightPanelHost->setObjectName("RightPanelHost");
  auto* rightLayout = new QVBoxLayout(m_rightPanelHost);
  rightLayout->setContentsMargins(4, 4, 4, 4);
  rightLayout->setSpacing(6);
  m_rightSplitter = new QSplitter(Qt::Vertical, m_rightPanelHost);
  rightLayout->addWidget(m_rightSplitter, 1);

  auto* layerGroup = makePanelGroup("Layer", m_layerPanel, m_rightSplitter);

  m_rightTabWidget = new QTabWidget(m_rightSplitter);
  auto* infoPanel = new QWidget(m_rightTabWidget);
  auto* infoLayout = new QVBoxLayout(infoPanel);
  infoLayout->setContentsMargins(8, 8, 8, 8);
  infoLayout->setSpacing(6);
  auto* infoTitle = new QLabel("Tool Guide", infoPanel);
  infoTitle->setStyleSheet("font-weight: 700;");
  auto* infoText = new QLabel("Use left panels to pick tool/sub tool and tune properties.", infoPanel);
  infoText->setWordWrap(true);
  infoLayout->addWidget(infoTitle);
  infoLayout->addWidget(infoText);
  infoLayout->addStretch(1);
  m_rightTabWidget->addTab(infoPanel, "Info");

  m_rightSplitter->addWidget(layerGroup);
  m_rightSplitter->addWidget(m_rightTabWidget);
  m_rightSplitter->setStretchFactor(0, 3);
  m_rightSplitter->setStretchFactor(1, 1);

  m_mainSplitter->addWidget(m_leftToolHost);
  m_mainSplitter->addWidget(centerHost);
  m_mainSplitter->addWidget(m_rightPanelHost);
  m_mainSplitter->setStretchFactor(0, 0);
  m_mainSplitter->setStretchFactor(1, 1);
  m_mainSplitter->setStretchFactor(2, 0);
  m_mainSplitter->setSizes({340, 920, 320});

  rootLayout->addWidget(m_mainSplitter, 1);
  setCentralWidget(root);
}

void MainWindow::createMenus() {
  auto* fileMenu = menuBar()->addMenu("&File");
  auto* editMenu = menuBar()->addMenu("&Edit");
  auto* toolMenu = menuBar()->addMenu("&Tool");
  auto* selectMenu = menuBar()->addMenu("&Select");
  auto* layerMenu = menuBar()->addMenu("&Layer");

  m_newCanvasAction = new QAction("&New Canvas", this);
  m_undoAction = new QAction("&Undo", this);
  m_redoAction = new QAction("&Redo", this);
  m_addLayerAction = new QAction("&Add Layer", this);
  m_moveLayerUpAction = new QAction("Move Layer &Up", this);
  m_moveLayerDownAction = new QAction("Move Layer &Down", this);
  m_toggleLayerVisibilityAction = new QAction("&Toggle Visibility", this);
  m_clearSelectionAction = new QAction("&Clear Selection", this);
  m_invertSelectionAction = new QAction("&Invert Selection", this);
  m_brushSizeDownAction = new QAction("Brush Size &Down", this);
  m_brushSizeUpAction = new QAction("Brush Size &Up", this);

  m_newCanvasAction->setShortcut(QKeySequence::New);
  m_undoAction->setShortcut(QKeySequence::Undo);
  m_redoAction->setShortcuts({QKeySequence::Redo, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z)});
  m_addLayerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
  m_moveLayerUpAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Up));
  m_moveLayerDownAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Down));
  m_toggleLayerVisibilityAction->setShortcut(QKeySequence(Qt::Key_V));
  m_clearSelectionAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
  m_invertSelectionAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I));
  m_brushSizeDownAction->setShortcut(QKeySequence(Qt::Key_BracketLeft));
  m_brushSizeUpAction->setShortcut(QKeySequence(Qt::Key_BracketRight));

  fileMenu->addAction(m_newCanvasAction);
  editMenu->addAction(m_undoAction);
  editMenu->addAction(m_redoAction);
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
  selectMenu->addAction(m_invertSelectionAction);

  layerMenu->addAction(m_addLayerAction);
  layerMenu->addAction(m_moveLayerUpAction);
  layerMenu->addAction(m_moveLayerDownAction);
  layerMenu->addAction(m_toggleLayerVisibilityAction);

  connect(m_newCanvasAction, &QAction::triggered, this, &MainWindow::onNewCanvas);
  connect(m_undoAction, &QAction::triggered, this, &MainWindow::onUndoTriggered);
  connect(m_redoAction, &QAction::triggered, this, &MainWindow::onRedoTriggered);
  connect(m_clearSelectionAction, &QAction::triggered, this, &MainWindow::onClearSelectionTriggered);
  connect(m_invertSelectionAction, &QAction::triggered, this, &MainWindow::onInvertSelectionTriggered);
  connect(m_toggleLayerVisibilityAction, &QAction::triggered, this, &MainWindow::onToggleLayerVisibilityTriggered);
  connect(m_moveLayerUpAction, &QAction::triggered, this, &MainWindow::onMoveLayerUpTriggered);
  connect(m_moveLayerDownAction, &QAction::triggered, this, &MainWindow::onMoveLayerDownTriggered);
  connect(m_brushSizeDownAction, &QAction::triggered, this, &MainWindow::onDecreaseBrushSizeTriggered);
  connect(m_brushSizeUpAction, &QAction::triggered, this, &MainWindow::onIncreaseBrushSizeTriggered);
  connect(m_addLayerAction, &QAction::triggered, m_controller, &app::bridge::AppController::addLayer);

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
  m_undoAction->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
  m_redoAction->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
  m_addLayerAction->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
  m_toggleLayerVisibilityAction->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
  m_moveLayerUpAction->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
  m_moveLayerDownAction->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));

  m_quickToolBar->addAction(m_newCanvasAction);
  m_quickToolBar->addSeparator();
  m_quickToolBar->addAction(m_undoAction);
  m_quickToolBar->addAction(m_redoAction);
  m_quickToolBar->addSeparator();
  m_quickToolBar->addAction(m_addLayerAction);
  m_quickToolBar->addAction(m_moveLayerUpAction);
  m_quickToolBar->addAction(m_moveLayerDownAction);
  m_quickToolBar->addAction(m_toggleLayerVisibilityAction);
}

void MainWindow::applyUiChrome() {
  setStyleSheet(
      "QMainWindow { background: #202328; color: #e8e8e8; }"
      "#TopBarHost { background: #2b2f36; border: 1px solid #3a3f48; border-radius: 4px; }"
      "#LeftToolHost, #RightPanelHost { background: #24272d; border: 1px solid #353b45; border-radius: 4px; }"
      "#CenterCanvasHost { background: #1f2227; border: 1px solid #353b45; border-radius: 4px; }"
      "QGroupBox { border: 1px solid #3f4652; border-radius: 4px; margin-top: 12px; padding-top: 10px; }"
      "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; color: #d9dde5; font-weight: 700; }"
      "QListWidget { background: #1f2328; border: 1px solid #353b44; }"
      "QListWidget::item:selected { background: #32507a; color: #ffffff; }"
      "QPushButton { background: #2d323a; border: 1px solid #4b5464; border-radius: 3px; padding: 4px 8px; }"
      "QPushButton:hover { background: #39404a; }"
      "QMenuBar, QToolBar { background: #2a2f36; }"
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
  m_activeLayerStatusLabel->setText(QString("Layer: %1").arg(QString::fromStdString(doc.layerAt(active).name())));
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

void MainWindow::onDecreaseBrushSizeTriggered() {
  m_controller->adjustBrushSize(-1);
}

void MainWindow::onIncreaseBrushSizeTriggered() {
  m_controller->adjustBrushSize(1);
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
    }
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
