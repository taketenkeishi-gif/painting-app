#include "app/mainwindow/MainWindow.h"

#include <cstdint>

#include <QAction>
#include <QColorDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFormLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>

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

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_controller(new app::bridge::AppController(this)),
      m_canvasWidget(new app::canvasview::CanvasWidget(this)),
      m_layerPanel(new app::panels::LayerPanel(this)) {
  setWindowTitle("Layered Paint App (MVP)");
  resize(1200, 800);

  m_canvasWidget->setController(m_controller);
  setCentralWidget(m_canvasWidget);

  auto* layerDock = new QDockWidget("Layers", this);
  layerDock->setWidget(m_layerPanel);
  addDockWidget(Qt::RightDockWidgetArea, layerDock);

  m_layerPanel->setController(m_controller);
  createMenus();
  connect(m_controller, &app::bridge::AppController::toolStateChanged, this, &MainWindow::onToolStateChanged);
  onToolStateChanged();
}

void MainWindow::createMenus() {
  auto* fileMenu = menuBar()->addMenu("&File");
  auto* editMenu = menuBar()->addMenu("&Edit");
  auto* layerMenu = menuBar()->addMenu("&Layer");

  auto* newCanvasAction = new QAction("&New Canvas", this);
  auto* undoAction = new QAction("&Undo", this);
  auto* redoAction = new QAction("&Redo", this);
  auto* addLayerAction = new QAction("&Add Layer", this);
  newCanvasAction->setShortcut(QKeySequence::New);
  undoAction->setShortcut(QKeySequence::Undo);
  redoAction->setShortcut(QKeySequence::Redo);
  addLayerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));

  fileMenu->addAction(newCanvasAction);
  editMenu->addAction(undoAction);
  editMenu->addAction(redoAction);
  layerMenu->addAction(addLayerAction);

  auto* quickToolbar = addToolBar("Quick Actions");
  quickToolbar->setMovable(false);
  quickToolbar->addAction(newCanvasAction);
  quickToolbar->addAction(undoAction);
  quickToolbar->addAction(redoAction);
  quickToolbar->addAction(addLayerAction);
  quickToolbar->addSeparator();

  m_brushColorButton = new QPushButton("Color", this);
  m_brushSizeSpin = new QSpinBox(this);
  m_brushSizeSpin->setRange(1, 128);
  m_brushSizeSpin->setSingleStep(1);
  m_brushSizeSpin->setMinimumWidth(64);
  m_brushColorButton->setMinimumWidth(120);

  quickToolbar->addWidget(new QLabel("Color", this));
  quickToolbar->addWidget(m_brushColorButton);
  quickToolbar->addSeparator();
  quickToolbar->addWidget(new QLabel("Size", this));
  quickToolbar->addWidget(m_brushSizeSpin);

  connect(newCanvasAction, &QAction::triggered, this, &MainWindow::onNewCanvas);
  auto updateUndoRedoState = [this, undoAction, redoAction]() {
    const bool canUndo = m_controller->canUndo();
    const bool canRedo = m_controller->canRedo();

    if (canUndo) {
      const QString label = QString::fromStdString(m_controller->nextUndoActionName());
      undoAction->setText(label.isEmpty() ? "&Undo" : QString("&Undo %1").arg(label));
    } else {
      undoAction->setText("&Undo");
    }

    if (canRedo) {
      const QString label = QString::fromStdString(m_controller->nextRedoActionName());
      redoAction->setText(label.isEmpty() ? "&Redo" : QString("&Redo %1").arg(label));
    } else {
      redoAction->setText("&Redo");
    }

    undoAction->setEnabled(canUndo);
    redoAction->setEnabled(canRedo);
  };
  undoAction->setEnabled(false);
  redoAction->setEnabled(false);

  connect(undoAction, &QAction::triggered, this, [this, updateUndoRedoState]() {
    m_controller->undo();
    updateUndoRedoState();
  });
  connect(redoAction, &QAction::triggered, this, [this, updateUndoRedoState]() {
    m_controller->redo();
    updateUndoRedoState();
  });
  connect(m_controller, &app::bridge::AppController::documentChanged, this, updateUndoRedoState);
  connect(m_controller, &app::bridge::AppController::layersChanged, this, updateUndoRedoState);
  connect(addLayerAction, &QAction::triggered, m_controller, &app::bridge::AppController::addLayer);
  connect(m_brushColorButton, &QPushButton::clicked, this, &MainWindow::onChooseBrushColor);
  connect(m_brushSizeSpin, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::onBrushSizeChanged);
  updateUndoRedoState();

  auto* brushSizeStatus = new QLabel("Size: 8", this);
  brushSizeStatus->setObjectName("BrushSizeStatusLabel");
  auto* colorStatus = new QLabel("Color: #000000", this);
  colorStatus->setObjectName("BrushColorStatusLabel");
  auto* layerStatus = new QLabel("Layer: Layer 1", this);
  layerStatus->setObjectName("ActiveLayerStatusLabel");
  auto* zoomStatus = new QLabel("Zoom: 100%", this);
  zoomStatus->setObjectName("ZoomStatusLabel");
  statusBar()->addPermanentWidget(colorStatus);
  statusBar()->addPermanentWidget(brushSizeStatus);
  statusBar()->addPermanentWidget(zoomStatus);
  statusBar()->addPermanentWidget(layerStatus);

  auto updateActiveLayerStatus = [this, layerStatus]() {
    const core::Document& doc = m_controller->document();
    if (doc.layerCount() == 0) {
      layerStatus->setText("Layer: (none)");
      return;
    }
    const std::size_t active = doc.activeLayerIndex();
    layerStatus->setText(QString("Layer: %1").arg(QString::fromStdString(doc.layerAt(active).name())));
  };
  connect(m_controller, &app::bridge::AppController::layersChanged, this, updateActiveLayerStatus);
  connect(m_controller, &app::bridge::AppController::documentChanged, this, updateActiveLayerStatus);
  updateActiveLayerStatus();
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
