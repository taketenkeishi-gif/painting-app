#include "app/mainwindow/MainWindow.h"

#include <QAction>
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
#include <QSpinBox>
#include <QStatusBar>
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

  connect(m_controller, &app::bridge::AppController::toolStateChanged, this, &MainWindow::onToolStateChanged);
  connect(m_controller, &app::bridge::AppController::layersChanged, this, &MainWindow::updateActiveLayerStatus);
  connect(m_controller, &app::bridge::AppController::documentChanged, this, &MainWindow::updateActiveLayerStatus);
  connect(m_controller, &app::bridge::AppController::documentChanged, this, &MainWindow::updateUndoRedoState);
  connect(m_controller, &app::bridge::AppController::layersChanged, this, &MainWindow::updateUndoRedoState);

  onToolStateChanged();
  updateUndoRedoState();
  updateActiveLayerStatus();
  updateTopToolInfo();
}

void MainWindow::setupShellLayout() {
  auto* root = new QWidget(this);
  auto* rootLayout = new QHBoxLayout(root);
  rootLayout->setContentsMargins(6, 6, 6, 6);
  rootLayout->setSpacing(8);

  m_leftToolHost = new QFrame(root);
  m_leftToolHost->setObjectName("LeftToolHost");
  m_leftToolHost->setMinimumWidth(110);
  m_leftToolHost->setMaximumWidth(140);
  auto* leftLayout = new QVBoxLayout(m_leftToolHost);
  leftLayout->setContentsMargins(8, 8, 8, 8);
  leftLayout->setSpacing(8);
  auto* leftTitle = new QLabel("Tools", m_leftToolHost);
  leftTitle->setStyleSheet("font-weight: 700;");
  leftLayout->addWidget(leftTitle);
  leftLayout->addWidget(m_toolPanel, 1);

  auto* centerHost = new QWidget(root);
  auto* centerLayout = new QVBoxLayout(centerHost);
  centerLayout->setContentsMargins(0, 0, 0, 0);
  centerLayout->setSpacing(6);

  m_topBar = new QFrame(centerHost);
  m_topBar->setObjectName("TopBarHost");
  auto* topLayout = new QHBoxLayout(m_topBar);
  topLayout->setContentsMargins(10, 8, 10, 8);
  topLayout->setSpacing(12);
  m_currentToolLabel = new QLabel("Tool: Brush", m_topBar);
  m_currentSubToolLabel = new QLabel("Sub Tool: Round Brush", m_topBar);
  topLayout->addWidget(m_currentToolLabel);
  topLayout->addWidget(m_currentSubToolLabel);
  topLayout->addStretch(1);

  centerLayout->addWidget(m_topBar);
  centerLayout->addWidget(m_canvasWidget, 1);

  m_rightPanelHost = new QFrame(root);
  m_rightPanelHost->setObjectName("RightPanelHost");
  m_rightPanelHost->setMinimumWidth(300);
  m_rightPanelHost->setMaximumWidth(380);
  auto* rightLayout = new QVBoxLayout(m_rightPanelHost);
  rightLayout->setContentsMargins(4, 4, 4, 4);
  rightLayout->setSpacing(8);

  rightLayout->addWidget(makePanelGroup("Layer", m_layerPanel, m_rightPanelHost), 2);
  rightLayout->addWidget(makePanelGroup("Sub Tool", m_subToolPanel, m_rightPanelHost), 1);
  rightLayout->addWidget(makePanelGroup("Tool Property", m_toolPropertyPanel, m_rightPanelHost), 2);

  rootLayout->addWidget(m_leftToolHost);
  rootLayout->addWidget(centerHost, 1);
  rootLayout->addWidget(m_rightPanelHost);

  setCentralWidget(root);
}

void MainWindow::createMenus() {
  auto* fileMenu = menuBar()->addMenu("&File");
  auto* editMenu = menuBar()->addMenu("&Edit");
  auto* layerMenu = menuBar()->addMenu("&Layer");

  m_newCanvasAction = new QAction("&New Canvas", this);
  m_undoAction = new QAction("&Undo", this);
  m_redoAction = new QAction("&Redo", this);
  m_addLayerAction = new QAction("&Add Layer", this);

  m_newCanvasAction->setShortcut(QKeySequence::New);
  m_undoAction->setShortcut(QKeySequence::Undo);
  m_redoAction->setShortcut(QKeySequence::Redo);
  m_addLayerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));

  fileMenu->addAction(m_newCanvasAction);
  editMenu->addAction(m_undoAction);
  editMenu->addAction(m_redoAction);
  layerMenu->addAction(m_addLayerAction);

  connect(m_newCanvasAction, &QAction::triggered, this, &MainWindow::onNewCanvas);
  connect(m_undoAction, &QAction::triggered, this, &MainWindow::onUndoTriggered);
  connect(m_redoAction, &QAction::triggered, this, &MainWindow::onRedoTriggered);
  connect(m_addLayerAction, &QAction::triggered, m_controller, &app::bridge::AppController::addLayer);

  m_toolStatusLabel = new QLabel("Tool: Brush", this);
  m_toolStatusLabel->setObjectName("ToolStatusLabel");
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

  statusBar()->addWidget(m_toolStatusLabel);
  statusBar()->addWidget(m_guideStatusLabel, 1);
  statusBar()->addPermanentWidget(m_colorStatusLabel);
  statusBar()->addPermanentWidget(m_sizeStatusLabel);
  statusBar()->addPermanentWidget(m_zoomStatusLabel);
  statusBar()->addPermanentWidget(m_activeLayerStatusLabel);
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
}

void MainWindow::updateTopToolInfo() {
  if (m_currentToolLabel == nullptr || m_currentSubToolLabel == nullptr) {
    return;
  }

  m_currentToolLabel->setText(QString("Tool: %1").arg(QString::fromStdString(m_controller->currentToolDisplayName())));
  m_currentSubToolLabel->setText(QString("Sub Tool: %1").arg(QString::fromStdString(m_controller->currentSubToolDisplayName())));
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

  if (m_guideStatusLabel != nullptr) {
    m_guideStatusLabel->setText(QString("Guide: %1").arg(QString::fromStdString(m_controller->currentToolGuide())));
  }

  updateTopToolInfo();
}

} // namespace app::mainwindow
