#include "app/mainwindow/MainWindow.h"

#include <QAction>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>

#include <algorithm>
#include <cstdint>
#include <vector>

#include <QAction>
#include <QActionGroup>
#include <QClipboard>
#include <QColorDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLayoutItem>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMap>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSettings>
#include <QSlider>
#include <QSplitter>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyle>
#include <QSet>
#include <QTabWidget>
#include <QToolBar>
#include <QTimer>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>
#include <QUrl>

#include "app/bridge/AppController.h"
#include "app/canvasview/CanvasWidget.h"
#include "app/panels/LayerPanel.h"
#include "app/panels/SubToolPanel.h"
#include "app/panels/ToolPanel.h"
#include "app/panels/ToolPropertyPanel.h"
#include "app/panels/ColorWheelWidget.h"
#include "app/ui/IconLoader.h"
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

QString toolNameJa(core::ToolKind kind) {
  switch (kind) {
    case core::ToolKind::Brush:
      return "ブラシ";
    case core::ToolKind::Eraser:
      return "消しゴム";
    case core::ToolKind::Eyedropper:
      return "スポイト";
    case core::ToolKind::Fill:
      return "塗りつぶし";
    case core::ToolKind::Line:
      return "直線";
    case core::ToolKind::RectSelection:
      return "選択";
    case core::ToolKind::MoveLayer:
      return "移動";
    case core::ToolKind::Hand:
      return "手のひら";
    case core::ToolKind::Zoom:
      return "ズーム";
    default:
      return "ツール";
  }
}

QString layerKindJa(core::LayerKind kind) {
  switch (kind) {
    case core::LayerKind::Raster:
      return "ラスタ";
    case core::LayerKind::Vector:
      return "ベクター";
    case core::LayerKind::Folder:
      return "フォルダ";
    default:
      return "不明";
  }
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_controller(new app::bridge::AppController(this)),
      m_canvasWidget(new app::canvasview::CanvasWidget(this)),
      m_layerPanel(new app::panels::LayerPanel(this)),
      m_toolPanel(new app::panels::ToolPanel(this)),
      m_quickSliderPanel(new app::panels::ToolPanel(this)),
      m_subToolPanel(new app::panels::SubToolPanel(this)),
      m_toolPropertyPanel(new app::panels::ToolPropertyPanel(this)) {
  // CommandRegistry smoke: first visible backend connection.
  m_commandRegistry.registerCommand(
      app::commands::CommandDescriptor {
          "app.commandRegistrySmoke",
          "Command Registry Smoke",
          "Developer",
          "",
          "Verifies that CommandRegistry is wired into MainWindow.",
          true
      },
      [this]() {
        QMessageBox::information(this, QStringLiteral("CommandRegistry"), QStringLiteral("CommandRegistry is connected."));
        return true;
      });

  // Keep the smoke command internal; do not expose it in user-facing UI.

  setWindowTitle("LayeredPaint");
  resize(1400, 860);

  m_canvasWidget->setController(m_controller);
  m_layerPanel->setController(m_controller);
  m_toolPanel->setController(m_controller);
  m_quickSliderPanel->setController(m_controller);
  m_subToolPanel->setController(m_controller);
  m_toolPropertyPanel->setController(m_controller);
  m_toolPanel->setSections(app::panels::ToolPanel::ButtonsOnly);
  m_quickSliderPanel->setSections(app::panels::ToolPanel::QuickSlidersOnly);

  setupShellLayout();
  createMenus();
  loadWorkspaceLayoutState();
  loadShortcutOverrides();
  createToolBar();
  applyUiChrome();

  connect(m_controller, &app::bridge::AppController::toolStateChanged, this, &MainWindow::onToolStateChanged);
  connect(m_controller, &app::bridge::AppController::foregroundColorUsed, this, [this]() {
    pushForegroundColorHistory(m_controller->toolState().color);
    refreshColorHistoryButtons();
  }, Qt::QueuedConnection);
  connect(m_controller, &app::bridge::AppController::layersChanged, this, &MainWindow::updateActiveLayerStatus);
  connect(m_controller, &app::bridge::AppController::documentChanged, this, &MainWindow::updateActiveLayerStatus);
  connect(m_controller, &app::bridge::AppController::documentChanged, this, &MainWindow::updateUndoRedoState);
  connect(m_controller, &app::bridge::AppController::layersChanged, this, &MainWindow::updateUndoRedoState);
  connect(m_controller, &app::bridge::AppController::documentChanged, this, &MainWindow::updateNavigatorPreview);
  connect(m_controller, &app::bridge::AppController::layersChanged, this, &MainWindow::updateNavigatorPreview);
  connect(m_controller, &app::bridge::AppController::overlayChanged, this, &MainWindow::updateObjectSelectionStatus);
  connect(m_controller, &app::bridge::AppController::documentChanged, this, &MainWindow::updateObjectSelectionStatus);

  onToolStateChanged();
  updateUndoRedoState();
  updateActiveLayerStatus();
  updateTopToolInfo();
  updateColorPanel();
  updateNavigatorPreview();
  updateObjectSelectionStatus();
}

void MainWindow::setupShellLayout() {
  setCentralWidget(m_canvasWidget);
  setDockNestingEnabled(true);
  setDockOptions(
      QMainWindow::AnimatedDocks |
      QMainWindow::AllowNestedDocks |
      QMainWindow::AllowTabbedDocks |
      QMainWindow::GroupedDragging);
  setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);
  setTabPosition(Qt::RightDockWidgetArea, QTabWidget::North);

  auto* colorPanel = new QWidget(this);
  m_colorPanelWidget = colorPanel;
  auto* colorLayout = new QVBoxLayout(colorPanel);
  colorLayout->setContentsMargins(2, 2, 2, 2);
  colorLayout->setSpacing(1);
  auto* colorTitle = new QLabel("カラー", colorPanel);
  colorTitle->setVisible(false);
  colorTitle->setStyleSheet("font-weight: 700; font-size: 11px;");
  m_foregroundColorButton = new QPushButton(colorPanel);
  m_backgroundColorButton = new QPushButton(colorPanel);
  auto* swapColorButton = new QPushButton(colorPanel);
  auto* resetColorButton = new QPushButton(colorPanel);
  auto* transparentColorButton = new QPushButton(colorPanel);
  m_hueSlider = new QSlider(Qt::Horizontal, colorPanel);
  m_satSlider = new QSlider(Qt::Horizontal, colorPanel);
  m_valSlider = new QSlider(Qt::Horizontal, colorPanel);
  m_alphaSlider = new QSlider(Qt::Horizontal, colorPanel);
  m_hueSpin = new QSpinBox(colorPanel);
  m_satSpin = new QSpinBox(colorPanel);
  m_valSpin = new QSpinBox(colorPanel);
  m_alphaSpin = new QSpinBox(colorPanel);
  m_colorWheelWidget = new app::panels::ColorWheelWidget(colorPanel);
  m_colorWheelWidget->setMinimumSize(104, 104);
  m_foregroundColorButton->setFixedSize(22, 22);
  m_backgroundColorButton->setFixedSize(22, 22);
  swapColorButton->setFixedSize(20, 20);
  resetColorButton->setFixedSize(20, 20);
  transparentColorButton->setFixedSize(18, 10);
  swapColorButton->setIcon(app::ui::icon("swap"));
  swapColorButton->setIconSize(QSize(12, 12));
  resetColorButton->setIcon(app::ui::icon("reset_bw"));
  resetColorButton->setIconSize(QSize(12, 12));
  transparentColorButton->setIcon(app::ui::icon("transparent"));
  transparentColorButton->setIconSize(QSize(10, 10));
  swapColorButton->setFlat(true);
  resetColorButton->setFlat(true);
  transparentColorButton->setFlat(true);
  const QString colorOpButtonStyle = QStringLiteral(
      "QPushButton { min-width: 20px; max-width: 20px; min-height: 20px; max-height: 20px; "
      "padding: 0px; margin: 0px; border: 1px solid #4c5a70; border-radius: 2px; background: #26303c; }"
      "QPushButton:hover { border: 1px solid #8fb5e8; background: #303b4a; }"
      "QPushButton:pressed { border: 1px solid #b7d4ff; background: #223a56; }");
  swapColorButton->setStyleSheet(colorOpButtonStyle);
  resetColorButton->setStyleSheet(colorOpButtonStyle);

  m_hueSlider->setRange(0, 359);
  m_satSlider->setRange(0, 255);
  m_valSlider->setRange(0, 255);
  m_alphaSlider->setRange(0, 255);
  m_hueSpin->setRange(0, 359);
  m_satSpin->setRange(0, 255);
  m_valSpin->setRange(0, 255);
  m_alphaSpin->setRange(0, 255);
  swapColorButton->setToolTip("描画色と背景色を入れ替え");
  resetColorButton->setToolTip("描画色/背景色を白黒に戻す");
  transparentColorButton->setToolTip("前景色を透明にする");
  auto* colorButtons = new QHBoxLayout();
  colorButtons->setContentsMargins(0, 0, 0, 0);
  colorButtons->setSpacing(1);
  auto* colorSwatchHost = new QWidget(colorPanel);
  colorSwatchHost->setFixedSize(42, 36);
  colorSwatchHost->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  colorSwatchHost->setStyleSheet("background: transparent;");

  m_backgroundColorButton->setParent(colorSwatchHost);
  m_backgroundColorButton->move(16, 7);
  m_backgroundColorButton->setFixedSize(22, 22);

  m_foregroundColorButton->setParent(colorSwatchHost);
  m_foregroundColorButton->move(2, 0);
  m_foregroundColorButton->setFixedSize(22, 22);

  transparentColorButton->setParent(colorSwatchHost);
  transparentColorButton->move(2, 24);
  transparentColorButton->setFixedSize(18, 10);

  m_backgroundColorButton->show();
  m_foregroundColorButton->show();
  transparentColorButton->show();
  auto makeTransparentCheckerIcon = []() {
    QPixmap pixmap(16, 8);
    pixmap.fill(QColor(244, 244, 244));

    QPainter checkerPainter(&pixmap);
    const QColor light(244, 244, 244);
    const QColor dark(142, 151, 164);
    constexpr int cell = 2;
    for (int y = 0; y < pixmap.height(); y += cell) {
      for (int x = 0; x < pixmap.width(); x += cell) {
        const bool useDark = ((x / cell) + (y / cell)) % 2 == 1;
        checkerPainter.fillRect(QRect(x, y, cell, cell), useDark ? dark : light);
      }
    }
    checkerPainter.end();
    return QIcon(pixmap);
  };

  transparentColorButton->setIcon(makeTransparentCheckerIcon());
  transparentColorButton->setIconSize(QSize(16, 8));
  transparentColorButton->setText(QString());
  transparentColorButton->setToolTip(QStringLiteral("透明色"));
  transparentColorButton->setStyleSheet(QStringLiteral(
      "QPushButton { min-width: 18px; max-width: 18px; min-height: 10px; max-height: 10px; "
      "padding: 0px; margin: 0px; border: 1px solid #6a7484; border-radius: 0px; background: transparent; }"
      "QPushButton:hover { border: 1px solid #eef3fb; }"));

  m_backgroundColorButton->raise();
  m_foregroundColorButton->raise();
  transparentColorButton->raise();

  colorButtons->addWidget(colorSwatchHost, 0, Qt::AlignLeft | Qt::AlignTop);
  auto* colorOps = new QHBoxLayout();
  colorOps->setContentsMargins(0, 0, 0, 0);
  colorOps->setSpacing(1);
  colorOps->addStretch(1);
  colorOps->addWidget(swapColorButton);
  colorOps->addWidget(resetColorButton);
  // transparentColorButton is shown in colorSwatchHost.
  colorButtons->addLayout(colorOps, 1);
  auto addHsvRow = [this, colorPanel](const QString& label, QSlider* slider, QSpinBox* spin) {
    auto* row = new QHBoxLayout();
    row->setContentsMargins(2, 1, 2, 1);
    row->setSpacing(4);

    const QString accent =
        label == QStringLiteral("H") ? QStringLiteral("#ff5a8a") :
        label == QStringLiteral("S") ? QStringLiteral("#57d68d") :
        label == QStringLiteral("V") ? QStringLiteral("#f2c45d") :
        QStringLiteral("#8fb5ff");

    const QString accentSoft =
        label == QStringLiteral("H") ? QStringLiteral("#8a3552") :
        label == QStringLiteral("S") ? QStringLiteral("#2f7450") :
        label == QStringLiteral("V") ? QStringLiteral("#7b6531") :
        QStringLiteral("#415d8c");

    auto* nameLabel = new QLabel(label, colorPanel);
    nameLabel->setFixedWidth(17);
    nameLabel->setMinimumHeight(18);
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setStyleSheet(QStringLiteral(
        "QLabel {"
        "  color: %1;"
        "  background: #202936;"
        "  border: 1px solid %2;"
        "  border-radius: 2px;"
        "  font-weight: 800;"
        "  padding: 0px;"
        "}").arg(accent, accentSoft));

    slider->setMinimumWidth(76);
    slider->setFixedHeight(18);
    slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    slider->setStyleSheet(QStringLiteral(
        "QSlider::groove:horizontal {"
        "  height: 7px;"
        "  border-radius: 3px;"
        "  background: #202630;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  height: 7px;"
        "  border-radius: 3px;"
        "  background: %1;"
        "}"
        "QSlider::add-page:horizontal {"
        "  height: 7px;"
        "  border-radius: 3px;"
        "  background: #141a22;"
        "}"
        "QSlider::handle:horizontal {"
        "  width: 12px;"
        "  height: 12px;"
        "  margin: -4px 0px;"
        "  border-radius: 6px;"
        "  border: 1px solid #dce6f2;"
        "  background: %1;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "  border: 1px solid #ffffff;"
        "}").arg(accent));

    spin->setMinimumWidth(44);
    spin->setMaximumWidth(48);
    spin->setFixedWidth(46);
    spin->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
    spin->setStyleSheet(QStringLiteral(
        "QSpinBox {"
        "  min-height: 22px;"
        "  padding-left: 4px;"
        "  padding-right: 14px;"
        "  border: 1px solid #455166;"
        "  background: #121820;"
        "  color: #eef3fb;"
        "}"
        "QSpinBox::up-button, QSpinBox::down-button {"
        "  width: 13px;"
        "  subcontrol-origin: border;"
        "  background: #1d2632;"
        "  border-left: 1px solid #3f4a5d;"
        "}"
        "QSpinBox::up-button:hover, QSpinBox::down-button:hover {"
        "  background: #2b3544;"
        "}"
        "QSpinBox::up-arrow, QSpinBox::down-arrow {"
        "  width: 7px;"
        "  height: 7px;"
        "}"));

    row->addWidget(nameLabel, 0, Qt::AlignVCenter);
    row->addWidget(slider, 1, Qt::AlignVCenter);
    row->addWidget(spin, 0, Qt::AlignVCenter);
    return row;
  };
  auto* historyTitle = new QLabel("カラーヒストリー", colorPanel);
  historyTitle->setVisible(false);
  historyTitle->setStyleSheet("font-weight: 600; font-size: 10px;");
  m_colorHistoryGridWidget = new QWidget(nullptr);
  m_colorHistoryGridWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  m_colorHistoryLayout = new QGridLayout(m_colorHistoryGridWidget);
  m_colorHistoryLayout->setContentsMargins(0, 0, 0, 0);
  m_colorHistoryLayout->setSpacing(0);
  m_colorHistoryLayout->setHorizontalSpacing(0);
  m_colorHistoryLayout->setVerticalSpacing(0);
  m_colorHistoryLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  m_colorHistoryLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
  m_colorHistoryButtons.clear();
  m_colorHistoryButtons.reserve(84);
  for (int i = 0; i < 84; ++i) {
    auto* chip = new QPushButton(nullptr);
    chip->setFixedSize(24, 24);
    chip->setMinimumSize(24, 24);
    chip->setMaximumSize(24, 24);
    chip->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    chip->setProperty("colorHistoryChip", true);
    chip->setStyleSheet("QPushButton { min-width: 24px; max-width: 24px; min-height: 24px; max-height: 24px; padding: 0px; margin: 0px; border: 1px solid #2f3746; border-radius: 0px; background: #202833; }QPushButton:disabled { min-width: 24px; max-width: 24px; min-height: 24px; max-height: 24px; padding: 0px; margin: 0px; border: 1px solid #2f3746; border-radius: 0px; background: #202833; }QPushButton:hover { border: 1px solid #9fb5d6; }");
    chip->setStyleSheet("QPushButton { min-width: 18px; max-width: 18px; min-height: 18px; max-height: 18px; padding: 0px; margin: 0px; border: 1px solid #343d4d; border-radius: 0px; background: #202833; }");
    chip->setToolTip("最近使った色");
    chip->setEnabled(false);
    m_colorHistoryLayout->addWidget(chip, i / 12, i % 12);
    m_colorHistoryButtons.push_back(chip);
  }
  for (int col = 0; col < 8; ++col) {
    m_colorHistoryLayout->setColumnMinimumWidth(col, 24);
    m_colorHistoryLayout->setColumnStretch(col, 0);
  }
  for (int row = 0; row < 11; ++row) {
    m_colorHistoryLayout->setRowMinimumHeight(row, 24);
    m_colorHistoryLayout->setRowStretch(row, 0);
  }
  m_colorHistoryGridWidget->setMinimumSize(1, 1);
  auto* colorMainWidget = new QWidget(nullptr);
  auto* colorMainLayout = new QVBoxLayout(colorMainWidget);
  colorMainLayout->setContentsMargins(0, 0, 0, 0);
  colorMainLayout->setSpacing(0);
  colorMainLayout->addWidget(colorTitle);
  colorMainLayout->addLayout(colorButtons);
  colorMainLayout->addWidget(m_colorWheelWidget, 1);

  auto* hsvWidget = new QWidget(nullptr);
  hsvWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  hsvWidget->setMinimumWidth(176);
  auto* hsvLayout = new QVBoxLayout(hsvWidget);
  hsvLayout->setContentsMargins(0, 0, 0, 0);
  hsvLayout->setSpacing(2);
  hsvLayout->addLayout(addHsvRow("H", m_hueSlider, m_hueSpin));
  hsvLayout->addLayout(addHsvRow("S", m_satSlider, m_satSpin));
  hsvLayout->addLayout(addHsvRow("V", m_valSlider, m_valSpin));
  hsvLayout->addLayout(addHsvRow("A", m_alphaSlider, m_alphaSpin));

  auto* historyWidget = new QWidget(nullptr);
  historyWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  historyWidget->setMaximumHeight(QWIDGETSIZE_MAX);
  auto* historyWrap = new QVBoxLayout(historyWidget);
  historyWrap->setContentsMargins(0, 0, 0, 0);
  historyWrap->setSpacing(2);
  historyWrap->addWidget(historyTitle);
  historyWrap->addWidget(m_colorHistoryGridWidget, 0, Qt::AlignLeft | Qt::AlignTop);
  QTimer::singleShot(0, this, [this]() { relayoutColorHistoryGrid(); });
  m_hueSpin->setMinimumWidth(52);
  m_satSpin->setMinimumWidth(52);
  m_valSpin->setMinimumWidth(52);
  m_alphaSpin->setMinimumWidth(52);
  m_hueSpin->setMaximumWidth(56);
  m_satSpin->setMaximumWidth(56);
  m_valSpin->setMaximumWidth(56);
  m_alphaSpin->setMaximumWidth(56);

  m_hueSpin->setStyleSheet("QSpinBox { padding-right: 12px; } QSpinBox::up-button, QSpinBox::down-button { width: 13px; }");
  m_satSpin->setStyleSheet("QSpinBox { padding-right: 12px; } QSpinBox::up-button, QSpinBox::down-button { width: 13px; }");
  m_valSpin->setStyleSheet("QSpinBox { padding-right: 12px; } QSpinBox::up-button, QSpinBox::down-button { width: 13px; }");
  m_alphaSpin->setStyleSheet("QSpinBox { padding-right: 12px; } QSpinBox::up-button, QSpinBox::down-button { width: 13px; }");

  connect(m_foregroundColorButton, &QPushButton::clicked, this, &MainWindow::onChooseForegroundColor);
  connect(m_backgroundColorButton, &QPushButton::clicked, this, &MainWindow::onChooseBackgroundColor);
  connect(swapColorButton, &QPushButton::clicked, this, &MainWindow::onSwapColors);
  connect(resetColorButton, &QPushButton::clicked, this, &MainWindow::onResetBlackWhiteColors);
  connect(transparentColorButton, &QPushButton::clicked, this, &MainWindow::onUseTransparentColor);
  connect(m_hueSlider, &QSlider::valueChanged, this, [this](int value) {
    if (m_updatingColorControls) {
      return;
    }
    m_updatingColorControls = true;
    m_hueSpin->setValue(value);
    m_updatingColorControls = false;
    applyForegroundFromHsvControls();
  });
  connect(m_hueSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
    if (m_updatingColorControls) {
      return;
    }
    m_updatingColorControls = true;
    m_hueSlider->setValue(value);
    m_updatingColorControls = false;
    applyForegroundFromHsvControls();
  });
  connect(m_satSlider, &QSlider::valueChanged, this, [this](int value) {
    if (m_updatingColorControls) {
      return;
    }
    m_updatingColorControls = true;
    m_satSpin->setValue(value);
    m_updatingColorControls = false;
    applyForegroundFromHsvControls();
  });
  connect(m_satSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
    if (m_updatingColorControls) {
      return;
    }
    m_updatingColorControls = true;
    m_satSlider->setValue(value);
    m_updatingColorControls = false;
    applyForegroundFromHsvControls();
  });
  connect(m_valSlider, &QSlider::valueChanged, this, [this](int value) {
    if (m_updatingColorControls) {
      return;
    }
    m_updatingColorControls = true;
    m_valSpin->setValue(value);
    m_updatingColorControls = false;
    applyForegroundFromHsvControls();
  });
  connect(m_valSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
    if (m_updatingColorControls) {
      return;
    }
    m_updatingColorControls = true;
    m_valSlider->setValue(value);
    m_updatingColorControls = false;
    applyForegroundFromHsvControls();
  });
  connect(m_alphaSlider, &QSlider::valueChanged, this, [this](int value) {
    if (m_updatingColorControls) {
      return;
    }
    m_updatingColorControls = true;
    m_alphaSpin->setValue(value);
    m_updatingColorControls = false;
    applyForegroundFromHsvControls();
  });
  connect(m_alphaSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
    if (m_updatingColorControls) {
      return;
    }
    m_updatingColorControls = true;
    m_alphaSlider->setValue(value);
    m_updatingColorControls = false;
    applyForegroundFromHsvControls();
  });
  connect(m_colorWheelWidget, &app::panels::ColorWheelWidget::colorChanged, this, [this](const QColor& color) {
    if (m_updatingColorControls) {
      return;
    }
    m_controller->setBrushColor(toCoreColor(color));
  });
  for (QPushButton* chip : m_colorHistoryButtons) {
    connect(chip, &QPushButton::clicked, this, [this, chip]() {
      const QVariant value = chip->property("coreColor");
      if (!value.isValid()) {
        return;
      }
      const QColor color = value.value<QColor>();
      if (!color.isValid()) {
        return;
      }
      m_controller->setBrushColor(toCoreColor(color));
    });
  }

  auto* infoPanel = new QWidget(this);
  auto* infoLayout = new QVBoxLayout(infoPanel);
  infoLayout->setContentsMargins(8, 8, 8, 8);
  infoLayout->setSpacing(6);
  auto* infoTitle = new QLabel("情報", infoPanel);
  infoTitle->setStyleSheet("font-weight: 700;");
  auto* navigatorTitle = new QLabel("ナビゲーター", infoPanel);
  navigatorTitle->setStyleSheet("font-weight: 700;");
  m_navigatorImageLabel = new QLabel(infoPanel);
  m_navigatorImageLabel->setMinimumSize(180, 120);
  m_navigatorImageLabel->setAlignment(Qt::AlignCenter);
  m_navigatorImageLabel->setStyleSheet("background:#14181f; border:1px solid #3a4453;");
  auto* navigatorButtons = new QHBoxLayout();
  navigatorButtons->setContentsMargins(0, 0, 0, 0);
  navigatorButtons->setSpacing(6);
  auto* zoom100Button = new QPushButton("100%", infoPanel);
  auto* fitButton = new QPushButton("画面に合わせる", infoPanel);
  zoom100Button->setMinimumHeight(24);
  fitButton->setMinimumHeight(24);
  navigatorButtons->addWidget(zoom100Button);
  navigatorButtons->addWidget(fitButton);
  auto* infoText = new QLabel("ツール状態やレイヤー制約はステータスバーに表示されます。", infoPanel);
  infoText->setWordWrap(true);
  infoLayout->addWidget(infoTitle);
  infoLayout->addWidget(navigatorTitle);
  infoLayout->addWidget(m_navigatorImageLabel);
  infoLayout->addLayout(navigatorButtons);
  infoLayout->addWidget(infoText);
  infoLayout->addStretch(1);
  connect(zoom100Button, &QPushButton::clicked, this, &MainWindow::onResetZoomTriggered);
  connect(fitButton, &QPushButton::clicked, this, &MainWindow::onFitToScreenTriggered);

  auto makeDock = [this](const QString& title, QWidget* widget, const char* name) {
    auto* dock = new QDockWidget(title, this);
    dock->setObjectName(name);
    dock->setWidget(widget);
    dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    return dock;
  };

  auto makeScrollable = [this](QWidget* content) {
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);
    return scroll;
  };

  m_toolDock = makeDock("ツール", makeScrollable(m_toolPanel), "ToolDock");
  m_toolSliderDock = makeDock("ツールスライダー", makeScrollable(m_quickSliderPanel), "ToolSliderDock");
  m_subToolDock = makeDock("サブツール", makeScrollable(m_subToolPanel), "SubToolDock");
  m_toolPropertyDock = makeDock("ツールプロパティ", makeScrollable(m_toolPropertyPanel), "ToolPropertyDock");
  m_colorDock = makeDock("カラー", makeScrollable(colorMainWidget), "ColorDock");
  m_colorDock->setMinimumWidth(128);
  auto* colorSliderDock = makeDock("カラースライダー", makeScrollable(hsvWidget), "ColorSliderDock");
  colorSliderDock->setMinimumWidth(188);
  auto* colorHistoryDock = makeDock("カラーヒストリー", makeScrollable(historyWidget), "ColorHistoryDock");
  colorHistoryDock->setMinimumWidth(188);
  m_layerDock = makeDock("レイヤー", m_layerPanel, "LayerDock");
  m_infoDock = makeDock("情報", infoPanel, "InfoDock");
  auto applyThinDockTitleBar = [](QDockWidget* dock) {
    if (dock == nullptr) {
      return;
    }

    auto* bar = new QWidget(dock);
    bar->setObjectName(QStringLiteral("ThinDockTitleBar"));
    bar->setFixedHeight(18);
    bar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    bar->setStyleSheet(QStringLiteral(
        "QWidget#ThinDockTitleBar {"
        " background: #202833;"
        " border-left: 1px solid #354052;"
        " border-right: 1px solid #354052;"
        " border-bottom: 1px solid #354052;"
        "}"));

    auto* row = new QHBoxLayout(bar);
    row->setContentsMargins(6, 0, 4, 0);
    row->setSpacing(3);

    auto* title = new QLabel(dock->windowTitle(), bar);
    title->setStyleSheet(QStringLiteral("color:#d8e1ef; font-size:10px; font-weight:600;"));
    title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    row->addWidget(title);

    auto makeTinyDockButton = [](const QString& text, const QString& tip, QWidget* parent) {
      auto* button = new QPushButton(text, parent);
      button->setFixedSize(14, 14);
      button->setFocusPolicy(Qt::NoFocus);
      button->setFlat(true);
      button->setToolTip(tip);
      button->setStyleSheet(QStringLiteral(
          "QPushButton {"
          " margin: 0px; padding: 0px;"
          " border: 1px solid transparent;"
          " border-radius: 2px;"
          " color: #7e8da2;"
          " background: transparent;"
          " font-size: 10px;"
          "}"
          "QPushButton:hover {"
          " border: 1px solid #4c5d73;"
          " background: #263243;"
          " color: #e4ebf6;"
          "}"));
      return button;
    };

    auto* floatButton = makeTinyDockButton(QStringLiteral("□"), QStringLiteral("パネルを分離/戻す"), bar);
    auto* closeButton = makeTinyDockButton(QStringLiteral("×"), QStringLiteral("パネルを閉じる"), bar);
    row->addWidget(floatButton);
    row->addWidget(closeButton);

    QObject::connect(floatButton, &QPushButton::clicked, dock, [dock]() {
      dock->setFloating(!dock->isFloating());
    });
    QObject::connect(closeButton, &QPushButton::clicked, dock, [dock]() {
      dock->hide();
    });

    dock->setTitleBarWidget(bar);
  };

  addDockWidget(Qt::LeftDockWidgetArea, m_toolDock);
  addDockWidget(Qt::LeftDockWidgetArea, m_toolSliderDock);
  addDockWidget(Qt::LeftDockWidgetArea, m_subToolDock);
  addDockWidget(Qt::LeftDockWidgetArea, m_toolPropertyDock);
  addDockWidget(Qt::LeftDockWidgetArea, m_colorDock);
  addDockWidget(Qt::LeftDockWidgetArea, colorSliderDock);
  addDockWidget(Qt::LeftDockWidgetArea, colorHistoryDock);
  splitDockWidget(m_toolDock, m_toolSliderDock, Qt::Horizontal);
  splitDockWidget(m_toolSliderDock, m_subToolDock, Qt::Horizontal);
  tabifyDockWidget(m_subToolDock, m_toolPropertyDock);
  tabifyDockWidget(m_subToolDock, m_colorDock);
  tabifyDockWidget(m_subToolDock, colorSliderDock);
  tabifyDockWidget(m_subToolDock, colorHistoryDock);
  resizeDocks({m_toolDock, m_toolSliderDock, m_subToolDock}, {68, 72, 220}, Qt::Horizontal);
  m_subToolDock->raise();
  applyThinDockTitleBar(m_toolDock);
  applyThinDockTitleBar(m_toolSliderDock);
  applyThinDockTitleBar(m_subToolDock);
  applyThinDockTitleBar(m_toolPropertyDock);
  applyThinDockTitleBar(m_colorDock);
  applyThinDockTitleBar(colorSliderDock);
  applyThinDockTitleBar(colorHistoryDock);

  addDockWidget(Qt::RightDockWidgetArea, m_layerDock);
  splitDockWidget(m_layerDock, m_infoDock, Qt::Vertical);
  applyThinDockTitleBar(m_layerDock);
  applyThinDockTitleBar(m_infoDock);
  resizeDocks({m_layerDock, m_infoDock}, {620, 210}, Qt::Vertical);

  m_toolDock->raise();
  m_layerDock->raise();
  m_defaultDockState = saveState();
  adjustRightDockLayout();
}

void MainWindow::createMenus() {
  auto* fileMenu = menuBar()->addMenu("ファイル(&F)");
  auto* editMenu = menuBar()->addMenu("編集(&E)");
  auto* toolMenu = menuBar()->addMenu("ツール(&T)");
  auto* selectMenu = menuBar()->addMenu("選択(&S)");
  auto* layerMenu = menuBar()->addMenu("レイヤー(&L)");
  auto* viewMenu = menuBar()->addMenu("表示(&V)");
  auto* windowMenu = menuBar()->addMenu("ウィンドウ(&W)");
  auto* helpMenu = menuBar()->addMenu("ヘルプ(&H)");

  m_newCanvasAction = new QAction("新規キャンバス(&N)", this);
  m_openAction = new QAction("開く(&O)...", this);
  m_newFromClipboardAction = new QAction("クリップボードから新規作成(&C)", this);
  m_importAsLayerAction = new QAction("画像をレイヤーとして読み込み(&I)...", this);
  m_saveAction = new QAction("保存(&S)", this);
  m_saveAsAction = new QAction("名前を付けて保存(&A)...", this);
  m_exportPngAction = new QAction("PNG書き出し(&P)...", this);
  m_exportFlattenedAction = new QAction("統合画像を書き出し(&E)...", this);
  m_exitAction = new QAction("終了(&X)", this);
  auto* closeAction = new QAction("閉じる(&C)", this);
  m_undoAction = new QAction("元に戻す(&U)", this);
  m_redoAction = new QAction("やり直し(&R)", this);
  m_cutAction = new QAction("切り取り(&T)", this);
  m_copyAction = new QAction("コピー(&C)", this);
  m_pasteAction = new QAction("貼り付け(&P)", this);
  m_deletePixelsAction = new QAction("選択ピクセルを削除(&D)", this);
  m_fillAction = new QAction("塗りつぶし(&F)", this);
  m_clearAction = new QAction("クリア(&L)", this);
  m_addLayerAction = new QAction("新規ラスターレイヤー(&R)", this);
  m_addRasterLayerAction = m_addLayerAction;
  m_addVectorLayerAction = new QAction("新規ベクターレイヤー(&V)", this);
  m_addFolderLayerAction = new QAction("新規フォルダーレイヤー(&F)", this);
  m_duplicateLayerAction = new QAction("レイヤーを複製(&D)", this);
  m_deleteLayerAction = new QAction("レイヤーを削除(&Y)", this);
  m_moveLayerUpAction = new QAction("レイヤーを上へ移動(&U)", this);
  m_moveLayerDownAction = new QAction("レイヤーを下へ移動(&W)", this);
  m_toggleLayerVisibilityAction = new QAction("表示/非表示を切替(&V)", this);
  m_mergeDownAction = new QAction("下のレイヤーと結合(&M)", this);
  m_rasterizeLayerAction = new QAction("ラスタライズ(&R)", this);
  m_toggleLayerClipAction = new QAction("クリッピングを切替(&C)", this);
  m_toggleLayerMaskAction = new QAction("マスクを切替(&K)", this);
  m_removeLayerMaskAction = new QAction("マスクを削除(&H)", this);
  m_toggleLayerLockAction = new QAction("ロックを切替(&L)", this);
  m_toggleLayerAlphaLockAction = new QAction("透明保護を切替(&A)", this);
  m_toggleLayerPositionLockAction = new QAction("位置固定を切替(&P)", this);

  m_selectAllAction = new QAction("すべて選択(&A)", this);
  m_deselectAction = new QAction("選択解除(&D)", this);
  m_clearSelectionAction = new QAction("選択範囲をクリア(&C)", this);
  m_invertSelectionAction = new QAction("選択範囲を反転(&I)", this);
  m_brushSizeDownAction = new QAction("ブラシサイズを小さく(&-)", this);
  m_brushSizeUpAction = new QAction("ブラシサイズを大きく(&+)", this);
  m_zoomInAction = new QAction("ズームイン(&I)", this);
  m_zoomOutAction = new QAction("ズームアウト(&O)", this);
  m_resetZoomAction = new QAction("ズームをリセット(&Z)", this);
  m_fitToScreenAction = new QAction("画面に合わせる(&F)", this);
  m_rotateViewLeftAction = new QAction("ビューを左回転", this);
  m_rotateViewRightAction = new QAction("ビューを右回転", this);
  m_resetViewRotationAction = new QAction("ビュー回転をリセット", this);
  m_toggleGridAction = new QAction("グリッド表示を切替(&G)", this);
  m_toggleOverlayAction = new QAction("オーバーレイ表示を切替(&O)", this);
  m_resetWorkspaceAction = new QAction("ワークスペースを初期化(&R)", this);
  m_saveWorkspaceAction = new QAction("ワークスペースを保存(&S)...", this);
  m_deleteWorkspaceAction = new QAction("ワークスペースを削除(&D)...", this);
  m_restoreLastWorkspaceAction = new QAction("前回のワークスペースを復元(&L)", this);
  auto* aboutAction = new QAction("このアプリについて(&A)", this);
  m_shortcutSummaryAction = new QAction("ショートカット一覧(&S)", this);
  m_openDocsAction = new QAction("READMEを開く(&D)", this);
  m_shortcutSettingsAction = new QAction("ショートカット設定(&K)...", this);
  m_commandPaletteAction = new QAction("コマンドパレット(&P)...", this);
  m_swapColorsAction = new QAction("描画色/背景色を入れ替え(&X)", this);
  m_resetColorsAction = new QAction("描画色/背景色を白黒に戻す(&D)", this);
  m_transparentColorAction = new QAction("透明色を使用(&T)", this);
  m_clearRecentFilesAction = new QAction("最近使ったファイルをクリア", this);

  m_recentFilesMenu = fileMenu->addMenu("最近使ったファイル");

  m_newCanvasAction->setShortcut(QKeySequence::New);
  m_openAction->setShortcut(QKeySequence::Open);
  m_newFromClipboardAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V));
  m_importAsLayerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_O));
  m_saveAction->setShortcut(QKeySequence::Save);
  m_saveAsAction->setShortcut(QKeySequence::SaveAs);
  m_exportPngAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E));
  m_exportFlattenedAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_E));
  m_exitAction->setShortcut(QKeySequence::Quit);
  closeAction->setShortcut(QKeySequence::Close);
  m_undoAction->setShortcut(QKeySequence::Undo);
  m_redoAction->setShortcuts({QKeySequence::Redo, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z)});
  m_cutAction->setShortcut(QKeySequence::Cut);
  m_copyAction->setShortcut(QKeySequence::Copy);
  m_pasteAction->setShortcut(QKeySequence::Paste);
  m_deletePixelsAction->setShortcut(QKeySequence(Qt::Key_Delete));
  m_fillAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Backspace));
  m_clearAction->setShortcut(QKeySequence(Qt::Key_Backspace));
  m_addLayerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
  m_addVectorLayerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_N));
  m_addFolderLayerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));
  m_duplicateLayerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
  m_deleteLayerAction->setShortcut(QKeySequence::Delete);
  m_moveLayerUpAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Up));
  m_moveLayerDownAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Down));
  m_toggleLayerVisibilityAction->setShortcut(QKeySequence(Qt::Key_V));
  m_mergeDownAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
  m_rasterizeLayerAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
  m_toggleLayerClipAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C));
  m_toggleLayerMaskAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_M));
  m_removeLayerMaskAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_M));
  m_toggleLayerLockAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_L));
  m_toggleLayerAlphaLockAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_L));
  m_toggleLayerPositionLockAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_P));
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
  m_rotateViewLeftAction->setShortcut(QKeySequence(Qt::Key_Comma));
  m_rotateViewRightAction->setShortcut(QKeySequence(Qt::Key_Period));
  m_resetViewRotationAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_0));
  m_toggleGridAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_QuoteLeft));
  m_toggleOverlayAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_8));
  m_resetWorkspaceAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W));
  m_saveWorkspaceAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_S));
  m_restoreLastWorkspaceAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_W));
  m_shortcutSettingsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_K));
  m_commandPaletteAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
  m_swapColorsAction->setShortcut(QKeySequence(Qt::Key_X));
  m_resetColorsAction->setShortcut(QKeySequence(Qt::Key_D));
  m_transparentColorAction->setShortcut(QKeySequence(Qt::Key_C));
  m_toggleGridAction->setCheckable(true);
  m_toggleGridAction->setChecked(m_canvasWidget->isGridVisible());
  m_toggleOverlayAction->setCheckable(true);
  m_toggleOverlayAction->setChecked(m_canvasWidget->isOverlayVisible());

  fileMenu->addAction(m_newCanvasAction);
  fileMenu->addAction(m_openAction);
  fileMenu->addAction(m_newFromClipboardAction);
  fileMenu->addAction(m_importAsLayerAction);
  fileMenu->addAction(m_saveAction);
  fileMenu->addAction(m_saveAsAction);
  fileMenu->addSeparator();
  fileMenu->addAction(m_exportPngAction);
  fileMenu->addAction(m_exportFlattenedAction);
  if (m_recentFilesMenu != nullptr) {
    rebuildRecentFilesMenu();
    fileMenu->addMenu(m_recentFilesMenu);
  }
  fileMenu->addSeparator();
  fileMenu->addAction(closeAction);
  fileMenu->addAction(m_exitAction);
  editMenu->addAction(m_undoAction);
  editMenu->addAction(m_redoAction);
  editMenu->addSeparator();
  editMenu->addAction(m_cutAction);
  editMenu->addAction(m_copyAction);
  editMenu->addAction(m_pasteAction);
  editMenu->addAction(m_deletePixelsAction);
  editMenu->addAction(m_fillAction);
  editMenu->addAction(m_clearAction);
  editMenu->addSeparator();
  editMenu->addAction(m_brushSizeDownAction);
  editMenu->addAction(m_brushSizeUpAction);

  auto* toolGroup = new QActionGroup(this);
  toolGroup->setExclusive(true);
  auto bindTool = [&](core::ToolKind kind, const QString& text, const QKeySequence& shortcut) {
    QAction* action = createToolAction(toolMenu, kind, text, shortcut);
    action->setActionGroup(toolGroup);
  };
  bindTool(core::ToolKind::Brush, "ブラシ(&B)", QKeySequence(Qt::Key_B));
  bindTool(core::ToolKind::Eraser, "消しゴム(&E)", QKeySequence(Qt::Key_E));
  bindTool(core::ToolKind::Eyedropper, "スポイト(&I)", QKeySequence(Qt::Key_I));
  bindTool(core::ToolKind::Fill, "塗りつぶし(&G)", QKeySequence(Qt::Key_G));
  bindTool(core::ToolKind::Line, "直線(&U)", QKeySequence(Qt::Key_U));
  bindTool(core::ToolKind::RectSelection, "選択(&R)", QKeySequence(Qt::Key_R));
  bindTool(core::ToolKind::MoveLayer, "移動(&M)", QKeySequence(Qt::Key_M));
  bindTool(core::ToolKind::Hand, "手のひら(&H)", QKeySequence(Qt::Key_H));
  bindTool(core::ToolKind::Zoom, "ズーム(&Z)", QKeySequence(Qt::Key_Z));

  selectMenu->addAction(m_clearSelectionAction);
  selectMenu->addAction(m_selectAllAction);
  selectMenu->addAction(m_deselectAction);
  selectMenu->addAction(m_invertSelectionAction);

  toolMenu->addSeparator();
  toolMenu->addAction(m_swapColorsAction);
  toolMenu->addAction(m_resetColorsAction);
  toolMenu->addAction(m_transparentColorAction);

  layerMenu->addAction(m_addRasterLayerAction);
  layerMenu->addAction(m_addVectorLayerAction);
  layerMenu->addAction(m_addFolderLayerAction);
  layerMenu->addAction(m_duplicateLayerAction);
  layerMenu->addAction(m_deleteLayerAction);
  layerMenu->addAction(m_mergeDownAction);
  layerMenu->addAction(m_rasterizeLayerAction);
  layerMenu->addSeparator();
  layerMenu->addAction(m_toggleLayerClipAction);
  layerMenu->addAction(m_toggleLayerMaskAction);
  layerMenu->addAction(m_removeLayerMaskAction);
  layerMenu->addAction(m_toggleLayerLockAction);
  layerMenu->addAction(m_toggleLayerAlphaLockAction);
  layerMenu->addAction(m_toggleLayerPositionLockAction);
  layerMenu->addSeparator();
  layerMenu->addAction(m_moveLayerUpAction);
  layerMenu->addAction(m_moveLayerDownAction);
  layerMenu->addAction(m_toggleLayerVisibilityAction);

  viewMenu->addAction(m_zoomInAction);
  viewMenu->addAction(m_zoomOutAction);
  viewMenu->addAction(m_resetZoomAction);
  viewMenu->addAction(m_fitToScreenAction);
  viewMenu->addSeparator();
  viewMenu->addAction(m_rotateViewLeftAction);
  viewMenu->addAction(m_rotateViewRightAction);
  viewMenu->addAction(m_resetViewRotationAction);
  viewMenu->addSeparator();
  viewMenu->addAction(m_toggleGridAction);
  viewMenu->addAction(m_toggleOverlayAction);

  if (m_toolDock != nullptr) {
    windowMenu->addAction(m_toolDock->toggleViewAction());
  }
  if (m_toolSliderDock != nullptr) {
    windowMenu->addAction(m_toolSliderDock->toggleViewAction());
  }
  if (m_subToolDock != nullptr) {
    windowMenu->addAction(m_subToolDock->toggleViewAction());
  } else if (m_subToolPanel != nullptr) {
    auto* action = windowMenu->addAction("サブツール");
    action->setCheckable(true);
    action->setChecked(m_subToolPanel->isVisible());
    connect(action, &QAction::toggled, m_subToolPanel, &QWidget::setVisible);
  }
  if (m_toolPropertyDock != nullptr) {
    windowMenu->addAction(m_toolPropertyDock->toggleViewAction());
  } else if (m_toolPropertyPanel != nullptr) {
    auto* action = windowMenu->addAction("ツールプロパティ");
    action->setCheckable(true);
    action->setChecked(m_toolPropertyPanel->isVisible());
    connect(action, &QAction::toggled, m_toolPropertyPanel, &QWidget::setVisible);
  }
  if (m_colorDock != nullptr) {
    windowMenu->addAction(m_colorDock->toggleViewAction());
  } else if (m_colorPanelWidget != nullptr) {
    auto* action = windowMenu->addAction("カラー");
    action->setCheckable(true);
    action->setChecked(m_colorPanelWidget->isVisible());
    connect(action, &QAction::toggled, m_colorPanelWidget, &QWidget::setVisible);
  }
  if (m_layerDock != nullptr) {
    windowMenu->addAction(m_layerDock->toggleViewAction());
  }
  if (m_infoDock != nullptr) {
    windowMenu->addAction(m_infoDock->toggleViewAction());
  }
  windowMenu->addSeparator();
  m_workspaceLayoutsMenu = windowMenu->addMenu("ワークスペースを読み込み");
  windowMenu->addAction(m_saveWorkspaceAction);
  windowMenu->addAction(m_deleteWorkspaceAction);
  windowMenu->addAction(m_restoreLastWorkspaceAction);
  windowMenu->addAction(m_resetWorkspaceAction);
  windowMenu->addSeparator();
  windowMenu->addAction(m_commandPaletteAction);
  rebuildWorkspaceLayoutsMenu();

  helpMenu->addAction(aboutAction);
  helpMenu->addAction(m_shortcutSummaryAction);
  helpMenu->addAction(m_shortcutSettingsAction);
  helpMenu->addAction(m_openDocsAction);

  connect(m_newCanvasAction, &QAction::triggered, this, &MainWindow::onNewCanvas);
  connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpenTriggered);
  connect(m_newFromClipboardAction, &QAction::triggered, this, &MainWindow::onNewFromClipboardTriggered);
  connect(m_importAsLayerAction, &QAction::triggered, this, &MainWindow::onImportAsLayerTriggered);
  connect(m_saveAction, &QAction::triggered, this, &MainWindow::onSaveTriggered);
  connect(m_saveAsAction, &QAction::triggered, this, &MainWindow::onSaveAsTriggered);
  connect(m_exportPngAction, &QAction::triggered, this, &MainWindow::onExportPngTriggered);
  connect(m_exportFlattenedAction, &QAction::triggered, this, &MainWindow::onExportFlattenedTriggered);
  connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
  connect(closeAction, &QAction::triggered, this, &QWidget::close);
  connect(m_undoAction, &QAction::triggered, this, &MainWindow::onUndoTriggered);
  connect(m_redoAction, &QAction::triggered, this, &MainWindow::onRedoTriggered);
  connect(m_cutAction, &QAction::triggered, this, &MainWindow::onCutTriggered);
  connect(m_copyAction, &QAction::triggered, this, &MainWindow::onCopyTriggered);
  connect(m_pasteAction, &QAction::triggered, this, &MainWindow::onPasteTriggered);
  connect(m_deletePixelsAction, &QAction::triggered, this, &MainWindow::onDeletePixelsTriggered);
  connect(m_fillAction, &QAction::triggered, this, &MainWindow::onFillTriggered);
  connect(m_clearAction, &QAction::triggered, this, &MainWindow::onDeletePixelsTriggered);
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
  connect(m_addFolderLayerAction, &QAction::triggered, this, &MainWindow::onAddFolderLayerTriggered);
  connect(m_duplicateLayerAction, &QAction::triggered, this, &MainWindow::onDuplicateLayerTriggered);
  connect(m_deleteLayerAction, &QAction::triggered, this, &MainWindow::onDeleteLayerTriggered);
  connect(m_mergeDownAction, &QAction::triggered, this, &MainWindow::onMergeDownTriggered);
  connect(m_rasterizeLayerAction, &QAction::triggered, this, &MainWindow::onRasterizeLayerTriggered);
  connect(m_toggleLayerClipAction, &QAction::triggered, this, &MainWindow::onToggleLayerClipTriggered);
  connect(m_toggleLayerMaskAction, &QAction::triggered, this, &MainWindow::onToggleLayerMaskTriggered);
  connect(m_removeLayerMaskAction, &QAction::triggered, this, &MainWindow::onRemoveLayerMaskTriggered);
  connect(m_toggleLayerLockAction, &QAction::triggered, this, &MainWindow::onToggleLayerLockTriggered);
  connect(m_toggleLayerAlphaLockAction, &QAction::triggered, this, &MainWindow::onToggleLayerAlphaLockTriggered);
  connect(
      m_toggleLayerPositionLockAction,
      &QAction::triggered,
      this,
      &MainWindow::onToggleLayerPositionLockTriggered);
  connect(m_zoomInAction, &QAction::triggered, this, &MainWindow::onZoomInTriggered);
  connect(m_zoomOutAction, &QAction::triggered, this, &MainWindow::onZoomOutTriggered);
  connect(m_resetZoomAction, &QAction::triggered, this, &MainWindow::onResetZoomTriggered);
  connect(m_fitToScreenAction, &QAction::triggered, this, &MainWindow::onFitToScreenTriggered);
  connect(m_rotateViewLeftAction, &QAction::triggered, this, &MainWindow::onRotateViewLeftTriggered);
  connect(m_rotateViewRightAction, &QAction::triggered, this, &MainWindow::onRotateViewRightTriggered);
  connect(m_resetViewRotationAction, &QAction::triggered, this, &MainWindow::onResetViewRotationTriggered);
  connect(m_toggleGridAction, &QAction::toggled, m_canvasWidget, &app::canvasview::CanvasWidget::setGridVisible);
  connect(m_toggleOverlayAction, &QAction::toggled, m_canvasWidget, &app::canvasview::CanvasWidget::setOverlayVisible);
  connect(m_resetWorkspaceAction, &QAction::triggered, this, &MainWindow::onResetWorkspaceTriggered);
  connect(m_saveWorkspaceAction, &QAction::triggered, this, &MainWindow::onSaveWorkspaceTriggered);
  connect(m_deleteWorkspaceAction, &QAction::triggered, this, &MainWindow::onDeleteWorkspaceTriggered);
  connect(m_restoreLastWorkspaceAction, &QAction::triggered, this, &MainWindow::onRestoreLastWorkspaceTriggered);
  connect(m_commandPaletteAction, &QAction::triggered, this, &MainWindow::onCommandPaletteTriggered);
  connect(m_swapColorsAction, &QAction::triggered, this, &MainWindow::onSwapColors);
  connect(m_resetColorsAction, &QAction::triggered, this, &MainWindow::onResetBlackWhiteColors);
  connect(m_transparentColorAction, &QAction::triggered, this, &MainWindow::onUseTransparentColor);
  connect(m_clearRecentFilesAction, &QAction::triggered, this, [this]() {
    m_recentFiles.clear();
    rebuildRecentFilesMenu();
  });
  connect(aboutAction, &QAction::triggered, this, [this]() {
    statusBar()->showMessage("自作イラストアプリ - ラスタ/ベクター基盤", 4000);
  });
  connect(m_shortcutSummaryAction, &QAction::triggered, this, [this]() {
    QMessageBox::information(
        this,
        "ショートカット一覧",
        "B ブラシ\nE 消しゴム\nI スポイト\nG 塗りつぶし\nU 直線\nR 選択\nM 移動\nH 手のひら\nZ ズーム\n[ ] ブラシサイズ\nCtrl+Z / Ctrl+Y 元に戻す/やり直し\nSpace+ドラッグ / 中ボタン+ドラッグ 一時パン");
  });
  connect(m_shortcutSettingsAction, &QAction::triggered, this, &MainWindow::onShortcutSettingsTriggered);
  connect(m_openDocsAction, &QAction::triggered, this, []() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::currentPath() + "/README.md"));
  });

  m_toolStatusLabel = new QLabel("ツール: ブラシ", this);
  m_toolStatusLabel->setObjectName("ToolStatusLabel");
  m_subToolStatusLabel = new QLabel("サブツール: 通常", this);
  m_subToolStatusLabel->setObjectName("SubToolStatusLabel");
  m_guideStatusLabel = new QLabel("操作: 左ドラッグで描画 / ホイールでサイズ / 中ボタンドラッグで一時パン", this);
  m_guideStatusLabel->setObjectName("ToolGuideStatusLabel");
  m_colorStatusLabel = new QLabel("色: #000000", this);
  m_colorStatusLabel->setObjectName("BrushColorStatusLabel");
  m_sizeStatusLabel = new QLabel("サイズ: 8", this);
  m_sizeStatusLabel->setObjectName("BrushSizeStatusLabel");
  m_activeLayerStatusLabel = new QLabel("レイヤー: Layer 1", this);
  m_activeLayerStatusLabel->setObjectName("ActiveLayerStatusLabel");
  m_zoomStatusLabel = new QLabel("ズーム: 100%", this);
  m_zoomStatusLabel->setObjectName("ZoomStatusLabel");
  m_selectionStatusLabel = new QLabel("選択: OFF", this);
  m_selectionStatusLabel->setObjectName("SelectionStatusLabel");
  m_objectSelectionStatusLabel = new QLabel("selected object: (none)", this);
  m_objectSelectionStatusLabel->setObjectName("ObjectSelectionStatusLabel");

  statusBar()->addWidget(m_toolStatusLabel);
  statusBar()->addWidget(m_subToolStatusLabel);
  statusBar()->addWidget(m_guideStatusLabel, 1);
  statusBar()->addPermanentWidget(m_colorStatusLabel);
  statusBar()->addPermanentWidget(m_sizeStatusLabel);
  statusBar()->addPermanentWidget(m_zoomStatusLabel);
  statusBar()->addPermanentWidget(m_selectionStatusLabel);
  statusBar()->addPermanentWidget(m_objectSelectionStatusLabel, 1);
  statusBar()->addPermanentWidget(m_activeLayerStatusLabel);

  const auto markCommand = [](QAction* action, const QString& id) {
    if (action == nullptr) {
      return;
    }
    action->setProperty("commandId", id);
    action->setProperty("defaultShortcut", action->shortcut().toString(QKeySequence::PortableText));
  };
  markCommand(m_newCanvasAction, "file.new");
  markCommand(m_openAction, "file.open");
  markCommand(m_newFromClipboardAction, "file.new_from_clipboard");
  markCommand(m_importAsLayerAction, "file.import_as_layer");
  markCommand(m_saveAction, "file.save");
  markCommand(m_saveAsAction, "file.save_as");
  markCommand(m_exportPngAction, "file.export_png");
  markCommand(m_exportFlattenedAction, "file.export_flattened");
  markCommand(m_clearRecentFilesAction, "file.clear_recent");
  markCommand(m_exitAction, "file.exit");
  markCommand(m_undoAction, "edit.undo");
  markCommand(m_redoAction, "edit.redo");
  markCommand(m_cutAction, "edit.cut");
  markCommand(m_copyAction, "edit.copy");
  markCommand(m_pasteAction, "edit.paste");
  markCommand(m_deletePixelsAction, "edit.delete_pixels");
  markCommand(m_fillAction, "edit.fill");
  markCommand(m_clearAction, "edit.clear");
  markCommand(m_brushSizeDownAction, "edit.brush_size_down");
  markCommand(m_brushSizeUpAction, "edit.brush_size_up");
  markCommand(m_selectAllAction, "select.all");
  markCommand(m_deselectAction, "select.deselect");
  markCommand(m_clearSelectionAction, "select.clear");
  markCommand(m_invertSelectionAction, "select.invert");
  markCommand(m_addRasterLayerAction, "layer.new_raster");
  markCommand(m_addVectorLayerAction, "layer.new_vector");
  markCommand(m_addFolderLayerAction, "layer.new_folder");
  markCommand(m_duplicateLayerAction, "layer.duplicate");
  markCommand(m_deleteLayerAction, "layer.delete");
  markCommand(m_moveLayerUpAction, "layer.move_up");
  markCommand(m_moveLayerDownAction, "layer.move_down");
  markCommand(m_toggleLayerVisibilityAction, "layer.toggle_visibility");
  markCommand(m_mergeDownAction, "layer.merge_down");
  markCommand(m_rasterizeLayerAction, "layer.rasterize");
  markCommand(m_toggleLayerClipAction, "layer.toggle_clipping");
  markCommand(m_toggleLayerMaskAction, "layer.toggle_mask");
  markCommand(m_removeLayerMaskAction, "layer.remove_mask");
  markCommand(m_toggleLayerLockAction, "layer.toggle_lock");
  markCommand(m_toggleLayerAlphaLockAction, "layer.toggle_alpha_lock");
  markCommand(m_toggleLayerPositionLockAction, "layer.toggle_position_lock");
  markCommand(m_zoomInAction, "view.zoom_in");
  markCommand(m_zoomOutAction, "view.zoom_out");
  markCommand(m_resetZoomAction, "view.zoom_reset");
  markCommand(m_fitToScreenAction, "view.fit_screen");
  markCommand(m_rotateViewLeftAction, "view.rotate_left");
  markCommand(m_rotateViewRightAction, "view.rotate_right");
  markCommand(m_resetViewRotationAction, "view.rotate_reset");
  markCommand(m_toggleGridAction, "view.toggle_grid");
  markCommand(m_toggleOverlayAction, "view.toggle_overlay");
  markCommand(m_resetWorkspaceAction, "window.reset_workspace");
  markCommand(m_saveWorkspaceAction, "window.save_workspace");
  markCommand(m_deleteWorkspaceAction, "window.delete_workspace");
  markCommand(m_restoreLastWorkspaceAction, "window.restore_last_workspace");
  markCommand(m_commandPaletteAction, "window.command_palette");
  markCommand(m_shortcutSettingsAction, "help.shortcut_settings");
  markCommand(m_swapColorsAction, "color.swap");
  markCommand(m_resetColorsAction, "color.reset_bw");
  markCommand(m_transparentColorAction, "color.transparent");
  for (const auto& [kind, action] : m_toolActions) {
    if (action != nullptr) {
      markCommand(action, QString("tool.%1").arg(static_cast<int>(kind)));
    }
  }
}

void MainWindow::createToolBar() {
  m_quickToolBar = addToolBar("クイック操作");
  m_quickToolBar->setMovable(false);
  m_quickToolBar->setFloatable(false);
  m_quickToolBar->setAllowedAreas(Qt::TopToolBarArea);
  m_quickToolBar->setIconSize(QSize(18, 18));
  m_quickToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  m_quickToolBar->setContentsMargins(2, 1, 2, 1);
  m_quickToolBar->setToolTip("主要操作（ファイル / 履歴 / レイヤー / 表示）");
  m_quickToolBar->setStyleSheet(
      "QToolBar { spacing: 2px; border: none; }"
      "QToolButton { min-width: 24px; min-height: 24px; padding: 2px; border-radius: 3px; }");

  m_newCanvasAction->setIcon(app::ui::icon("new_file"));
  m_openAction->setIcon(app::ui::icon("open"));
  m_saveAction->setIcon(app::ui::icon("save"));
  m_exportPngAction->setIcon(app::ui::icon("export"));
  m_undoAction->setIcon(app::ui::icon("undo"));
  m_redoAction->setIcon(app::ui::icon("redo"));
  m_addRasterLayerAction->setIcon(app::ui::icon("layer_add"));
  m_addVectorLayerAction->setIcon(app::ui::icon("vector_add"));
  m_addFolderLayerAction->setIcon(app::ui::icon("folder_add"));
  m_duplicateLayerAction->setIcon(app::ui::icon("duplicate"));
  m_deleteLayerAction->setIcon(app::ui::icon("delete"));
  m_toggleLayerVisibilityAction->setIcon(app::ui::icon("visibility"));
  m_moveLayerUpAction->setIcon(app::ui::icon("up"));
  m_moveLayerDownAction->setIcon(app::ui::icon("down"));
  m_zoomInAction->setIcon(app::ui::icon("zoom_in"));
  m_zoomOutAction->setIcon(app::ui::icon("zoom_out"));
  m_resetZoomAction->setIcon(app::ui::icon("zoom_reset"));
  m_fitToScreenAction->setIcon(app::ui::icon("fit"));
  m_rotateViewLeftAction->setIcon(app::ui::icon("undo"));
  m_rotateViewRightAction->setIcon(app::ui::icon("redo"));
  m_resetViewRotationAction->setIcon(app::ui::icon("zoom_reset"));
  m_commandPaletteAction->setIcon(app::ui::icon("command_palette"));

  m_newCanvasAction->setToolTip("新規キャンバス");
  m_openAction->setToolTip("開く");
  m_saveAction->setToolTip("保存");
  m_undoAction->setToolTip("元に戻す");
  m_redoAction->setToolTip("やり直し");
  m_commandPaletteAction->setToolTip("コマンドパレット（Ctrl+Shift+P）");
  m_addRasterLayerAction->setToolTip("新規ラスターレイヤー");
  m_addVectorLayerAction->setToolTip("新規ベクターレイヤー");
  m_deleteLayerAction->setToolTip("レイヤー削除");
  m_zoomInAction->setToolTip("ズームイン");
  m_zoomOutAction->setToolTip("ズームアウト");
  m_resetZoomAction->setToolTip("ズームを100%に戻す");
  m_fitToScreenAction->setToolTip("画面に合わせる");
  m_rotateViewLeftAction->setToolTip("表示ビューを左に15度回転");
  m_rotateViewRightAction->setToolTip("表示ビューを右に15度回転");
  m_resetViewRotationAction->setToolTip("表示ビューの回転を戻す");

  m_quickToolBar->addAction(m_newCanvasAction);
  m_quickToolBar->addAction(m_openAction);
  m_quickToolBar->addAction(m_saveAction);
  m_quickToolBar->addSeparator();
  m_quickToolBar->addAction(m_undoAction);
  m_quickToolBar->addAction(m_redoAction);
  m_quickToolBar->addAction(m_commandPaletteAction);
  m_quickToolBar->addSeparator();
  m_quickToolBar->addAction(m_addRasterLayerAction);
  m_quickToolBar->addAction(m_addVectorLayerAction);
  m_quickToolBar->addAction(m_addFolderLayerAction);
  m_quickToolBar->addAction(m_duplicateLayerAction);
  m_quickToolBar->addAction(m_deleteLayerAction);
  m_quickToolBar->addSeparator();
  m_quickToolBar->addAction(m_zoomInAction);
  m_quickToolBar->addAction(m_zoomOutAction);
  m_quickToolBar->addAction(m_resetZoomAction);
  m_quickToolBar->addAction(m_fitToScreenAction);
  m_quickToolBar->addAction(m_rotateViewLeftAction);
  m_quickToolBar->addAction(m_rotateViewRightAction);
}

void MainWindow::adjustRightDockLayout() {
  if (m_layerDock == nullptr || m_infoDock == nullptr) {
    return;
  }

  const int availableHeight = std::max(240, height() - menuBar()->height() - statusBar()->height());
  if (availableHeight < 680) {
    m_infoDock->setMaximumHeight(150);
    resizeDocks({m_layerDock, m_infoDock}, {availableHeight - 160, 140}, Qt::Vertical);
  } else {
    m_infoDock->setMaximumHeight(QWIDGETSIZE_MAX);
    resizeDocks({m_layerDock, m_infoDock}, {static_cast<int>(availableHeight * 0.72), static_cast<int>(availableHeight * 0.28)}, Qt::Vertical);
  }
}

void MainWindow::resizeEvent(QResizeEvent* event) {
  QMainWindow::resizeEvent(event);
  adjustRightDockLayout();
  relayoutColorHistoryGrid();
}

void MainWindow::applyUiChrome() {
  // ---------------------------------------------------------------
  // グローバルQSS — 制作ソフト風ダークテーマ
  // 個別ウィジェットのsetStyleSheet()はこれより優先される
  // ---------------------------------------------------------------
  setStyleSheet(
      // ===== BASE =====
      "QMainWindow { background: #14171d; color: #dfe4ee; }"
      "QWidget { color: #d8e2f0; }"

      // ===== DOCK =====
      "QDockWidget { color: #c8d4e4; font-size: 11px; }"
      "QDockWidget::title {"
      "  background: #1e2430;"
      "  border-bottom: 1px solid #2c3848;"
      "  padding: 3px 8px;"
      "  font-weight: 600;"
      "  font-size: 11px;"
      "}"
      "QDockWidget > QWidget { background: #1c2028; }"

      // ===== TABS (ドックのタブ切替) =====
      "QTabWidget::pane {"
      "  border: none;"
      "  border-top: 1px solid #2c3848;"
      "  background: #1c2028;"
      "}"
      "QTabBar { background: #181d26; }"
      "QTabBar::tab {"
      "  background: #181d26;"
      "  color: #7a8898;"
      "  border: none;"
      "  border-right: 1px solid #252e3c;"
      "  padding: 4px 10px;"
      "  min-width: 48px;"
      "  min-height: 20px;"
      "  font-size: 10px;"
      "  font-weight: 600;"
      "}"
      "QTabBar::tab:selected {"
      "  background: #1c2028;"
      "  color: #dce8f8;"
      "  border-top: 2px solid #4a84c8;"
      "}"
      "QTabBar::tab:hover:!selected {"
      "  background: #1e2534;"
      "  color: #b4c4d8;"
      "}"

      // ===== GROUPBOX =====
      "QGroupBox {"
      "  border: 1px solid #2a3344;"
      "  border-radius: 3px;"
      "  margin-top: 14px;"
      "  padding-top: 6px;"
      "  background: transparent;"
      "}"
      "QGroupBox::title {"
      "  subcontrol-origin: margin;"
      "  subcontrol-position: top left;"
      "  left: 8px;"
      "  padding: 0 4px;"
      "  color: #7a8ea8;"
      "  font-size: 10px;"
      "  font-weight: 600;"
      "}"

      // ===== LIST / TREE =====
      "QListWidget, QTreeWidget {"
      "  background: #171b22;"
      "  border: 1px solid #2c3646;"
      "  color: #d8e2f0;"
      "  outline: none;"
      "}"
      "QListWidget::item { min-height: 22px; }"
      "QListWidget::item:selected, QTreeWidget::item:selected {"
      "  background: #244070;"
      "  color: #e8f0ff;"
      "}"
      "QListWidget::item:hover, QTreeWidget::item:hover {"
      "  background: #1e2b3e;"
      "}"

      // ===== BUTTONS =====
      "QPushButton, QToolButton {"
      "  background: #252d3a;"
      "  border: 1px solid #3a4a60;"
      "  border-radius: 3px;"
      "  padding: 3px 8px;"
      "  min-height: 22px;"
      "  color: #d0dcea;"
      "  font-size: 11px;"
      "}"
      "QPushButton:hover, QToolButton:hover {"
      "  background: #2e3a4c;"
      "  border-color: #5a7aa0;"
      "  color: #e8f0ff;"
      "}"
      "QPushButton:pressed, QToolButton:pressed {"
      "  background: #1c3052;"
      "  border-color: #5a98d8;"
      "  color: #e8f4ff;"
      "}"
      "QPushButton:checked, QToolButton:checked {"
      "  background: #1a3a62;"
      "  border-color: #3a78c8;"
      "  color: #c8e0ff;"
      "}"
      "QPushButton:checked:hover, QToolButton:checked:hover {"
      "  background: #204878;"
      "  border-color: #5a98e0;"
      "}"
      "QPushButton:disabled, QToolButton:disabled {"
      "  background: #1e2430;"
      "  color: #505c6e;"
      "  border-color: #2c3444;"
      "}"
      "QPushButton:flat, QToolButton:autoRaise {"
      "  background: transparent;"
      "  border: 1px solid transparent;"
      "}"
      "QPushButton:flat:hover { background: #2a3448; border-color: #3a4c64; }"

      // ===== MENUBAR / MENU =====
      "QMenuBar {"
      "  background: #181c24;"
      "  color: #c0cede;"
      "  border-bottom: 1px solid #252e3e;"
      "  font-size: 12px;"
      "  padding: 1px 0;"
      "}"
      "QMenuBar::item { padding: 3px 10px; border-radius: 2px; }"
      "QMenuBar::item:selected { background: #263044; color: #e0eaff; }"
      "QMenu {"
      "  background: #1a1f28;"
      "  color: #d0dcea;"
      "  border: 1px solid #2e3a4e;"
      "  padding: 2px 0;"
      "  font-size: 12px;"
      "}"
      "QMenu::item { padding: 4px 24px 4px 16px; min-width: 120px; }"
      "QMenu::item:selected { background: #244070; color: #e8f0ff; }"
      "QMenu::item:disabled { color: #505c6e; }"
      "QMenu::separator { height: 1px; background: #2a3448; margin: 3px 8px; }"

      // ===== TOOLBAR =====
      "QToolBar {"
      "  background: #181c24;"
      "  border-bottom: 1px solid #252e3e;"
      "  spacing: 2px;"
      "  padding: 2px 4px;"
      "}"
      "QToolBar::separator {"
      "  background: #2e3a4e;"
      "  width: 1px;"
      "  margin: 4px 3px;"
      "}"

      // ===== INPUT FIELDS =====
      "QLineEdit, QSpinBox, QComboBox {"
      "  background: #141820;"
      "  border: 1px solid #333e52;"
      "  color: #d4e0f0;"
      "  min-height: 22px;"
      "  border-radius: 2px;"
      "  padding-left: 4px;"
      "  selection-background-color: #244070;"
      "}"
      "QLineEdit:focus, QSpinBox:focus, QComboBox:focus {"
      "  border-color: #4a7ec0;"
      "  background: #16202e;"
      "}"
      "QSpinBox::up-button, QSpinBox::down-button {"
      "  background: #1e2632;"
      "  border-left: 1px solid #333e52;"
      "  width: 14px;"
      "}"
      "QSpinBox::up-button:hover, QSpinBox::down-button:hover {"
      "  background: #28364a;"
      "}"
      "QComboBox::drop-down {"
      "  border: none;"
      "  width: 20px;"
      "  background: #1e2632;"
      "  border-left: 1px solid #333e52;"
      "}"
      "QComboBox QAbstractItemView {"
      "  background: #181e28;"
      "  border: 1px solid #2e3a4e;"
      "  selection-background-color: #244070;"
      "  selection-color: #e8f0ff;"
      "  color: #c8d8ec;"
      "  outline: none;"
      "}"

      // ===== SLIDER =====
      "QSlider::groove:horizontal {"
      "  background: #1c2230;"
      "  height: 6px;"
      "  border-radius: 3px;"
      "}"
      "QSlider::sub-page:horizontal {"
      "  background: #2e5c9a;"
      "  height: 6px;"
      "  border-radius: 3px;"
      "}"
      "QSlider::handle:horizontal {"
      "  background: #7aaae0;"
      "  width: 12px;"
      "  height: 12px;"
      "  margin: -3px 0;"
      "  border-radius: 6px;"
      "  border: 1px solid #4a78b8;"
      "}"
      "QSlider::handle:horizontal:hover { background: #9cc0f0; }"
      "QSlider::groove:vertical {"
      "  background: #1c2230;"
      "  width: 6px;"
      "  border-radius: 3px;"
      "}"
      "QSlider::sub-page:vertical {"
      "  background: #2e5c9a;"
      "  width: 6px;"
      "  border-radius: 3px;"
      "}"
      "QSlider::handle:vertical {"
      "  background: #7aaae0;"
      "  width: 12px;"
      "  height: 12px;"
      "  margin: 0 -3px;"
      "  border-radius: 6px;"
      "  border: 1px solid #4a78b8;"
      "}"

      // ===== SCROLLBAR =====
      "QScrollBar:vertical {"
      "  background: #14181f;"
      "  width: 7px;"
      "  border: none;"
      "  margin: 0;"
      "}"
      "QScrollBar::handle:vertical {"
      "  background: #38465a;"
      "  min-height: 24px;"
      "  border-radius: 3px;"
      "  margin: 1px;"
      "}"
      "QScrollBar::handle:vertical:hover { background: #526a88; }"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
      "  height: 0; border: none;"
      "}"
      "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
      "  background: none;"
      "}"
      "QScrollBar:horizontal {"
      "  background: #14181f;"
      "  height: 7px;"
      "  border: none;"
      "  margin: 0;"
      "}"
      "QScrollBar::handle:horizontal {"
      "  background: #38465a;"
      "  min-width: 24px;"
      "  border-radius: 3px;"
      "  margin: 1px;"
      "}"
      "QScrollBar::handle:horizontal:hover { background: #526a88; }"
      "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
      "  width: 0; border: none;"
      "}"
      "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {"
      "  background: none;"
      "}"

      // ===== SPLITTER =====
      "QSplitter::handle { background: #1e2430; }"
      "QSplitter::handle:horizontal { width: 2px; }"
      "QSplitter::handle:vertical { height: 2px; }"
      "QSplitter::handle:hover { background: #3a5a8a; }"

      // ===== CHECKBOX =====
      "QCheckBox { color: #b8c8dc; spacing: 6px; font-size: 11px; }"
      "QCheckBox::indicator {"
      "  width: 13px; height: 13px;"
      "  border: 1px solid #3a4a60;"
      "  border-radius: 2px;"
      "  background: #16202c;"
      "}"
      "QCheckBox::indicator:checked {"
      "  background: #245098;"
      "  border-color: #4078c8;"
      "}"
      "QCheckBox::indicator:hover { border-color: #5888b8; }"
      "QCheckBox::indicator:disabled { background: #1c2430; border-color: #2a3444; }"

      // ===== STATUS BAR =====
      "QStatusBar {"
      "  background: #14181f;"
      "  border-top: 1px solid #252e3e;"
      "  color: #7888a0;"
      "  font-size: 11px;"
      "}"
      "QStatusBar::item { border: none; }"
      "QStatusBar QLabel {"
      "  color: #7888a0;"
      "  font-size: 11px;"
      "  padding: 0 4px;"
      "}"

      // ===== SCROLL AREA =====
      "QScrollArea { border: none; background: transparent; }"

      // ===== TOOLTIP =====
      "QToolTip {"
      "  background: #1a2336;"
      "  color: #c8dcf4;"
      "  border: 1px solid #384e6e;"
      "  padding: 3px 7px;"
      "  font-size: 11px;"
      "  border-radius: 3px;"
      "}"

      // ===== FRAME / SEPARATOR =====
      "QFrame[frameShape=\"4\"] { color: #28344a; }"  // HLine
      "QFrame[frameShape=\"5\"] { color: #28344a; }"  // VLine
  );
}

void MainWindow::updateUndoRedoState() {
  if (m_undoAction == nullptr || m_redoAction == nullptr) {
    return;
  }

  const bool canUndo = m_controller->canUndo();
  const bool canRedo = m_controller->canRedo();

  if (canUndo) {
    const QString label = QString::fromStdString(m_controller->nextUndoActionName());
    m_undoAction->setText(label.isEmpty() ? "元に戻す(&U)" : QString("元に戻す(&U) %1").arg(label));
  } else {
    m_undoAction->setText("元に戻す(&U)");
  }

  if (canRedo) {
    const QString label = QString::fromStdString(m_controller->nextRedoActionName());
    m_redoAction->setText(label.isEmpty() ? "やり直し(&R)" : QString("やり直し(&R) %1").arg(label));
  } else {
    m_redoAction->setText("やり直し(&R)");
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
    m_activeLayerStatusLabel->setText("レイヤー: （なし）");
    return;
  }
  const std::size_t active = doc.activeLayerIndex();
  const core::Layer& layer = doc.layerAt(active);
  m_activeLayerStatusLabel->setText(
      QString("レイヤー: %1（%2）").arg(QString::fromStdString(layer.name()), layerKindJa(layer.kind())));
  if (m_selectionStatusLabel != nullptr) {
    m_selectionStatusLabel->setText(QString("選択: %1").arg(doc.selection().hasSelection() ? "ON" : "OFF"));
  }
}

void MainWindow::updateTopToolInfo() {
  if (m_currentToolLabel == nullptr || m_currentSubToolLabel == nullptr) {
    return;
  }

  m_currentToolLabel->setText(QString("ツール: %1").arg(toolNameJa(m_controller->currentTool())));
  m_currentSubToolLabel->setText(QString("サブツール: %1").arg(QString::fromStdString(m_controller->currentSubToolDisplayName())));
  updateToolActionState();
}

void MainWindow::updateNavigatorPreview() {
  if (m_navigatorImageLabel == nullptr || m_controller == nullptr) {
    return;
  }
  const QImage image = platform::qt::QtImageConverter::toQImage(m_controller->compositedBuffer());
  if (image.isNull()) {
    m_navigatorImageLabel->clear();
    return;
  }
  const QSize target = m_navigatorImageLabel->size().expandedTo(QSize(1, 1));
  const QPixmap pixmap = QPixmap::fromImage(image).scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  m_navigatorImageLabel->setPixmap(pixmap);
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
  dialog.setWindowTitle("新規キャンバス");
  auto* form = new QFormLayout(&dialog);
  auto* widthSpin = new QSpinBox(&dialog);
  auto* heightSpin = new QSpinBox(&dialog);
  widthSpin->setRange(1, 8192);
  heightSpin->setRange(1, 8192);
  widthSpin->setValue(m_lastCanvasWidth);
  heightSpin->setValue(m_lastCanvasHeight);
  form->addRow("幅", widthSpin);
  form->addRow("高さ", heightSpin);

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
  if (m_lastForegroundColor.r != state.color.r || m_lastForegroundColor.g != state.color.g ||
      m_lastForegroundColor.b != state.color.b || m_lastForegroundColor.a != state.color.a) {
    m_lastForegroundColor = state.color;
  }

  if (m_sizeStatusLabel != nullptr) {
    m_sizeStatusLabel->setText(QString("サイズ: %1").arg(state.size));
  }

  if (m_colorStatusLabel != nullptr) {
    const QColor color(state.color.r, state.color.g, state.color.b, state.color.a);
    m_colorStatusLabel->setText(
        QString("色: %1 A%2").arg(color.name(QColor::HexRgb).toUpper()).arg(color.alpha()));
  }

  if (m_toolStatusLabel != nullptr) {
    m_toolStatusLabel->setText(QString("ツール: %1").arg(toolNameJa(m_controller->currentTool())));
  }
  if (m_subToolStatusLabel != nullptr) {
    m_subToolStatusLabel->setText(QString("サブツール: %1").arg(QString::fromStdString(m_controller->currentSubToolDisplayName())));
  }

  if (m_guideStatusLabel != nullptr) {
    m_guideStatusLabel->setText(
        QString("操作: %1 / 中ボタンドラッグで一時パン").arg(QString::fromStdString(m_controller->currentToolGuide())));
  }

  updateColorPanel();
  syncForegroundHsvControlsFromColor(state.color);
  updateTopToolInfo();
}

void MainWindow::onOpenTriggered() {
  const QString path = QFileDialog::getOpenFileName(
      this,
      "画像を開く",
      m_currentFilePath.isEmpty() ? QString() : QFileInfo(m_currentFilePath).absolutePath(),
      "画像ファイル (*.png *.jpg *.jpeg *.bmp)");
  if (path.isEmpty()) {
    return;
  }
  openImageFile(path);
}

void MainWindow::onNewFromClipboardTriggered() {
  const QImage image = QGuiApplication::clipboard()->image();
  if (image.isNull()) {
    statusBar()->showMessage("クリップボードに画像がありません", 1800);
    return;
  }
  const core::PixelBuffer buffer = platform::qt::QtImageConverter::fromQImage(image);
  if (buffer.width() <= 0 || buffer.height() <= 0) {
    statusBar()->showMessage("クリップボード画像が不正です", 1800);
    return;
  }
  m_controller->importFlattenedBuffer(buffer, "クリップボード");
  m_currentFilePath.clear();
  statusBar()->showMessage("クリップボード画像から新規キャンバスを作成しました", 2200);
}

void MainWindow::onImportAsLayerTriggered() {
  const QString path = QFileDialog::getOpenFileName(
      this,
      "画像をレイヤーとして読み込み",
      m_currentFilePath.isEmpty() ? QString() : QFileInfo(m_currentFilePath).absolutePath(),
      "画像ファイル (*.png *.jpg *.jpeg *.bmp)");
  if (path.isEmpty()) {
    return;
  }
  QImage image(path);
  if (image.isNull()) {
    statusBar()->showMessage(QString("読み込みに失敗しました: %1").arg(path), 2500);
    return;
  }
  const core::PixelBuffer buffer = platform::qt::QtImageConverter::fromQImage(image);
  const QFileInfo info(path);
  if (m_controller->pasteBufferAsNewRasterLayer(buffer, info.completeBaseName().toStdString())) {
    statusBar()->showMessage(QString("レイヤーとして読み込みました: %1").arg(info.fileName()), 2500);
    updateUndoRedoState();
  }
}

void MainWindow::onSaveTriggered() {
  if (m_currentFilePath.isEmpty()) {
    onSaveAsTriggered();
    return;
  }
  if (saveImageFile(m_currentFilePath)) {
    statusBar()->showMessage(QString("保存しました: %1").arg(m_currentFilePath), 2500);
  }
}

void MainWindow::onSaveAsTriggered() {
  const QString path = QFileDialog::getSaveFileName(
      this,
      "画像を保存",
      m_currentFilePath,
      "PNG画像 (*.png);;JPEG画像 (*.jpg *.jpeg);;BMP画像 (*.bmp)");
  if (path.isEmpty()) {
    return;
  }
  if (saveImageFile(path)) {
    m_currentFilePath = path;
    pushRecentFile(path);
    statusBar()->showMessage(QString("保存しました: %1").arg(path), 2500);
  }
}

void MainWindow::onExportPngTriggered() {
  const QString path = QFileDialog::getSaveFileName(this, "PNGを書き出し", QString(), "PNG画像 (*.png)");
  if (path.isEmpty()) {
    return;
  }
  if (saveImageFile(path)) {
    statusBar()->showMessage(QString("書き出しました: %1").arg(path), 2500);
  }
}

void MainWindow::onExportFlattenedTriggered() {
  const QString path = QFileDialog::getSaveFileName(
      this,
      "統合画像を書き出し",
      QString(),
      "PNG画像 (*.png);;JPEG画像 (*.jpg *.jpeg);;BMP画像 (*.bmp)");
  if (path.isEmpty()) {
    return;
  }
  if (saveImageFile(path)) {
    statusBar()->showMessage(QString("統合画像を書き出しました: %1").arg(path), 2500);
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
  statusBar()->showMessage("クリップボードにコピーしました", 1500);
}

void MainWindow::onPasteTriggered() {
  const QImage image = QGuiApplication::clipboard()->image();
  if (image.isNull()) {
    statusBar()->showMessage("クリップボードに画像がありません", 1500);
    return;
  }
  const core::PixelBuffer buffer = platform::qt::QtImageConverter::fromQImage(image);
  if (m_controller->pasteBufferAsNewRasterLayer(buffer, "貼り付けレイヤー")) {
    statusBar()->showMessage("新規ラスターレイヤーとして貼り付けました", 1500);
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

void MainWindow::onAddFolderLayerTriggered() {
  m_controller->addFolderLayer();
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

void MainWindow::onToggleLayerClipTriggered() {
  if (m_controller->toggleActiveLayerClipToBelow()) {
    updateUndoRedoState();
  }
}

void MainWindow::onToggleLayerMaskTriggered() {
  if (m_controller->toggleActiveLayerMask()) {
    updateUndoRedoState();
  }
}

void MainWindow::onRemoveLayerMaskTriggered() {
  if (m_controller->removeActiveLayerMask()) {
    updateUndoRedoState();
  }
}

void MainWindow::onToggleLayerLockTriggered() {
  if (m_controller->toggleActiveLayerLock()) {
    updateUndoRedoState();
  }
}

void MainWindow::onToggleLayerAlphaLockTriggered() {
  if (m_controller->toggleActiveLayerAlphaLock()) {
    updateUndoRedoState();
  }
}

void MainWindow::onToggleLayerPositionLockTriggered() {
  if (m_controller->toggleActiveLayerPositionLock()) {
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

void MainWindow::updateObjectSelectionStatus() {
  if (m_objectSelectionStatusLabel == nullptr || m_controller == nullptr) {
    return;
  }
  const auto overlay = m_controller->canvasOverlay();
  if (!overlay.objectSelectionRect.has_value() || !overlay.objectSelectionId.has_value()) {
    m_objectSelectionStatusLabel->setText(
        QString("selected object: (none) textObjects=%1").arg(static_cast<int>(overlay.textObjects.size())));
    return;
  }
  const core::Rect bounds = *overlay.objectSelectionRect;
  m_objectSelectionStatusLabel->setText(
      QString("selected object: layer/path=%1 bounds=%2,%3,%4,%5 textObjects=%6")
          .arg(QString::fromStdString(*overlay.objectSelectionId))
          .arg(bounds.x)
          .arg(bounds.y)
          .arg(bounds.width)
          .arg(bounds.height)
          .arg(static_cast<int>(overlay.textObjects.size())));
}

void MainWindow::onRotateViewLeftTriggered() {
  m_canvasWidget->rotateViewLeft();
}

void MainWindow::onRotateViewRightTriggered() {
  m_canvasWidget->rotateViewRight();
}

void MainWindow::onResetViewRotationTriggered() {
  m_canvasWidget->resetViewRotation();
}

void MainWindow::onResetWorkspaceTriggered() {
  if (m_defaultDockState.isEmpty()) {
    return;
  }
  restoreState(m_defaultDockState);
  QSettings settings("taketenkeishi", "LayeredPaintApp");
  settings.beginGroup("workspaces");
  settings.setValue("last", "__default__");
  settings.endGroup();
  statusBar()->showMessage("ワークスペースを初期化しました", 1800);
}

void MainWindow::onSaveWorkspaceTriggered() {
  bool ok = false;
  const QString name = QInputDialog::getText(
      this,
      "ワークスペース保存",
      "ワークスペース名",
      QLineEdit::Normal,
      "カスタム",
      &ok);
  if (!ok || name.trimmed().isEmpty()) {
    return;
  }
  saveWorkspaceLayout(name.trimmed());
  rebuildWorkspaceLayoutsMenu();
  statusBar()->showMessage(QString("ワークスペースを保存しました: %1").arg(name.trimmed()), 1800);
}

void MainWindow::onDeleteWorkspaceTriggered() {
  const QStringList names = workspaceLayoutNames();
  if (names.isEmpty()) {
    statusBar()->showMessage("保存済みワークスペースがありません", 1600);
    return;
  }
  bool ok = false;
  const QString name = QInputDialog::getItem(this, "ワークスペース削除", "対象", names, 0, false, &ok);
  if (!ok || name.isEmpty()) {
    return;
  }
  QSettings settings("taketenkeishi", "LayeredPaintApp");
  settings.beginGroup("workspaces/layouts");
  settings.remove(name);
  settings.endGroup();
  settings.beginGroup("workspaces");
  if (settings.value("last").toString() == name) {
    settings.setValue("last", "__default__");
  }
  settings.endGroup();
  rebuildWorkspaceLayoutsMenu();
  statusBar()->showMessage(QString("ワークスペースを削除しました: %1").arg(name), 1800);
}

void MainWindow::onRestoreLastWorkspaceTriggered() {
  QSettings settings("taketenkeishi", "LayeredPaintApp");
  settings.beginGroup("workspaces");
  const QString last = settings.value("last", "__default__").toString();
  settings.endGroup();
  if (last == "__default__") {
    onResetWorkspaceTriggered();
    return;
  }
  if (!restoreWorkspaceLayout(last)) {
    statusBar()->showMessage("前回のワークスペースが見つかりません", 1800);
  }
}

void MainWindow::onLoadWorkspaceByName(const QString& name) {
  if (name.isEmpty()) {
    return;
  }
  if (!restoreWorkspaceLayout(name)) {
    statusBar()->showMessage(QString("ワークスペースの読み込みに失敗しました: %1").arg(name), 1800);
  }
}

void MainWindow::onCommandPaletteTriggered() {
  struct CommandEntry {
    QAction* action {nullptr};
    QString label;
    QString commandId;
    QString category;
    QString shortcut;
    bool enabled {false};
  };

  const auto categoryLabelFromCommandId = [](const QString& commandId) -> QString {
    const QString prefix = commandId.section('.', 0, 0).toLower();
    if (prefix == "file") {
      return "ファイル";
    }
    if (prefix == "edit") {
      return "編集";
    }
    if (prefix == "tool") {
      return "ツール";
    }
    if (prefix == "select") {
      return "選択";
    }
    if (prefix == "layer") {
      return "レイヤー";
    }
    if (prefix == "view") {
      return "表示";
    }
    if (prefix == "window") {
      return "ウィンドウ";
    }
    if (prefix == "help") {
      return "ヘルプ";
    }
    return "その他";
  };

  QList<QAction*> allActions = findChildren<QAction*>();
  std::vector<CommandEntry> commands;
  commands.reserve(static_cast<std::size_t>(allActions.size()));
  QSet<QAction*> seen;
  for (QAction* action : allActions) {
    if (action == nullptr || !action->property("commandId").isValid()) {
      continue;
    }
    if (action == m_commandPaletteAction || seen.contains(action)) {
      continue;
    }
    seen.insert(action);
    CommandEntry entry;
    entry.action = action;
    entry.label = action->text().remove('&').trimmed();
    entry.commandId = action->property("commandId").toString();
    entry.category = categoryLabelFromCommandId(entry.commandId);
    entry.shortcut = action->shortcut().toString(QKeySequence::NativeText);
    entry.enabled = action->isEnabled();
    if (entry.label.isEmpty()) {
      entry.label = entry.commandId;
    }
    commands.push_back(entry);
  }

  std::sort(commands.begin(), commands.end(), [](const CommandEntry& lhs, const CommandEntry& rhs) {
    const int categoryCompare = lhs.category.compare(rhs.category, Qt::CaseInsensitive);
    if (categoryCompare != 0) {
      return categoryCompare < 0;
    }
    return lhs.label.compare(rhs.label, Qt::CaseInsensitive) < 0;
  });
  if (commands.empty()) {
    statusBar()->showMessage("コマンドが見つかりません", 1800);
    return;
  }

  QDialog dialog(this);
  dialog.setWindowTitle("コマンドパレット");
  dialog.resize(640, 480);
  auto* root = new QVBoxLayout(&dialog);
  root->setContentsMargins(10, 10, 10, 10);
  root->setSpacing(8);

  auto* filterEdit = new QLineEdit(&dialog);
  filterEdit->setPlaceholderText("コマンド名 / カテゴリ / ショートカット / ID で検索...");
  root->addWidget(filterEdit);

  auto* hint = new QLabel("Enter で実行、無効コマンドは一覧で確認のみできます。", &dialog);
  hint->setObjectName("commandPaletteHint");
  root->addWidget(hint);

  auto* list = new QListWidget(&dialog);
  list->setSelectionMode(QAbstractItemView::SingleSelection);
  list->setUniformItemSizes(true);
  list->setAlternatingRowColors(true);
  root->addWidget(list, 1);

  auto repopulate = [&]() {
    list->clear();
    const QString query = filterEdit->text().trimmed();
    for (std::size_t index = 0; index < commands.size(); ++index) {
      const CommandEntry& entry = commands[index];
      const bool matches = query.isEmpty()
          || entry.label.contains(query, Qt::CaseInsensitive)
          || entry.category.contains(query, Qt::CaseInsensitive)
          || entry.commandId.contains(query, Qt::CaseInsensitive)
          || entry.shortcut.contains(query, Qt::CaseInsensitive);
      if (!matches) {
        continue;
      }
      QString text = QString("[%1] %2").arg(entry.category, entry.label);
      if (!entry.enabled) {
        text += "  （無効）";
      }
      if (!entry.shortcut.isEmpty()) {
        text += QString("    [%1]").arg(entry.shortcut);
      }
      auto* item = new QListWidgetItem(text);
      item->setToolTip(entry.commandId);
      item->setData(Qt::UserRole, static_cast<int>(index));
      if (!entry.enabled) {
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
      }
      list->addItem(item);
    }
    if (list->count() > 0) {
      list->setCurrentRow(0);
    }
  };

  connect(filterEdit, &QLineEdit::textChanged, &dialog, repopulate);
  connect(filterEdit, &QLineEdit::returnPressed, &dialog, [&]() {
    if (list->currentItem() == nullptr && list->count() > 0) {
      list->setCurrentRow(0);
    }
    if (list->currentItem() != nullptr && (list->currentItem()->flags() & Qt::ItemIsEnabled)) {
      dialog.accept();
    }
  });
  connect(list, &QListWidget::itemDoubleClicked, &dialog, [&](QListWidgetItem* item) {
    if (item != nullptr && (item->flags() & Qt::ItemIsEnabled)) {
      dialog.accept();
    }
  });

  repopulate();
  filterEdit->setFocus();
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  QListWidgetItem* selected = list->currentItem();
  if (selected == nullptr) {
    return;
  }
  const int commandIndex = selected->data(Qt::UserRole).toInt();
  if (commandIndex < 0 || commandIndex >= static_cast<int>(commands.size())) {
    return;
  }
  QAction* action = commands[static_cast<std::size_t>(commandIndex)].action;
  if (action != nullptr && action->isEnabled()) {
    action->trigger();
  } else {
    statusBar()->showMessage("このコマンドは現在の状態では実行できません", 2200);
  }
}

void MainWindow::onShortcutSettingsTriggered() {
  struct ShortcutRow {
    QAction* action {nullptr};
    QKeySequenceEdit* edit {nullptr};
  };

  QList<QAction*> configurable;
  const QList<QAction*> allActions = findChildren<QAction*>();
  for (QAction* action : allActions) {
    if (action != nullptr && action->property("commandId").isValid()) {
      configurable.push_back(action);
    }
  }
  std::sort(configurable.begin(), configurable.end(), [](const QAction* lhs, const QAction* rhs) {
    return lhs->text() < rhs->text();
  });

  QDialog dialog(this);
  dialog.setWindowTitle("ショートカット設定");
  dialog.resize(640, 620);
  auto* root = new QVBoxLayout(&dialog);
  root->setContentsMargins(10, 10, 10, 10);
  root->setSpacing(8);

  auto* help = new QLabel(
      "ショートカットを設定します。空欄は割り当て解除です。重複は保存時に警告されます。",
      &dialog);
  help->setWordWrap(true);
  root->addWidget(help);

  auto* scroll = new QScrollArea(&dialog);
  scroll->setWidgetResizable(true);
  auto* host = new QWidget(scroll);
  auto* form = new QFormLayout(host);
  form->setContentsMargins(6, 6, 6, 6);
  form->setSpacing(8);

  std::vector<ShortcutRow> rows;
  rows.reserve(static_cast<std::size_t>(configurable.size()));
  for (QAction* action : configurable) {
    auto* edit = new QKeySequenceEdit(action->shortcut(), host);
    edit->setClearButtonEnabled(true);
    form->addRow(action->text().remove('&'), edit);
    rows.push_back(ShortcutRow {action, edit});
  }
  host->setLayout(form);
  scroll->setWidget(host);
  root->addWidget(scroll, 1);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  auto* resetButton = buttons->addButton("初期値に戻す", QDialogButtonBox::ResetRole);
  root->addWidget(buttons);

  connect(resetButton, &QPushButton::clicked, &dialog, [&rows]() {
    for (const ShortcutRow& row : rows) {
      if (row.action == nullptr || row.edit == nullptr) {
        continue;
      }
      const QString defaultText = row.action->property("defaultShortcut").toString();
      row.edit->setKeySequence(QKeySequence::fromString(defaultText, QKeySequence::PortableText));
    }
  });
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  QMap<QString, QStringList> duplicates;
  for (const ShortcutRow& row : rows) {
    if (row.action == nullptr || row.edit == nullptr) {
      continue;
    }
    const QString key = row.edit->keySequence().toString(QKeySequence::PortableText);
    if (!key.isEmpty()) {
      duplicates[key].push_back(row.action->text().remove('&'));
    }
  }
  QStringList conflictLines;
  for (auto it = duplicates.cbegin(); it != duplicates.cend(); ++it) {
    if (it.value().size() > 1) {
      conflictLines.push_back(QString("%1 -> %2").arg(it.key(), it.value().join(", ")));
    }
  }
  if (!conflictLines.isEmpty()) {
    const auto result = QMessageBox::question(
        this,
        "ショートカット競合",
        QString("重複するショートカットがあります:\n\n%1\n\nこのまま適用しますか？").arg(conflictLines.join("\n")));
    if (result != QMessageBox::Yes) {
      return;
    }
  }

  for (const ShortcutRow& row : rows) {
    if (row.action == nullptr || row.edit == nullptr) {
      continue;
    }
    row.action->setShortcut(row.edit->keySequence());
    saveShortcutOverride(row.action->property("commandId").toString(), row.edit->keySequence());
  }
}

void MainWindow::onChooseForegroundColor() {
  const QColor current = toQColor(m_controller->toolState().color);
  const QColor picked = QColorDialog::getColor(current, this, "描画色", QColorDialog::ShowAlphaChannel);
  if (!picked.isValid()) {
    return;
  }
  m_controller->setBrushColor(toCoreColor(picked));
}

void MainWindow::onChooseBackgroundColor() {
  const QColor current = toQColor(m_backgroundColor);
  const QColor picked = QColorDialog::getColor(current, this, "背景色", QColorDialog::ShowAlphaChannel);
  if (!picked.isValid()) {
    return;
  }
  m_backgroundColor = toCoreColor(picked);
  m_controller->setPaperColor(m_backgroundColor);
  updateColorPanel();
}

void MainWindow::onSwapColors() {
  const core::Color foreground = m_controller->toolState().color;
  m_controller->setBrushColor(m_backgroundColor);
  m_backgroundColor = foreground;
  m_controller->setPaperColor(m_backgroundColor);
  updateColorPanel();
}

void MainWindow::onResetBlackWhiteColors() {
  m_controller->setBrushColor(core::Color::OpaqueBlack());
  m_backgroundColor = core::Color {255, 255, 255, 255};
  m_controller->setPaperColor(m_backgroundColor);
  updateColorPanel();
}

void MainWindow::onUseTransparentColor() {
  core::Color color = m_controller->toolState().color;
  color.a = 0;
  m_controller->setBrushColor(color);
  updateColorPanel();
}

void MainWindow::updateColorPanel() {
  if (m_foregroundColorButton == nullptr || m_backgroundColorButton == nullptr) {
    return;
  }

  const QColor fg = toQColor(m_controller->toolState().color);
  const QColor bg = toQColor(m_backgroundColor);

  const auto swatchStyle = [](const QColor& color, int size, bool primary) {
    const QString border = primary ? QStringLiteral("#eef3fb") : QStringLiteral("#d8e0ec");
    const QString hover = primary ? QStringLiteral("#ffffff") : QStringLiteral("#eef3fb");
    const int displayAlpha = color.alpha() == 0 ? 255 : color.alpha();

    const QString background = QStringLiteral("rgba(%1,%2,%3,%4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(displayAlpha);

    return QStringLiteral(
               "QPushButton { min-width: %1px; max-width: %1px; min-height: %1px; max-height: %1px; "
               "padding: 0px; margin: 0px; border: %2px solid %3; border-radius: 2px; background: %4; }"
               "QPushButton:hover { border-color: %5; }"
               "QPushButton:pressed { border-color: #9fb5d6; }")
        .arg(size)
        .arg(primary ? 2 : 1)
        .arg(border)
        .arg(background)
        .arg(hover);
  };

  m_foregroundColorButton->setText(QString());
  m_backgroundColorButton->setText(QString());
  m_foregroundColorButton->setFixedSize(22, 22);
  m_backgroundColorButton->setFixedSize(22, 22);
  m_foregroundColorButton->setStyleSheet(swatchStyle(fg, 22, true));
  m_backgroundColorButton->setStyleSheet(swatchStyle(bg, 22, false));
  m_foregroundColorButton->setToolTip(
      fg.alpha() == 0 ? QStringLiteral("前景色: 透明") : QString("前景色: %1").arg(fg.name(QColor::HexArgb).toUpper()));
  m_backgroundColorButton->setToolTip(QString("背景色: %1").arg(bg.name(QColor::HexArgb).toUpper()));

  refreshColorHistoryButtons();
}

void MainWindow::pushForegroundColorHistory(const core::Color& color) {
  const auto sameColor = [&color](const core::Color& existing) {
    return existing.r == color.r && existing.g == color.g && existing.b == color.b && existing.a == color.a;
  };
  m_colorHistory.erase(std::remove_if(m_colorHistory.begin(), m_colorHistory.end(), sameColor), m_colorHistory.end());
  m_colorHistory.insert(m_colorHistory.begin(), color);
  constexpr std::size_t kHistoryMax = 84;
  if (m_colorHistory.size() > kHistoryMax) {
    m_colorHistory.resize(kHistoryMax);
  }
}

void MainWindow::syncForegroundHsvControlsFromColor(const core::Color& color) {
  if (m_hueSlider == nullptr || m_satSlider == nullptr || m_valSlider == nullptr || m_alphaSlider == nullptr ||
      m_hueSpin == nullptr || m_satSpin == nullptr || m_valSpin == nullptr || m_alphaSpin == nullptr) {
    return;
  }

  const QColor qcolor = toQColor(color);
  int hue = 0;
  int saturation = 0;
  int value = 0;
  qcolor.getHsv(&hue, &saturation, &value);
  if (hue < 0) {
    hue = 0;
  }

  m_updatingColorControls = true;
  m_hueSlider->setValue(hue);
  m_hueSpin->setValue(hue);
  m_satSlider->setValue(saturation);
  m_satSpin->setValue(saturation);
  m_valSlider->setValue(value);
  m_valSpin->setValue(value);
  m_alphaSlider->setValue(qcolor.alpha());
  m_alphaSpin->setValue(qcolor.alpha());
  if (m_colorWheelWidget != nullptr) {
    m_colorWheelWidget->setColor(qcolor);
  }
  m_updatingColorControls = false;
}

void MainWindow::applyForegroundFromHsvControls() {
  if (m_hueSpin == nullptr || m_satSpin == nullptr || m_valSpin == nullptr || m_alphaSpin == nullptr) {
    return;
  }
  const QColor color = QColor::fromHsv(m_hueSpin->value(), m_satSpin->value(), m_valSpin->value(), m_alphaSpin->value());
  m_controller->setBrushColor(toCoreColor(color));
}

void MainWindow::refreshColorHistoryButtons() {
  constexpr int kChipSize = 24;
  const QString emptyStyle = QStringLiteral(
      "QPushButton { min-width: 24px; max-width: 24px; min-height: 24px; max-height: 24px; "
      "padding: 0px; margin: 0px; border: 1px solid #2f3746; border-radius: 0px; background: #202833; }"
      "QPushButton:disabled { min-width: 24px; max-width: 24px; min-height: 24px; max-height: 24px; "
      "padding: 0px; margin: 0px; border: 1px solid #2f3746; border-radius: 0px; background: #202833; }"
      "QPushButton:hover { border: 1px solid #9fb5d6; }");

  for (std::size_t i = 0; i < m_colorHistoryButtons.size(); ++i) {
    auto* chip = m_colorHistoryButtons[i];
    if (chip == nullptr) {
      continue;
    }

    chip->setFixedSize(kChipSize, kChipSize);
    chip->setMinimumSize(kChipSize, kChipSize);
    chip->setMaximumSize(kChipSize, kChipSize);
    chip->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    if (i >= m_colorHistory.size()) {
      chip->setEnabled(false);
      chip->setStyleSheet(emptyStyle);
      chip->setToolTip(QStringLiteral("未使用"));
      continue;
    }

    const core::Color& color = m_colorHistory[i];
    const QString rgb = QStringLiteral("rgb(%1,%2,%3)").arg(color.r).arg(color.g).arg(color.b);
    chip->setEnabled(true);
    chip->setStyleSheet(QStringLiteral(
        "QPushButton { min-width: 24px; max-width: 24px; min-height: 24px; max-height: 24px; "
        "padding: 0px; margin: 0px; border: 1px solid #1f2632; border-radius: 0px; background: %1; }"
        "QPushButton:hover { border: 1px solid #ffffff; }"
        "QPushButton:pressed { border: 1px solid #9fb5d6; }")
        .arg(rgb));
    chip->setToolTip(QStringLiteral("#%1%2%3 A%4")
        .arg(color.r, 2, 16, QChar('0'))
        .arg(color.g, 2, 16, QChar('0'))
        .arg(color.b, 2, 16, QChar('0'))
        .arg(color.a));
  }
  relayoutColorHistoryGrid();
}

void MainWindow::relayoutColorHistoryGrid() {
  if (m_colorHistoryLayout == nullptr || m_colorHistoryGridWidget == nullptr || m_colorHistoryButtons.empty()) {
    return;
  }

  constexpr int kChipSize = 24;
  constexpr int kMinColumns = 1;
  constexpr int kMaxColumns = 48;

  int availableWidth = 0;
  for (QWidget* widget = m_colorHistoryGridWidget; widget != nullptr; widget = widget->parentWidget()) {
    auto* scrollArea = qobject_cast<QScrollArea*>(widget);
    if (scrollArea != nullptr && scrollArea->viewport() != nullptr) {
      availableWidth = scrollArea->viewport()->contentsRect().width();
      break;
    }
  }

  if (availableWidth <= 0 && m_colorHistoryGridWidget->parentWidget() != nullptr) {
    availableWidth = m_colorHistoryGridWidget->parentWidget()->contentsRect().width();
  }
  if (availableWidth <= 0) {
    availableWidth = m_colorHistoryGridWidget->width();
  }

  availableWidth = std::max(kChipSize, availableWidth - 1);
  const int columns = std::clamp(availableWidth / kChipSize, kMinColumns, kMaxColumns);
  const int rowCount = static_cast<int>((m_colorHistoryButtons.size() + static_cast<std::size_t>(columns) - 1) /
                                        static_cast<std::size_t>(columns));

  while (m_colorHistoryLayout->count() > 0) {
    QLayoutItem* item = m_colorHistoryLayout->takeAt(0);
    delete item;
  }

  for (std::size_t i = 0; i < m_colorHistoryButtons.size(); ++i) {
    QPushButton* chip = m_colorHistoryButtons[i];
    if (chip == nullptr) {
      continue;
    }

    chip->setFixedSize(kChipSize, kChipSize);
    chip->setMinimumSize(kChipSize, kChipSize);
    chip->setMaximumSize(kChipSize, kChipSize);
    const int row = static_cast<int>(i) / columns;
    const int col = static_cast<int>(i) % columns;
    m_colorHistoryLayout->addWidget(chip, row, col);
  }

  for (int col = 0; col < columns; ++col) {
    m_colorHistoryLayout->setColumnMinimumWidth(col, kChipSize);
    m_colorHistoryLayout->setColumnStretch(col, 0);
  }
  for (int row = 0; row < rowCount; ++row) {
    m_colorHistoryLayout->setRowMinimumHeight(row, kChipSize);
    m_colorHistoryLayout->setRowStretch(row, 0);
  }

  const int gridWidth = columns * kChipSize;
  const int gridHeight = std::max(1, rowCount) * kChipSize;
  m_colorHistoryGridWidget->setFixedSize(gridWidth, gridHeight);
  m_colorHistoryColumnCount = columns;
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
    statusBar()->showMessage(QString("開くのに失敗しました: %1").arg(path), 2500);
    return false;
  }
  const core::PixelBuffer buffer = platform::qt::QtImageConverter::fromQImage(image);
  const QFileInfo info(path);
  m_controller->importFlattenedBuffer(buffer, info.completeBaseName().toStdString());
  m_currentFilePath = path;
  pushRecentFile(path);
  statusBar()->showMessage(QString("開きました: %1").arg(path), 2500);
  return true;
}

bool MainWindow::saveImageFile(const QString& path) {
  if (path.isEmpty()) {
    return false;
  }
  const QImage image = platform::qt::QtImageConverter::toQImage(m_controller->compositedBuffer());
  const bool ok = image.save(path);
  if (!ok) {
    statusBar()->showMessage(QString("保存に失敗しました: %1").arg(path), 2500);
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
    QAction* empty = m_recentFilesMenu->addAction("（最近使ったファイルはありません）");
    empty->setEnabled(false);
    if (m_clearRecentFilesAction != nullptr) {
      m_recentFilesMenu->addSeparator();
      m_clearRecentFilesAction->setEnabled(false);
      m_recentFilesMenu->addAction(m_clearRecentFilesAction);
    }
    return;
  }
  for (const QString& path : m_recentFiles) {
    QAction* action = m_recentFilesMenu->addAction(path);
    connect(action, &QAction::triggered, this, [this, path]() {
      openImageFile(path);
    });
  }
  if (m_clearRecentFilesAction != nullptr) {
    m_recentFilesMenu->addSeparator();
    m_clearRecentFilesAction->setEnabled(true);
    m_recentFilesMenu->addAction(m_clearRecentFilesAction);
  }
}

QStringList MainWindow::workspaceLayoutNames() const {
  QSettings settings("taketenkeishi", "LayeredPaintApp");
  settings.beginGroup("workspaces/layouts");
  QStringList names = settings.childKeys();
  settings.endGroup();
  names.sort(Qt::CaseInsensitive);
  return names;
}

void MainWindow::rebuildWorkspaceLayoutsMenu() {
  if (m_workspaceLayoutsMenu == nullptr) {
    return;
  }
  m_workspaceLayoutsMenu->clear();
  const QStringList names = workspaceLayoutNames();
  if (names.isEmpty()) {
    QAction* empty = m_workspaceLayoutsMenu->addAction("（保存済みレイアウトはありません）");
    empty->setEnabled(false);
    return;
  }
  for (const QString& name : names) {
    QAction* action = m_workspaceLayoutsMenu->addAction(name);
    connect(action, &QAction::triggered, this, [this, name]() {
      onLoadWorkspaceByName(name);
    });
  }
}

void MainWindow::loadWorkspaceLayoutState() {
  QSettings settings("taketenkeishi", "LayeredPaintApp");
  settings.beginGroup("workspaces");
  const QString last = settings.value("last", "__default__").toString();
  settings.endGroup();
  if (last.isEmpty() || last == "__default__") {
    return;
  }
  if (!restoreWorkspaceLayout(last)) {
    settings.beginGroup("workspaces");
    settings.setValue("last", "__default__");
    settings.endGroup();
  }
}

void MainWindow::saveWorkspaceLayout(const QString& name) {
  if (name.isEmpty()) {
    return;
  }
  QSettings settings("taketenkeishi", "LayeredPaintApp");
  settings.beginGroup("workspaces/layouts");
  settings.setValue(name, saveState());
  settings.endGroup();
  settings.beginGroup("workspaces");
  settings.setValue("last", name);
  settings.endGroup();
}

bool MainWindow::restoreWorkspaceLayout(const QString& name) {
  if (name.isEmpty()) {
    return false;
  }
  QSettings settings("taketenkeishi", "LayeredPaintApp");
  settings.beginGroup("workspaces/layouts");
  const QVariant value = settings.value(name);
  settings.endGroup();
  if (!value.isValid()) {
    return false;
  }
  const QByteArray state = value.toByteArray();
  if (state.isEmpty()) {
    return false;
  }
  if (!restoreState(state)) {
    return false;
  }
  settings.beginGroup("workspaces");
  settings.setValue("last", name);
  settings.endGroup();
  statusBar()->showMessage(QString("ワークスペースを読み込みました: %1").arg(name), 1800);
  return true;
}

void MainWindow::loadShortcutOverrides() {
  QSettings settings("taketenkeishi", "LayeredPaintApp");
  settings.beginGroup("shortcuts");
  const QList<QAction*> actions = findChildren<QAction*>();
  for (QAction* action : actions) {
    if (action == nullptr || !action->property("commandId").isValid()) {
      continue;
    }
    const QString commandId = action->property("commandId").toString();
    if (!settings.contains(commandId)) {
      continue;
    }
    const QString saved = settings.value(commandId).toString();
    action->setShortcut(QKeySequence::fromString(saved, QKeySequence::PortableText));
  }
  settings.endGroup();
}

void MainWindow::saveShortcutOverride(const QString& commandId, const QKeySequence& sequence) {
  if (commandId.isEmpty()) {
    return;
  }
  QSettings settings("taketenkeishi", "LayeredPaintApp");
  settings.beginGroup("shortcuts");
  settings.setValue(commandId, sequence.toString(QKeySequence::PortableText));
  settings.endGroup();
}

QAction* MainWindow::createToolAction(QMenu* toolMenu, core::ToolKind kind, const QString& text, const QKeySequence& shortcut) {
  auto* action = new QAction(text, this);
  action->setCheckable(true);
  action->setShortcut(shortcut);
  action->setData(static_cast<int>(kind));
  action->setToolTip(toolNameJa(kind));
  connect(action, &QAction::triggered, this, &MainWindow::onSetToolTriggered);
  toolMenu->addAction(action);
  m_toolActions[kind] = action;
  return action;
}

} // namespace app::mainwindow



