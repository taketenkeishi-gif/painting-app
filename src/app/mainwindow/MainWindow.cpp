#include "app/mainwindow/MainWindow.h"

#include <cstdint>

#include <QAction>
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
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

#include "app/bridge/AppController.h"
#include "app/canvasview/CanvasWidget.h"
#include "app/panels/LayerPanel.h"

namespace app::mainwindow {

namespace {

core::Color toCoreColor(const QColor& color) {
  return core::Color {
      static_cast<std::uint8_t>(color.red()),
      static_cast<std::uint8_t>(color.green()),
      static_cast<std::uint8_t>(color.blue()),
      static_cast<std::uint8_t>(color.alpha())};
}

QColor toQColor(const core::Color& color) {
  return QColor(color.r, color.g, color.b, color.a);
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
      m_layerPanel(new app::panels::LayerPanel(this)) {
  setWindowTitle("Layered Paint App");
  resize(1400, 860);

  m_canvasWidget->setController(m_controller);
  m_layerPanel->setController(m_controller);

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
}

void MainWindow::setupShellLayout() {
  auto* root = new QWidget(this);
  auto* rootLayout = new QHBoxLayout(root);
  rootLayout->setContentsMargins(6, 6, 6, 6);
  rootLayout->setSpacing(8);

  m_leftToolHost = new QFrame(root);
  m_leftToolHost->setObjectName("LeftToolHost");
  m_leftToolHost->setMinimumWidth(96);
  m_leftToolHost->setMaximumWidth(120);
  auto* leftLayout = new QVBoxLayout(m_leftToolHost);
  leftLayout->setContentsMargins(8, 8, 8, 8);
  leftLayout->setSpacing(8);
  auto* leftTitle = new QLabel("Tools", m_leftToolHost);
  leftTitle->setStyleSheet("font-weight: 700;");
  leftLayout->addWidget(leftTitle);
  leftLayout->addWidget(new QLabel("Tool buttons\n(Phase UI2)", m_leftToolHost));
  leftLayout->addStretch(1);

  auto* centerHost = new QWidget(root);
  auto* centerLayout = new QVBoxLayout(centerHost);
  centerLayout->setContentsMargins(0, 0, 0, 0);
  centerLayout->setSpacing(6);

  m_topBar = new QFrame(centerHost);
  m_topBar->setObjectName("TopBarHost");
  auto* topLayout = new QHBoxLayout(m_topBar);
  topLayout->setContentsMargins(10, 8, 10, 8);
  topLayout->setSpacing(8);

  topLayout->addWidget(new QLabel("Color", m_topBar));
  m_brushColorButton = new QPushButton("Color", m_topBar);
  m_brushColorButton->setMinimumWidth(120);
  topLayout->addWidget(m_brushColorButton);

  topLayout->addWidget(new QLabel("Size", m_topBar));
  m_brushSizeSpin = new QSpinBox(m_topBar);
  m_brushSizeSpin->setRange(1, 128);
  m_brushSizeSpin->setSingleStep(1);
  m_brushSizeSpin->setMinimumWidth(72);
  topLayout->addWidget(m_brushSizeSpin);
  topLayout->addStretch(1);

  connect(m_brushColorButton, &QPushButton::clicked, this, &MainWindow::onChooseBrushColor);
  connect(m_brushSizeSpin, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::onBrushSizeChanged);

  centerLayout->addWidget(m_topBar);
  centerLayout->addWidget(m_canvasWidget, 1);

  m_rightPanelHost = new QFrame(root);
  m_rightPanelHost->setObjectName("RightPanelHost");
  m_rightPanelHost->setMinimumWidth(280);
  m_rightPanelHost->setMaximumWidth(360);
  auto* rightLayout = new QVBoxLayout(m_rightPanelHost);
  rightLayout->setContentsMargins(4, 4, 4, 4);
  rightLayout->setSpacing(8);

  rightLayout->addWidget(makePanelGroup("Layer", m_layerPanel, m_rightPanelHost), 2);

  auto* subToolPlaceholder = new QLabel("Sub tool panel\n(Phase UI2)", m_rightPanelHost);
  subToolPlaceholder->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  m_subToolPlaceholderLabel = subToolPlaceholder;
  rightLayout->addWidget(makePanelGroup("Sub Tool", subToolPlaceholder, m_rightPanelHost), 1);

  auto* propertyPlaceholder = new QLabel("Tool property panel\n(Phase UI2)", m_rightPanelHost);
  propertyPlaceholder->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  m_propertyPlaceholderLabel = propertyPlaceholder;
  rightLayout->addWidget(makePanelGroup("Tool Property", propertyPlaceholder, m_rightPanelHost), 1);

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

  auto* brushSizeStatus = new QLabel("Size: 8", this);
  brushSizeStatus->setObjectName("BrushSizeStatusLabel");
  auto* colorStatus = new QLabel("Color: #000000", this);
  colorStatus->setObjectName("BrushColorStatusLabel");
  m_activeLayerStatusLabel = new QLabel("Layer: Layer 1", this);
  m_activeLayerStatusLabel->setObjectName("ActiveLayerStatusLabel");
  auto* zoomStatus = new QLabel("Zoom: 100%", this);
  zoomStatus->setObjectName("ZoomStatusLabel");

  statusBar()->addPermanentWidget(colorStatus);
  statusBar()->addPermanentWidget(brushSizeStatus);
  statusBar()->addPermanentWidget(zoomStatus);
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

void MainWindow::onChooseBrushColor() {
  const app::bridge::ToolStateViewModel state = m_controller->toolState();
  const QColor picked = QColorDialog::getColor(toQColor(state.color), this, "Brush Color", QColorDialog::ShowAlphaChannel);
  if (!picked.isValid()) {
    return;
  }
  m_controller->setBrushColor(toCoreColor(picked));
}

void MainWindow::onBrushSizeChanged(int size) {
  m_controller->setBrushSize(size);
}

void MainWindow::onToolStateChanged() {
  if (m_brushSizeSpin == nullptr || m_brushColorButton == nullptr) {
    return;
  }

  const app::bridge::ToolStateViewModel state = m_controller->toolState();
  const QSignalBlocker spinBlocker(m_brushSizeSpin);
  m_brushSizeSpin->setValue(state.size);
  updateBrushColorButton();

  if (auto* sizeLabel = findChild<QLabel*>("BrushSizeStatusLabel"); sizeLabel != nullptr) {
    sizeLabel->setText(QString("Size: %1").arg(state.size));
  }
}

void MainWindow::updateBrushColorButton() {
  const app::bridge::ToolStateViewModel state = m_controller->toolState();
  const QColor color = toQColor(state.color);
  const int luminance = (299 * color.red() + 587 * color.green() + 114 * color.blue()) / 1000;
  const QString textColor = luminance > 128 ? "#111111" : "#f5f5f5";
  const QString hex = color.name(QColor::HexRgb).toUpper();

  m_brushColorButton->setText(QString("Color %1").arg(hex));
  m_brushColorButton->setStyleSheet(
      QString("QPushButton { background-color: rgba(%1, %2, %3, %4); color: %5; border: 2px solid #666; padding: 2px 6px; font-weight: 600; }")
          .arg(color.red())
          .arg(color.green())
          .arg(color.blue())
          .arg(color.alpha())
          .arg(textColor));

  if (auto* colorLabel = findChild<QLabel*>("BrushColorStatusLabel"); colorLabel != nullptr) {
    colorLabel->setText(QString("Color: %1").arg(hex));
  }
}

} // namespace app::mainwindow
