#include "app/mainwindow/MainWindow.h"
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <functional>
#include <vector>

#include <QAction>
#include <QActionGroup>
#include <QClipboard>
#include <QColorDialog>
#include <QDebug>
#include <QComboBox>
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
#include <QScreen>
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
#include <QMouseEvent>
#include <QToolBar>
#include <QTimer>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>
#include <QUrl>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>

#include "app/bridge/AppController.h"
#include "app/canvasview/CanvasWidget.h"
#include "app/panels/AiPanel.h"
#include "app/ui/Theme.h"
#include "app/panels/GenerativeFillDialog.h"
#include "app/panels/LayerPanel.h"
#include "app/panels/SubToolPanel.h"
#include "app/panels/ToolPanel.h"
#include "app/panels/ToolPropertyPanel.h"
#include "app/panels/ColorWheelWidget.h"
#include "app/ui/IconLoader.h"
#include "platform/qt/QtImageConverter.h"

namespace app::mainwindow {

namespace {

// Verification flags to track constructor execution
static bool g_colorSwatchWidgetCreated = false;


class TitleBarDragArea : public QWidget {
public:
  explicit TitleBarDragArea(QMainWindow* win, QWidget* parent = nullptr)
      : QWidget(parent), m_win(win) {
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  }

protected:
  void mousePressEvent(QMouseEvent* e) override {
    if (e->button() == Qt::LeftButton) {
      m_dragging = true;
      m_dragOffset = e->globalPosition().toPoint() - m_win->frameGeometry().topLeft();
      e->accept();
    }
  }
  void mouseMoveEvent(QMouseEvent* e) override {
    if (m_dragging && (e->buttons() & Qt::LeftButton)) {
      if (m_win->isMaximized()) {
        m_win->showNormal();
        m_dragOffset = QPoint(m_win->width() / 2, 10);
      }
      QPoint target = e->globalPosition().toPoint() - m_dragOffset;
      if (m_win->screen() != nullptr) {
        target.setY(std::max(target.y(), m_win->screen()->availableGeometry().top() + 4));
      }
      m_win->move(target);
      e->accept();
    }
  }
  void mouseReleaseEvent(QMouseEvent* e) override {
    if (e->button() == Qt::LeftButton) {
      m_dragging = false;
    }
  }
  void mouseDoubleClickEvent(QMouseEvent* e) override {
    if (e->button() == Qt::LeftButton) {
      m_win->isMaximized() ? m_win->showNormal() : m_win->showMaximized();
    }
  }

private:
  QMainWindow* m_win;
  QPoint m_dragOffset;
  bool m_dragging {false};
};

// ── DockTitleBar ──────────────────────────────────────────────────────────────
// Photoshop / CLIP STUDIO–style title bar for QDockWidget panels.
//
// Renders a horizontal row of tab buttons — one per dock in the same tabified
// group — directly inside the QDockWidget title bar widget.  The native Qt
// QTabBar for dock areas is hidden via QSS; this bar replaces it visually while
// the underlying tabifyDockWidget() structure (and Qt's dock-drag/float/redock
// machinery) is left completely intact.
//
// Why setTitleBarWidget() enables re-docking:
//   When a custom title bar is set, Qt's nativeWindowDeco() returns false →
//   Qt automatically applies FramelessWindowHint to the floating window →
//   OS never intercepts title-bar drag as SC_MOVE → Qt's own event filter on
//   this widget drives dock-drag mode → drop indicators appear → re-dock works.
//
// Drag safety: the grip widget and the right-hand stretch area have
//   WA_TransparentForMouseEvents so mouse-press/move events on those areas fall
//   through to this widget, where the QDockWidget event filter picks them up as
//   a dock-drag gesture.  Tab QPushButtons are NOT transparent (they handle
//   clicks independently).
class DockTitleBar : public QWidget {
public:
  explicit DockTitleBar(const QString& title, QDockWidget* dock)
      : QWidget(dock), m_dock(dock), m_ownTitle(title)
  {
    setFixedHeight(24);
    setMouseTracking(true);
    // QDockWidget のイベントフィルタがドラッグを検知できるよう
    // このウィジェット自身はマウスイベントを素通しさせる
    setAttribute(Qt::WA_TransparentForMouseEvents, false);

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 2, 0);
    m_layout->setSpacing(0);

    // ── Grip ─────────────────────────────────────────────────────────
    // Transparent → mouse events fall through to DockTitleBar itself →
    // QDockWidget event filter picks them up → dock-drag mode (shows drop
    // indicators, allows re-docking). DO NOT consume events here.
    m_grip = new QWidget(this);
    m_grip->setFixedSize(18, 22);
    m_grip->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_layout->addWidget(m_grip);

    // Tab buttons will be inserted here by rebuildTabs().

    // ── Stretch ───────────────────────────────────────────────────────
    // Wide transparent drag zone — the larger this is, the easier it is
    // to grab and drag the dock (both when docked and floating).
    m_stretch = new QWidget(this);
    m_stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_stretch->setMinimumWidth(40);  // 最低40px確保してドラッグしやすく
    m_stretch->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    // ── Float button ──────────────────────────────────────────────────
    m_floatBtn = new QPushButton("⧉", this);
    m_floatBtn->setFixedSize(20, 22);
    m_floatBtn->setFlat(true);
    m_floatBtn->setFocusPolicy(Qt::NoFocus);
    m_floatBtn->setToolTip("フロート / ドック切替");
    m_floatBtn->setStyleSheet(
        "QPushButton{background:transparent;border:none;color:#4a5570;font-size:12px;}"
        "QPushButton:hover{color:#c5cde0;background:#2f3650;border-radius:3px;}");
    connect(m_floatBtn, &QPushButton::clicked, this, [this] {
      m_dock->setFloating(!m_dock->isFloating());
    });

    // ── Close button ──────────────────────────────────────────────────
    m_closeBtn = new QPushButton("×", this);
    m_closeBtn->setFixedSize(20, 22);
    m_closeBtn->setFlat(true);
    m_closeBtn->setFocusPolicy(Qt::NoFocus);
    m_closeBtn->setToolTip("閉じる");
    m_closeBtn->setStyleSheet(
        "QPushButton{background:transparent;border:none;color:#4a5570;font-size:13px;}"
        "QPushButton:hover{color:#e05070;background:#3a2035;border-radius:3px;}");
    connect(m_closeBtn, &QPushButton::clicked, this, [this] {
      m_dock->close();
      // After hiding this dock Qt will show another dock in the group;
      // schedule a rebuild on siblings so they drop the stale tab.
      QTimer::singleShot(0, this, [this] {
        if (auto* mw = qobject_cast<QMainWindow*>(m_dock->parentWidget())) {
          for (auto* sib : mw->tabifiedDockWidgets(m_dock)) {
            if (auto* tb = dynamic_cast<DockTitleBar*>(sib->titleBarWidget()))
              tb->rebuildTabs();
          }
        }
      });
    });

    // Deferred first build: tabification has finished by the time the
    // event-loop processes this timer.
    QTimer::singleShot(0, this, &DockTitleBar::rebuildTabs);
  }

  // Rebuild the tab button row.
  // Called from MainWindow::updateDockTitleBars() whenever dock layout changes.
  void rebuildTabs() {
    // ── Remove old tab buttons ────────────────────────────────────────
    for (auto* btn : m_tabBtns) {
      m_layout->removeWidget(btn);
      delete btn;
    }
    m_tabBtns.clear();
    m_layout->removeWidget(m_stretch);
    m_layout->removeWidget(m_floatBtn);
    m_layout->removeWidget(m_closeBtn);

    // ── Collect group ─────────────────────────────────────────────────
    // m_dock is always first (active); siblings follow in Qt's tab order.
    QList<QDockWidget*> group;
    if (auto* mw = qobject_cast<QMainWindow*>(m_dock->parentWidget()))
      group = mw->tabifiedDockWidgets(m_dock);
    group.prepend(m_dock);

    // ── Create tab buttons ────────────────────────────────────────────
    for (auto* sib : group) {
      auto* btn = new QPushButton(sib->windowTitle(), this);
      btn->setFlat(true);
      btn->setFocusPolicy(Qt::NoFocus);
      btn->setFixedHeight(22);
      btn->setMinimumWidth(0);   // allow shrinking below text width
      btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
      applyTabStyle(btn, sib == m_dock);

      connect(btn, &QPushButton::clicked, this, [sib] {
        sib->show();
        sib->raise();
      });

      m_layout->addWidget(btn);
      m_tabBtns.append(btn);
    }

    // ── Re-add stretch + action buttons ───────────────────────────────
    m_layout->addWidget(m_stretch, 1);
    m_layout->addWidget(m_floatBtn);
    m_layout->addWidget(m_closeBtn);
  }

  // Keep DockTitleBar from enforcing a minimum width based on tab label text.
  // Only grip (14) + float (20) + close (20) = 54 px is the hard minimum;
  // the tab buttons clip/shrink when the dock is narrower than their content.
  QSize minimumSizeHint() const override { return QSize(54, 22); }
  QSize sizeHint()        const override { return QSize(54, 22); }

protected:
  void paintEvent(QPaintEvent*) override {
    QPainter p(this);
    // Background
    p.fillRect(rect(), QColor(0x1c, 0x20, 0x30));
    // Bottom border
    p.setPen(QColor(0x2a, 0x2e, 0x3e));
    p.drawLine(0, height() - 1, width() - 1, height() - 1);
    // Grip dots — 2 cols × 3 rows
    p.setPen(QColor(0x4a, 0x55, 0x70));
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 2; ++c)
        p.drawPoint(5 + c * 4, 5 + r * 4);
  }

private:
  static void applyTabStyle(QPushButton* btn, bool active) {
    if (active) {
      btn->setStyleSheet(
          "QPushButton{"
          "  background:#212535; color:#c5cde0; border:none;"
          "  border-right:1px solid #252838;"
          "  border-bottom:2px solid #4e8ef7;"
          "  padding:0 10px;"
          "  font-family:'Segoe UI',sans-serif; font-size:10px; font-weight:600;"
          "  letter-spacing:0.5px;}");
    } else {
      btn->setStyleSheet(
          "QPushButton{"
          "  background:#1c2030; color:#5a6d8a; border:none;"
          "  border-right:1px solid #252838;"
          "  padding:0 10px;"
          "  font-family:'Segoe UI',sans-serif; font-size:10px; font-weight:600;"
          "  letter-spacing:0.5px;}"
          "QPushButton:hover{background:#242840; color:#a0b0cc;}");
    }
  }

  QDockWidget*        m_dock     {nullptr};
  QString             m_ownTitle;
  QHBoxLayout*        m_layout   {nullptr};
  QWidget*            m_grip     {nullptr};
  QWidget*            m_stretch  {nullptr};
  QPushButton*        m_floatBtn {nullptr};
  QPushButton*        m_closeBtn {nullptr};
  QList<QPushButton*> m_tabBtns;
};

class ColorSwatchWidget : public QWidget {
public:
  explicit ColorSwatchWidget(QWidget* parent = nullptr)
      : QWidget(parent), m_fgColor(255, 0, 0, 255), m_bgColor(255, 255, 255, 255) {
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    g_colorSwatchWidgetCreated = true;
  }

  void setForegroundColor(const QColor& color) { m_fgColor = color; update(); }
  void setBackgroundColor(const QColor& color) { m_bgColor = color; update(); }

  QSize sizeHint() const override { return QSize(48, 48); }
  QSize minimumSizeHint() const override { return QSize(48, 48); }

  // Simple callback mechanism for clicks (no Qt signals needed)
  std::function<void()> onForegroundClicked;
  std::function<void()> onBackgroundClicked;

protected:
  void mousePressEvent(QMouseEvent* e) override {
    const int fgSize = 32;
    const int fgX = 0;
    const int fgY = 0;
    QRect fgRect(fgX, fgY, fgSize, fgSize);
    if (fgRect.contains(e->pos())) {
      if (onForegroundClicked) onForegroundClicked();
    } else {
      if (onBackgroundClicked) onBackgroundClicked();
    }
  }

  void paintEvent(QPaintEvent*) override {
    const qreal dpr = devicePixelRatioF();
    const int W = width();
    const int H = height();
    QImage img(static_cast<int>(W * dpr), static_cast<int>(H * dpr), QImage::Format_ARGB32_Premultiplied);
    img.setDevicePixelRatio(dpr);
    img.fill(Qt::transparent);

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.scale(dpr, dpr);

    // Helper: draw checker pattern into a rounded rect region
    auto drawChecker = [&](const QRectF& r, int radius) {
      const int cell = 4;
      QPainterPath clip;
      clip.addRoundedRect(r, radius, radius);
      p.save();
      p.setClipPath(clip);
      for (int cy = static_cast<int>(r.top()); cy < static_cast<int>(r.bottom()); cy += cell) {
        for (int cx = static_cast<int>(r.left()); cx < static_cast<int>(r.right()); cx += cell) {
          bool light = (((cx - static_cast<int>(r.left())) / cell) + ((cy - static_cast<int>(r.top())) / cell)) % 2 == 0;
          p.fillRect(QRectF(cx, cy, cell, cell), light ? QColor(220, 220, 220) : QColor(160, 160, 160));
        }
      }
      p.restore();
    };

    // Helper: draw a color swatch with outline
    auto drawSwatch = [&](const QRectF& r, const QColor& color, int radius,
                          const QColor& outlineColor, qreal outlineWidth,
                          bool hasShadow) {
      // Shadow (drop shadow effect: offset rect slightly darker)
      if (hasShadow) {
        QRectF shadowRect = r.translated(1.5, 1.5);
        p.save();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 60));
        p.drawRoundedRect(shadowRect, radius, radius);
        p.restore();
      }
      // White outline (outer glow)
      if (outlineWidth > 0 && outlineColor == Qt::white) {
        p.save();
        p.setPen(QPen(QColor(255, 255, 255, 230), outlineWidth + 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r.adjusted(-1, -1, 1, 1), radius + 1, radius + 1);
        p.restore();
      }
      // Checker pattern for transparent/semi-transparent
      if (color.alpha() < 128) {
        drawChecker(r, radius);
      }
      // Color fill
      p.save();
      p.setPen(Qt::NoPen);
      p.setBrush(color);
      p.drawRoundedRect(r, radius, radius);
      p.restore();
      // Dark border
      p.save();
      p.setPen(QPen(outlineColor, outlineWidth));
      p.setBrush(Qt::NoBrush);
      p.drawRoundedRect(r.adjusted(outlineWidth * 0.5, outlineWidth * 0.5,
                                   -outlineWidth * 0.5, -outlineWidth * 0.5),
                        radius, radius);
      p.restore();
    };

    // BG swatch: 24x24, offset 12px from FG top-left (bottom-right)
    const QRectF bgRect(12.0, 12.0, 24.0, 24.0);
    drawSwatch(bgRect, m_bgColor, 2, QColor(50, 50, 50), 1.0, false);

    // FG swatch: 32x32 at top-left
    const QRectF fgRect(0.0, 0.0, 32.0, 32.0);
    drawSwatch(fgRect, m_fgColor, 3, QColor(40, 40, 40), 1.0, true);
    // White inner outline on FG
    p.save();
    p.setPen(QPen(QColor(255, 255, 255, 180), 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(fgRect.adjusted(1.5, 1.5, -1.5, -1.5), 2, 2);
    p.restore();

    p.end();

    QPainter painter(this);
    painter.drawImage(0, 0, img);
  }

private:
  QColor m_fgColor;
  QColor m_bgColor;
};

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
    case core::ToolKind::AiSelect:
      return "AI 選択";
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
      m_aiPanel(new app::panels::AiPanel(this)),
      m_layerPanel(new app::panels::LayerPanel(this)),
      m_toolPanel(new app::panels::ToolPanel(this)),
      m_quickSliderPanel(new app::panels::ToolPanel(this)),
      m_subToolPanel(new app::panels::SubToolPanel(this)),
      m_toolPropertyPanel(new app::panels::ToolPropertyPanel(this)) {
  // FramelessWindowHint breaks Qt dock drop-zone detection on Windows:
  // QApplication::topLevelAt() can't find the main window as a drop target.
  // Use native window frame; custom chrome is applied via QSS instead.
  setAttribute(Qt::WA_StyledBackground, true);
  {
    const QRect ag = QGuiApplication::primaryScreen()
                         ? QGuiApplication::primaryScreen()->availableGeometry()
                         : QRect(0, 0, 1920, 1080);
    const int w = qMin(1400, ag.width()  - 40);
    const int h = qMin(860,  ag.height() - 40);
    resize(w, h);
    move(ag.left() + (ag.width()  - w) / 2,
         ag.top()  + (ag.height() - h) / 2);
  }

  m_canvasWidget->setController(m_controller);
  m_aiPanel->setController(m_controller);
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
  connect(m_canvasWidget, &app::canvasview::CanvasWidget::viewTransformChanged, this, &MainWindow::updateNavigatorPreview);
  connect(m_canvasWidget, &app::canvasview::CanvasWidget::viewTransformChanged, this, [this]() {
    if (m_zoomStatusLabel != nullptr) {
      m_zoomStatusLabel->setText(QString("ズーム: %1%").arg(m_canvasWidget->zoomPercent()));
    }
  });
  connect(m_canvasWidget, &app::canvasview::CanvasWidget::canvasPositionChanged,
          this, [this](int x, int y) {
    if (m_cursorPosStatusLabel == nullptr) return;
    if (x < 0 || y < 0) {
      m_cursorPosStatusLabel->setText("X: -  Y: -");
    } else {
      m_cursorPosStatusLabel->setText(QString("X: %1  Y: %2").arg(x).arg(y));
    }
  });

  onToolStateChanged();
  updateUndoRedoState();
  updateActiveLayerStatus();
  updateTopToolInfo();
  updateColorPanel();
  updateNavigatorPreview();
}

void MainWindow::setupShellLayout() {
  setCentralWidget(m_canvasWidget);
  setDockNestingEnabled(true);
  setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks |
                 QMainWindow::AnimatedDocks | QMainWindow::GroupedDragging);
  // 全エリアのタブ位置を統一
  for (auto area : {Qt::LeftDockWidgetArea, Qt::RightDockWidgetArea,
                    Qt::TopDockWidgetArea,  Qt::BottomDockWidgetArea}) {
    setTabPosition(area, QTabWidget::North);
  }

  auto* colorPanel = new QWidget(this);
  m_colorPanelWidget = colorPanel;
  auto* colorLayout = new QVBoxLayout(colorPanel);
  colorLayout->setContentsMargins(2, 2, 2, 2);
  colorLayout->setSpacing(1);
  // colorTitle removed: must not exist as child with text matching dock windowTitle "カラー"
  m_foregroundColorButton = new QPushButton(colorPanel);
  m_backgroundColorButton = new QPushButton(colorPanel);
  m_foregroundColorButton->setVisible(false);
  m_backgroundColorButton->setVisible(false);
  auto* swapColorButton = new QPushButton(colorPanel);
  auto* resetColorButton = new QPushButton(colorPanel);
  auto* transparentColorButton = new QPushButton(colorPanel);
  m_colorSwatchWidget = new ColorSwatchWidget(colorPanel);
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
  // Swatch widget: 48x48 for high-quality CSP-like display
  m_colorSwatchWidget->setFixedSize(48, 48);
  swapColorButton->setFixedSize(18, 18);
  resetColorButton->setFixedSize(18, 18);
  swapColorButton->setIcon(app::ui::icon("swap"));
  swapColorButton->setIconSize(QSize(12, 12));
  resetColorButton->setIcon(app::ui::icon("reset_bw"));
  resetColorButton->setIconSize(QSize(12, 12));
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
  transparentColorButton->setToolTip("透明色で描画（アルファ消去）");

  // Checker pattern icon for transparent button (sharper, 20x20)
  {
    QPixmap pixmap(20, 20);
    pixmap.fill(QColor(244, 244, 244));
    QPainter cp(&pixmap);
    constexpr int cell = 4;
    for (int y = 0; y < pixmap.height(); y += cell) {
      for (int x = 0; x < pixmap.width(); x += cell) {
        if (((x / cell) + (y / cell)) % 2 == 1)
          cp.fillRect(QRect(x, y, cell, cell), QColor(130, 140, 155));
      }
    }
    cp.end();
    transparentColorButton->setIcon(QIcon(pixmap));
    transparentColorButton->setIconSize(QSize(14, 14));
    transparentColorButton->setText(QString());
  }
  transparentColorButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  transparentColorButton->setFixedSize(20, 20);
  transparentColorButton->setStyleSheet(QStringLiteral(
      "QPushButton { min-width:20px; max-width:20px; min-height:20px; max-height:20px;"
      "  padding: 0px; margin: 0px; border: 1px solid #6a7484;"
      "  border-radius: 3px; background: transparent; }"
      "QPushButton:hover { border: 1px solid #eef3fb; }"));

  // Layout: [swatch 48x48 top-aligned] [transparent 20x20 / swap 18x18 / reset 18x18 vertical, top-aligned]
  auto* rightVBox = new QVBoxLayout();
  rightVBox->setContentsMargins(0, 0, 0, 0);
  rightVBox->setSpacing(2);
  rightVBox->addWidget(transparentColorButton, 0, Qt::AlignLeft | Qt::AlignTop);
  rightVBox->addWidget(swapColorButton, 0, Qt::AlignLeft | Qt::AlignTop);
  rightVBox->addWidget(resetColorButton, 0, Qt::AlignLeft | Qt::AlignTop);
  rightVBox->addStretch(1);

  auto* colorButtons = new QHBoxLayout();
  colorButtons->setContentsMargins(0, 0, 0, 0);
  colorButtons->setSpacing(4);
  colorButtons->addWidget(m_colorSwatchWidget, 0, Qt::AlignTop | Qt::AlignLeft);
  colorButtons->addLayout(rightVBox, 0);
  colorButtons->addStretch(1);
  auto addHsvRow = [this, colorPanel](const QString& label, QSlider* slider, QSpinBox* spin) {
    auto* row = new QHBoxLayout();
    row->setContentsMargins(1, 0, 1, 0);
    row->setSpacing(2);

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
    slider->setMinimumHeight(20);
    slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    slider->setStyleSheet(QStringLiteral(
        // Reserve handle overhang via groove margin (no negative widget margins)
        "QSlider { padding: 0; }"
        "QSlider::groove:horizontal {"
        "  height: 7px;"
        "  border-radius: 3px;"
        "  background: #202630;"
        "  margin: 0 6px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  height: 7px;"
        "  border-radius: 3px;"
        "  background: %1;"
        "  margin: 0 6px;"
        "}"
        "QSlider::add-page:horizontal {"
        "  height: 7px;"
        "  border-radius: 3px;"
        "  background: #141a22;"
        "  margin: 0 6px;"
        "}"
        "QSlider::handle:horizontal {"
        "  width: 12px;"
        "  height: 12px;"
        "  margin: -3px -6px;"
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
  // historyTitle removed: must not exist as child matching dock windowTitle "カラーヒストリー"
  m_colorHistoryGridWidget = new QWidget(nullptr);
  m_colorHistoryGridWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  m_colorHistoryLayout = new QGridLayout(m_colorHistoryGridWidget);
  m_colorHistoryLayout->setContentsMargins(0, 0, 0, 0);
  m_colorHistoryLayout->setSpacing(1);
  m_colorHistoryLayout->setHorizontalSpacing(1);
  m_colorHistoryLayout->setVerticalSpacing(1);
  m_colorHistoryLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  m_colorHistoryLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
  m_colorHistoryButtons.clear();
  m_colorHistoryButtons.reserve(84);
  for (int i = 0; i < 84; ++i) {
    auto* chip = new QPushButton(nullptr);
    chip->setFixedSize(18, 18);
    chip->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    chip->setProperty("colorHistoryChip", true);
    chip->setStyleSheet("QPushButton { min-width: 18px; max-width: 18px; min-height: 18px; max-height: 18px; padding: 0px; margin: 0px; border: 1px solid #343d4d; border-radius: 0px; background: #202833; }"
                        "QPushButton:hover { border: 1px solid #9fb5d6; }");
    chip->setToolTip("最近使った色");
    chip->setEnabled(false);
    m_colorHistoryLayout->addWidget(chip, i / 12, i % 12);
    m_colorHistoryButtons.push_back(chip);
  }
  for (int col = 0; col < 12; ++col) {
    m_colorHistoryLayout->setColumnMinimumWidth(col, 18);
    m_colorHistoryLayout->setColumnStretch(col, 0);
  }
  for (int row = 0; row < 7; ++row) {
    m_colorHistoryLayout->setRowMinimumHeight(row, 18);
    m_colorHistoryLayout->setRowStretch(row, 0);
  }
  m_colorHistoryGridWidget->setMinimumSize(1, 1);
  auto* colorMainWidget = new QWidget(nullptr);
  auto* colorMainLayout = new QVBoxLayout(colorMainWidget);
  colorMainLayout->setContentsMargins(1, 1, 1, 1);
  colorMainLayout->setSpacing(2); // gap between top row and wheel: 2px (≤4px rule)
  colorMainLayout->addLayout(colorButtons);
  colorMainLayout->addWidget(m_colorWheelWidget, 1);

  auto* hsvWidget = new QWidget(nullptr);
  hsvWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  hsvWidget->setMinimumWidth(176);
  auto* hsvLayout = new QVBoxLayout(hsvWidget);
  hsvLayout->setContentsMargins(0, 0, 0, 0);
  hsvLayout->setSpacing(0);
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
  // historyTitle removed from layout - tab label is sufficient
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
  // ColorSwatchWidget callbacks
  {
    auto* swatchWidget = static_cast<ColorSwatchWidget*>(m_colorSwatchWidget);
    swatchWidget->onForegroundClicked = [this]() { onChooseForegroundColor(); };
    swatchWidget->onBackgroundClicked = [this]() { onChooseBackgroundColor(); };
  }
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
  infoLayout->setContentsMargins(2, 2, 2, 2);
  infoLayout->setSpacing(2);
  // No internal title: dock tab "情報" is sufficient
  m_navigatorImageLabel = new QLabel(infoPanel);
  m_navigatorImageLabel->setMinimumSize(160, 100);
  m_navigatorImageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_navigatorImageLabel->setAlignment(Qt::AlignCenter);
  m_navigatorImageLabel->setStyleSheet("background:#1e1e1e; border:1px solid #1a1a1a;");
  auto* navigatorButtons = new QHBoxLayout();
  navigatorButtons->setContentsMargins(0, 0, 0, 0);
  navigatorButtons->setSpacing(2);
  auto* zoom100Button = new QPushButton("100%", infoPanel);
  auto* fitButton = new QPushButton("全体表示", infoPanel);
  zoom100Button->setFixedHeight(22);
  fitButton->setFixedHeight(22);
  navigatorButtons->addWidget(zoom100Button);
  navigatorButtons->addWidget(fitButton);
  // Navigator: preview top, controls immediately below — no overlap
  infoLayout->addWidget(m_navigatorImageLabel, 1);
  infoLayout->addLayout(navigatorButtons, 0);
  connect(zoom100Button, &QPushButton::clicked, this, &MainWindow::onResetZoomTriggered);
  connect(fitButton, &QPushButton::clicked, this, &MainWindow::onFitToScreenTriggered);

  auto makeDock = [this](const QString& title, QWidget* widget, const char* name) {
    auto* dock = new QDockWidget(title, this);
    dock->setObjectName(name);
    dock->setWidget(widget);
    dock->setFeatures(QDockWidget::DockWidgetMovable |
                      QDockWidget::DockWidgetFloatable |
                      QDockWidget::DockWidgetClosable);
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    // Custom tab-styled title bar.
    // When setTitleBarWidget() is used, Qt's nativeWindowDeco() returns false
    // for this dock, so Qt automatically applies FramelessWindowHint when floating.
    // The QDockWidget event filter installed on DockTitleBar then handles
    // title-bar mouse-drag → dock-drag mode on Windows (no OS SC_MOVE intercept).
    dock->setTitleBarWidget(new DockTitleBar(title, dock));
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
  m_colorSliderDock = makeDock("カラースライダー", makeScrollable(hsvWidget), "ColorSliderDock");
  m_colorSliderDock->setMinimumWidth(188);
  m_colorHistoryDock = makeDock("カラーヒストリー", makeScrollable(historyWidget), "ColorHistoryDock");
  m_colorHistoryDock->setMinimumWidth(188);
  m_layerDock = makeDock("レイヤー", m_layerPanel, "LayerDock");
  m_aiDock = makeDock("AI 生成", m_aiPanel, "AiDock");
  m_infoDock = makeDock("情報", infoPanel, "InfoDock");
  // Native title bars are kept so Qt's dock drag/float/rearrange machinery works.
  // They are styled compact and dark via QSS in applyUiChrome().
  // Tabified docks can be floated by right-clicking the tab (Qt standard behavior).

  // ── Left dock area ────────────────────────────────────────────────────────
  // Three columns: Tool | ToolSlider | SubTool+Color group
  addDockWidget(Qt::LeftDockWidgetArea, m_toolDock);
  addDockWidget(Qt::LeftDockWidgetArea, m_toolSliderDock);
  addDockWidget(Qt::LeftDockWidgetArea, m_subToolDock);
  addDockWidget(Qt::LeftDockWidgetArea, m_toolPropertyDock);
  addDockWidget(Qt::LeftDockWidgetArea, m_colorDock);
  addDockWidget(Qt::LeftDockWidgetArea, m_colorSliderDock);
  addDockWidget(Qt::LeftDockWidgetArea, m_colorHistoryDock);
  splitDockWidget(m_toolDock, m_toolSliderDock, Qt::Horizontal);
  splitDockWidget(m_toolSliderDock, m_subToolDock, Qt::Horizontal);
  splitDockWidget(m_subToolDock, m_colorDock, Qt::Vertical);
  tabifyDockWidget(m_subToolDock, m_toolPropertyDock);
  tabifyDockWidget(m_colorDock, m_colorSliderDock);
  tabifyDockWidget(m_colorDock, m_colorHistoryDock);
  m_subToolDock->raise();
  m_colorDock->raise();

  // ── Right dock area ────────────────────────────────────────────────────────
  // All three panels tabified together — same pattern as MinimalDockTest.
  // No vertical split: splitDockWidget after tabifyDockWidget breaks drop-zone detection.
  addDockWidget(Qt::RightDockWidgetArea, m_layerDock);
  addDockWidget(Qt::RightDockWidgetArea, m_aiDock);
  addDockWidget(Qt::RightDockWidgetArea, m_infoDock);
  tabifyDockWidget(m_layerDock, m_aiDock);
  tabifyDockWidget(m_layerDock, m_infoDock);

  m_toolDock->raise();
  m_layerDock->raise();
  m_defaultDockState = saveState();

  // Connect dock state changes to dynamic title bar management
  const QList<QDockWidget*> allDocks = {
      m_toolDock, m_toolSliderDock, m_subToolDock, m_toolPropertyDock,
      m_colorDock, m_colorSliderDock, m_colorHistoryDock,
      m_layerDock, m_aiDock, m_infoDock
  };
  for (auto* dock : allDocks) {
    if (!dock) continue;
    connect(dock, &QDockWidget::topLevelChanged, this, [this](bool) {
      QTimer::singleShot(0, this, &MainWindow::updateDockTitleBars);
    });
    connect(dock, &QDockWidget::dockLocationChanged, this, [this](Qt::DockWidgetArea) {
      QTimer::singleShot(0, this, &MainWindow::updateDockTitleBars);
    });
  }

  // After layout pass: tag native dock tab bars (for QSS hide), rebuild
  // DockTitleBar tab buttons, and connect visibilityChanged so that
  // closing/showing a dock refreshes the sibling tab lists.
  QTimer::singleShot(0, this, [this] {
    for (auto* tb : findChildren<QTabBar*>()) {
      if (qobject_cast<QTabWidget*>(tb->parentWidget())) continue;
      tb->setProperty("dockTabBar", true);
      // Force QSS re-evaluation for the property to take effect immediately.
      tb->style()->unpolish(tb);
      tb->style()->polish(tb);
      tb->update();
    }
    updateDockTitleBars();
  });
}

void MainWindow::createMenus() {
  auto* fileMenu   = menuBar()->addMenu(QString::fromUtf8(u8"ファイル(&F)"));
  auto* editMenu   = menuBar()->addMenu(QString::fromUtf8(u8"編集(&E)"));
  auto* layerMenu  = menuBar()->addMenu(QString::fromUtf8(u8"レイヤー(&L)"));
  auto* selectMenu = menuBar()->addMenu(QString::fromUtf8(u8"選択範囲(&S)"));
  auto* filterMenu = menuBar()->addMenu(QString::fromUtf8(u8"フィルター(&I)"));
  auto* aiMenu     = menuBar()->addMenu(QString::fromUtf8(u8"AI(&A)"));
  auto* toolMenu   = menuBar()->addMenu(QString::fromUtf8(u8"ツール(&T)"));
  auto* viewMenu   = menuBar()->addMenu(QString::fromUtf8(u8"表示(&V)"));
  auto* windowMenu = menuBar()->addMenu(QString::fromUtf8(u8"ウィンドウ(&W)"));
  auto* helpMenu   = menuBar()->addMenu(QString::fromUtf8(u8"ヘルプ(&H)"));

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
  m_createMaskFromSelAction = new QAction("選択範囲からマスク作成(&M)", this);
  m_invertLayerMaskAction   = new QAction("マスクを反転(&I)", this);
  m_applyLayerMaskAction    = new QAction("マスクを適用(&A)", this);
  m_addAdjBrightnessAction  = new QAction("明るさ・コントラスト", this);
  m_addAdjHueSatAction      = new QAction("色相・彩度", this);
  m_addAdjLevelsAction      = new QAction("レベル補正", this);
  m_addAdjInvertAction      = new QAction("階調の反転", this);
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
  m_swapColorsAction = new QAction("描画色と背景色を切り替え(&C)", this);
  m_resetColorsAction = new QAction("描画色/背景色を白黒に戻す(&D)", this);
  m_transparentColorAction = new QAction("描画色と透明色を切り替え(&X)", this);
  m_generativeFillAction      = new QAction(QString::fromUtf8(u8"AI 生成塗りつぶし(&A)..."), this);
  m_connectComfyUiAction      = new QAction(QString::fromUtf8(u8"ComfyUI に接続(&Y)..."), this);
  m_clearRecentFilesAction    = new QAction(QString::fromUtf8(u8"最近使ったファイルをクリア"), this);
  m_brightnessContrastAction  = new QAction(QString::fromUtf8(u8"明るさ・コントラスト(&B)..."), this);
  m_hueSatLightAction         = new QAction(QString::fromUtf8(u8"色相・彩度・明度(&H)..."), this);
  m_resetRotationAction       = new QAction(QString::fromUtf8(u8"キャンバス回転をリセット(&R)"), this);
  m_mirrorViewAction          = new QAction(QString::fromUtf8(u8"左右反転表示(&F)"), this);
  m_expandSelectionAction     = new QAction(QString::fromUtf8(u8"選択範囲を拡張(&E)..."), this);
  m_contractSelectionAction   = new QAction(QString::fromUtf8(u8"選択範囲を縮小(&C)..."), this);
  m_gaussianBlurAction        = new QAction(QString::fromUtf8(u8"ガウスぼかし(&G)..."), this);
  m_motionBlurAction          = new QAction(QString::fromUtf8(u8"モーションぼかし(&M)..."), this);
  m_transformAction           = new QAction(QString::fromUtf8(u8"変形(&T)"), this);
  m_freeTransformAction       = new QAction(QString::fromUtf8(u8"自由変形(&F)"), this);

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
  m_toggleGridAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_QuoteLeft));
  m_toggleOverlayAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_8));
  m_resetWorkspaceAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W));
  m_saveWorkspaceAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_S));
  m_restoreLastWorkspaceAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_W));
  m_shortcutSettingsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_K));
  m_commandPaletteAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
  m_swapColorsAction->setShortcut(QKeySequence(Qt::Key_C));
  m_resetColorsAction->setShortcut(QKeySequence(Qt::Key_D));
  m_transparentColorAction->setShortcut(QKeySequence(Qt::Key_X));
  m_generativeFillAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));
  m_connectComfyUiAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Y));
  m_brightnessContrastAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
  m_hueSatLightAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));
  m_resetRotationAction->setShortcut(QKeySequence(Qt::Key_R));
  m_transformAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
  m_freeTransformAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
  m_mirrorViewAction->setCheckable(true);
  m_toggleGridAction->setCheckable(true);
  m_toggleGridAction->setChecked(m_canvasWidget->isGridVisible());
  m_toggleOverlayAction->setCheckable(true);
  m_toggleOverlayAction->setChecked(m_canvasWidget->isOverlayVisible());

  fileMenu->addAction(m_newCanvasAction);
  fileMenu->addAction(m_openAction);
  fileMenu->addAction(m_newFromClipboardAction);
  fileMenu->addAction(m_importAsLayerAction);
  auto* resizeCanvasAction = new QAction("キャンバスサイズを変更(&R)...", this);
  resizeCanvasAction->setShortcut(QKeySequence("Ctrl+Alt+R"));
  connect(resizeCanvasAction, &QAction::triggered, this, &MainWindow::onResizeCanvas);
  fileMenu->addAction(resizeCanvasAction);
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
  editMenu->addSeparator();
  editMenu->addAction(m_deletePixelsAction);
  editMenu->addAction(m_fillAction);
  editMenu->addAction(m_clearAction);
  editMenu->addSeparator();
  {
    auto* transformSubMenu = editMenu->addMenu(QString::fromUtf8(u8"変形(&T)"));
    transformSubMenu->addAction(m_transformAction);
    transformSubMenu->addAction(m_freeTransformAction);
  }
  editMenu->addSeparator();
  editMenu->addAction(m_brushSizeDownAction);
  editMenu->addAction(m_brushSizeUpAction);
  editMenu->addSeparator();
  editMenu->addAction(m_swapColorsAction);
  editMenu->addAction(m_resetColorsAction);
  editMenu->addAction(m_transparentColorAction);

  // フィルターメニュー
  {
    auto* blurSubMenu = filterMenu->addMenu(QString::fromUtf8(u8"ぼかし(&B)"));
    blurSubMenu->addAction(m_gaussianBlurAction);
    blurSubMenu->addAction(m_motionBlurAction);
    auto* adjustSubMenu = filterMenu->addMenu(QString::fromUtf8(u8"色調補正(&A)"));
    adjustSubMenu->addAction(m_brightnessContrastAction);
    adjustSubMenu->addAction(m_hueSatLightAction);
  }

  // ── AI メニュー ─────────────────────────────────────────────────────────
  {
    auto* connGroup = aiMenu->addMenu(QString::fromUtf8(u8"バックエンド接続(&B)"));
    connGroup->addAction(m_connectComfyUiAction);
    aiMenu->addSeparator();
    aiMenu->addAction(m_generativeFillAction);
    auto* removeAction = aiMenu->addAction(QString::fromUtf8(u8"背景削除..."));
    removeAction->setEnabled(false);  // Phase C で有効化
    auto* inpaintAction = aiMenu->addAction(QString::fromUtf8(u8"生成塗りつぶし..."));
    inpaintAction->setEnabled(false);  // Phase D で有効化
    auto* upscaleAction = aiMenu->addAction(QString::fromUtf8(u8"高解像度化..."));
    upscaleAction->setEnabled(false);
    aiMenu->addSeparator();
    if (m_aiDock != nullptr) {
      aiMenu->addAction(m_aiDock->toggleViewAction());
    }
  }

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
  bindTool(core::ToolKind::Gradient, "グラデーション(&N)", QKeySequence(Qt::Key_N));
  bindTool(core::ToolKind::Line, "直線(&U)", QKeySequence(Qt::Key_U));
  bindTool(core::ToolKind::RectSelection, QString::fromUtf8(u8"選択(&S)"), QKeySequence(Qt::Key_S));
  bindTool(core::ToolKind::MoveLayer, "移動(&M)", QKeySequence(Qt::Key_M));
  bindTool(core::ToolKind::Hand, "手のひら(&H)", QKeySequence(Qt::Key_H));
  bindTool(core::ToolKind::Zoom, "ズーム(&Z)", QKeySequence(Qt::Key_Z));
  bindTool(core::ToolKind::AiSelect, "AI 選択(&W)", QKeySequence(Qt::Key_W));

  selectMenu->addAction(m_selectAllAction);
  selectMenu->addAction(m_deselectAction);
  selectMenu->addSeparator();
  selectMenu->addAction(m_invertSelectionAction);
  selectMenu->addAction(m_expandSelectionAction);
  selectMenu->addAction(m_contractSelectionAction);
  selectMenu->addSeparator();
  selectMenu->addAction(m_clearSelectionAction);

  toolMenu->addSeparator();
  toolMenu->addAction(m_brushSizeDownAction);
  toolMenu->addAction(m_brushSizeUpAction);

  layerMenu->addAction(m_addRasterLayerAction);
  layerMenu->addAction(m_addVectorLayerAction);
  layerMenu->addAction(m_addFolderLayerAction);
  layerMenu->addAction(m_duplicateLayerAction);
  layerMenu->addAction(m_deleteLayerAction);
  layerMenu->addAction(m_mergeDownAction);
  layerMenu->addAction(m_rasterizeLayerAction);
  layerMenu->addSeparator();
  // 色調補正レイヤーサブメニュー
  auto* adjMenu = layerMenu->addMenu(QString::fromUtf8(u8"新規色調補正レイヤー(&J)"));
  adjMenu->addAction(m_addAdjBrightnessAction);
  adjMenu->addAction(m_addAdjHueSatAction);
  adjMenu->addAction(m_addAdjLevelsAction);
  adjMenu->addSeparator();
  adjMenu->addAction(m_addAdjInvertAction);
  layerMenu->addSeparator();
  layerMenu->addAction(m_toggleLayerClipAction);
  layerMenu->addAction(m_toggleLayerMaskAction);
  layerMenu->addAction(m_createMaskFromSelAction);
  layerMenu->addAction(m_invertLayerMaskAction);
  layerMenu->addAction(m_applyLayerMaskAction);
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
  viewMenu->addAction(m_resetRotationAction);
  viewMenu->addAction(m_mirrorViewAction);
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
  if (m_aiDock != nullptr) {
    windowMenu->addAction(m_aiDock->toggleViewAction());
  }
  if (m_infoDock != nullptr) {
    windowMenu->addAction(m_infoDock->toggleViewAction());
  }
  windowMenu->addSeparator();
  // ── ワークスペースプリセット ─────────────────────────────────────────────
  {
    auto* presetMenu = windowMenu->addMenu(QString::fromUtf8(u8"ワークスペース切替(&X)"));
    auto addPreset = [&](const QString& label, auto fn) {
      connect(presetMenu->addAction(label), &QAction::triggered, this, fn);
    };
    addPreset(QString::fromUtf8(u8"Painting（作画）"), [this] {
      // ブラシ系中心: ツール/サブツール/カラー/レイヤー表示, AI非表示
      if (m_toolDock)         m_toolDock->setVisible(true);
      if (m_toolSliderDock)   m_toolSliderDock->setVisible(true);
      if (m_subToolDock)      m_subToolDock->setVisible(true);
      if (m_toolPropertyDock) m_toolPropertyDock->setVisible(true);
      if (m_colorDock)        m_colorDock->setVisible(true);
      if (m_layerDock)        m_layerDock->setVisible(true);
      if (m_aiDock)           m_aiDock->setVisible(false);
      if (m_infoDock)         m_infoDock->setVisible(false);
    });
    addPreset(QString::fromUtf8(u8"Photo Editing（写真編集）"), [this] {
      // レイヤー/カラー/情報中心
      if (m_toolDock)         m_toolDock->setVisible(true);
      if (m_toolSliderDock)   m_toolSliderDock->setVisible(false);
      if (m_subToolDock)      m_subToolDock->setVisible(false);
      if (m_toolPropertyDock) m_toolPropertyDock->setVisible(true);
      if (m_colorDock)        m_colorDock->setVisible(true);
      if (m_layerDock)        m_layerDock->setVisible(true);
      if (m_aiDock)           m_aiDock->setVisible(false);
      if (m_infoDock)         m_infoDock->setVisible(true);
    });
    addPreset(QString::fromUtf8(u8"AI Editing（AI編集）"), [this] {
      // AI Studio中心
      if (m_toolDock)         m_toolDock->setVisible(true);
      if (m_toolSliderDock)   m_toolSliderDock->setVisible(false);
      if (m_subToolDock)      m_subToolDock->setVisible(false);
      if (m_toolPropertyDock) m_toolPropertyDock->setVisible(false);
      if (m_colorDock)        m_colorDock->setVisible(false);
      if (m_layerDock)        m_layerDock->setVisible(true);
      if (m_aiDock)           m_aiDock->setVisible(true);
      if (m_infoDock)         m_infoDock->setVisible(false);
      if (m_aiDock)           m_aiDock->raise();
    });
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
  helpMenu->addAction(m_openDocsAction);

  // ── ショートカットメニュー（メニューバー）————全カテゴリ網羅────────────
  // NOTE: このセクションは markCommand の直後に配置すること。
  // ツールアクションは m_toolActions に格納済み。
  auto* shortcutMenu = menuBar()->addMenu("ショートカット(&K)");
  shortcutMenu->addAction(m_shortcutSettingsAction);
  shortcutMenu->addAction(m_commandPaletteAction);
  shortcutMenu->addAction(m_shortcutSummaryAction);
  shortcutMenu->addSeparator();

  // ── ファイル ──────────────────────────────────────────────────────────
  auto* scFile = shortcutMenu->addMenu("ファイル");
  scFile->addAction(m_newCanvasAction);
  scFile->addAction(m_openAction);
  scFile->addAction(m_newFromClipboardAction);
  scFile->addAction(m_importAsLayerAction);
  scFile->addSeparator();
  scFile->addAction(m_saveAction);
  scFile->addAction(m_saveAsAction);
  scFile->addAction(m_exportPngAction);
  scFile->addAction(m_exportFlattenedAction);
  scFile->addSeparator();
  scFile->addAction(m_exitAction);

  // ── 編集 ─────────────────────────────────────────────────────────────
  auto* scEdit = shortcutMenu->addMenu("編集");
  scEdit->addAction(m_undoAction);
  scEdit->addAction(m_redoAction);
  scEdit->addSeparator();
  scEdit->addAction(m_cutAction);
  scEdit->addAction(m_copyAction);
  scEdit->addAction(m_pasteAction);
  scEdit->addAction(m_deletePixelsAction);
  scEdit->addAction(m_fillAction);
  scEdit->addAction(m_clearAction);
  scEdit->addSeparator();
  scEdit->addAction(m_brushSizeDownAction);
  scEdit->addAction(m_brushSizeUpAction);
  scEdit->addSeparator();
  // AI アクションは AI メニューへ移動済み

  // ── ツール ────────────────────────────────────────────────────────────
  auto* scTool = shortcutMenu->addMenu("ツール");
  for (const auto& [kind, action] : m_toolActions) {
    if (action != nullptr) {
      scTool->addAction(action);
    }
  }
  scTool->addSeparator();
  scTool->addAction(m_swapColorsAction);
  scTool->addAction(m_transparentColorAction);
  scTool->addAction(m_resetColorsAction);

  // ── 選択 ─────────────────────────────────────────────────────────────
  auto* scSelect = shortcutMenu->addMenu("選択");
  scSelect->addAction(m_selectAllAction);
  scSelect->addAction(m_deselectAction);
  scSelect->addAction(m_clearSelectionAction);
  scSelect->addAction(m_invertSelectionAction);

  // ── レイヤー ──────────────────────────────────────────────────────────
  auto* scLayer = shortcutMenu->addMenu("レイヤー");
  scLayer->addAction(m_addRasterLayerAction);
  scLayer->addAction(m_addVectorLayerAction);
  scLayer->addAction(m_addFolderLayerAction);
  scLayer->addAction(m_duplicateLayerAction);
  scLayer->addAction(m_deleteLayerAction);
  scLayer->addAction(m_mergeDownAction);
  scLayer->addAction(m_rasterizeLayerAction);
  scLayer->addSeparator();
  scLayer->addAction(m_toggleLayerVisibilityAction);
  scLayer->addAction(m_toggleLayerClipAction);
  scLayer->addAction(m_toggleLayerMaskAction);
  scLayer->addAction(m_removeLayerMaskAction);
  scLayer->addAction(m_toggleLayerLockAction);
  scLayer->addAction(m_toggleLayerAlphaLockAction);
  scLayer->addAction(m_toggleLayerPositionLockAction);
  scLayer->addSeparator();
  scLayer->addAction(m_moveLayerUpAction);
  scLayer->addAction(m_moveLayerDownAction);

  // ── 表示 ─────────────────────────────────────────────────────────────
  auto* scView = shortcutMenu->addMenu("表示");
  scView->addAction(m_zoomInAction);
  scView->addAction(m_zoomOutAction);
  scView->addAction(m_resetZoomAction);
  scView->addAction(m_fitToScreenAction);
  scView->addSeparator();
  scView->addAction(m_toggleGridAction);
  scView->addAction(m_toggleOverlayAction);

  // ── ウィンドウ ────────────────────────────────────────────────────────
  auto* scWindow = shortcutMenu->addMenu("ウィンドウ");
  scWindow->addAction(m_saveWorkspaceAction);
  scWindow->addAction(m_deleteWorkspaceAction);
  scWindow->addAction(m_restoreLastWorkspaceAction);
  scWindow->addAction(m_resetWorkspaceAction);

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
  connect(m_createMaskFromSelAction, &QAction::triggered, this, [this] {
    m_controller->createLayerMaskFromSelection();
  });
  connect(m_invertLayerMaskAction, &QAction::triggered, this, [this] {
    m_controller->invertLayerMask();
  });
  connect(m_applyLayerMaskAction, &QAction::triggered, this, [this] {
    m_controller->applyLayerMask();
  });
  connect(m_addAdjBrightnessAction, &QAction::triggered, this, [this] {
    m_controller->addAdjustmentLayerByKind(core::AdjustmentKind::BrightnessContrast);
  });
  connect(m_addAdjHueSatAction, &QAction::triggered, this, [this] {
    m_controller->addAdjustmentLayerByKind(core::AdjustmentKind::HueSaturation);
  });
  connect(m_addAdjLevelsAction, &QAction::triggered, this, [this] {
    m_controller->addAdjustmentLayerByKind(core::AdjustmentKind::Levels);
  });
  connect(m_addAdjInvertAction, &QAction::triggered, this, [this] {
    m_controller->addAdjustmentLayerByKind(core::AdjustmentKind::Invert);
  });
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
  connect(m_generativeFillAction,  &QAction::triggered, this, &MainWindow::onGenerativeFillTriggered);
  connect(m_connectComfyUiAction,  &QAction::triggered, this, &MainWindow::onConnectComfyUiTriggered);
  connect(m_brightnessContrastAction, &QAction::triggered, this, &MainWindow::onBrightnessContrastTriggered);
  connect(m_hueSatLightAction,        &QAction::triggered, this, &MainWindow::onHueSatLightTriggered);
  connect(m_resetRotationAction, &QAction::triggered, this, [this]() {
    if (m_canvasWidget != nullptr) {
      m_canvasWidget->resetRotation();
    }
  });
  connect(m_mirrorViewAction, &QAction::toggled, this, [this](bool checked) {
    if (m_canvasWidget != nullptr) {
      m_canvasWidget->setMirrorView(checked);
    }
  });
  // スタブ: 変形/フィルターは将来実装
  connect(m_transformAction,     &QAction::triggered, this, [this]() {
    statusBar()->showMessage(QString::fromUtf8(u8"変形: 未実装"), 3000);
  });
  connect(m_freeTransformAction, &QAction::triggered, this, [this]() {
    statusBar()->showMessage(QString::fromUtf8(u8"自由変形: 未実装"), 3000);
  });
  connect(m_gaussianBlurAction,  &QAction::triggered, this, [this]() {
    statusBar()->showMessage(QString::fromUtf8(u8"ガウスぼかし: 未実装"), 3000);
  });
  connect(m_motionBlurAction,    &QAction::triggered, this, [this]() {
    statusBar()->showMessage(QString::fromUtf8(u8"モーションぼかし: 未実装"), 3000);
  });
  connect(m_expandSelectionAction,   &QAction::triggered, this, [this]() {
    statusBar()->showMessage(QString::fromUtf8(u8"選択範囲を拡張: 未実装"), 3000);
  });
  connect(m_contractSelectionAction, &QAction::triggered, this, [this]() {
    statusBar()->showMessage(QString::fromUtf8(u8"選択範囲を縮小: 未実装"), 3000);
  });
  connect(m_controller, &app::bridge::AppController::comfyUiStateChanged,
          this, &MainWindow::onComfyUiStateChanged);
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
  m_cursorPosStatusLabel = new QLabel("X: -  Y: -", this);
  m_cursorPosStatusLabel->setObjectName("CursorPosStatusLabel");
  m_cursorPosStatusLabel->setMinimumWidth(100);
  m_selectionStatusLabel = new QLabel("選択: OFF", this);
  m_selectionStatusLabel->setObjectName("SelectionStatusLabel");
  m_comfyUiStatusLabel = new QLabel("ComfyUI: 未接続", this);
  m_comfyUiStatusLabel->setObjectName("ComfyUiStatusLabel");
  m_comfyUiStatusLabel->setStyleSheet("color: #6a7484; font-size: 10px;");

  statusBar()->addWidget(m_toolStatusLabel);
  statusBar()->addWidget(m_subToolStatusLabel);
  statusBar()->addWidget(m_guideStatusLabel, 1);
  statusBar()->addPermanentWidget(m_colorStatusLabel);
  statusBar()->addPermanentWidget(m_sizeStatusLabel);
  statusBar()->addPermanentWidget(m_cursorPosStatusLabel);
  statusBar()->addPermanentWidget(m_zoomStatusLabel);
  statusBar()->addPermanentWidget(m_selectionStatusLabel);
  statusBar()->addPermanentWidget(m_activeLayerStatusLabel);
  statusBar()->addPermanentWidget(m_comfyUiStatusLabel);

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
  markCommand(m_generativeFillAction,  "edit.generative_fill");
  markCommand(m_connectComfyUiAction,  "edit.connect_comfyui");
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

  // With native window frame, OS provides minimize/maximize/close buttons.
  // No custom corner widget needed — the native title bar handles window dragging.
}

void MainWindow::adjustRightDockLayout() {
  // No-op: right docks are now fully tabified (no vertical split).
  // Removed resizeDocks/setMaximumHeight calls that interfered with dock drag.
}

void MainWindow::resizeEvent(QResizeEvent* event) {
  QMainWindow::resizeEvent(event);
  relayoutColorHistoryGrid();
}

void MainWindow::updateDockTitleBars() {
  // Rebuild the tab-button row inside every DockTitleBar.
  // Called after topLevelChanged / dockLocationChanged so that floating or
  // re-docked panels update their tab lists immediately.
  const QList<QDockWidget*> docks = {
      m_toolDock, m_toolSliderDock, m_subToolDock, m_toolPropertyDock,
      m_colorDock, m_colorSliderDock, m_colorHistoryDock,
      m_layerDock, m_aiDock, m_infoDock
  };
  for (auto* dock : docks) {
    if (!dock) continue;
    if (auto* tb = dynamic_cast<DockTitleBar*>(dock->titleBarWidget()))
      tb->rebuildTabs();
  }
}

void MainWindow::applyUiChrome() {
  // ── Design token palette ─────────────────────────────────────────
  // Base layer:    #13151c  (near-black, canvas surround)
  // Surface 0:     #1a1d27  (panel background)
  // Surface 1:     #212532  (groupbox, list background)
  // Surface 2:     #272c3c  (input fields, raised areas)
  // Surface 3:     #2f3447  (hover surface)
  // Border dim:    #2a2e3e
  // Border:        #363d54
  // Border bright: #4a5370
  // Accent:        #4e8ef7  (blue — interactive focus / selection)
  // Accent hover:  #6da6ff
  // Accent press:  #3a75e0
  // Text bright:   #edf0f9
  // Text normal:   #c5cde0
  // Text muted:    #7a86a3
  // Text disabled: #4a5268
  // ─────────────────────────────────────────────────────────────────

  setStyleSheet(
      // ── Global font + window ──────────────────────────────────────
      // Use QWidget (not *) to avoid styling Qt-internal transient widgets
      // like QRubberBand / dock drop indicators, which must render correctly.
      "QWidget { font-family: 'Segoe UI', sans-serif; font-size: 11px; }"
      "QMainWindow { background: #13151c; color: #c5cde0; }"
      // Panel background: scoped to known container types, not all QWidgets.
      // QRubberBand is intentionally excluded so dock drop indicators are visible.
      "QDockWidget > QWidget,"
      "QScrollArea > QWidget > QWidget,"
      "QGroupBox,"
      "QDialog,"
      "QTabWidget::pane,"
      "QStatusBar,"
      "QMenuBar,"
      "QToolBar "
      "{ background: #1a1d27; color: #c5cde0; }"
      "QRubberBand { background: rgba(78,142,247,60); border: 2px solid #4e8ef7; }"

      // ── Dock widgets ─────────────────────────────────────────────
      // Title bar: standalone/floating docks only (tabified docks suppress
      // their title bar; the tab bar is the drag+label handle instead).
      // Both title bar and tab bar share the same visual tokens → unified row.
      "QDockWidget { color: #c5cde0; font-size: 11px; }"
      "QDockWidget::title {"
      "  padding: 0 24px 0 8px;"   // right pad for float/close buttons
      "  background: #1c2030;"
      "  border-bottom: 1px solid #2a2e3e;"
      "  color: #8a9ab8;"
      "  font-size: 10px;"
      "  font-weight: 600;"
      "  letter-spacing: 0.5px;"
      "  text-transform: uppercase;"
      "  min-height: 20px;"
      "  max-height: 22px;"
      "}"
      "QDockWidget > QWidget { background: #1a1d27; }"
      "QDockWidget::float-button, QDockWidget::close-button {"
      "  background: transparent; border: none; padding: 0;"
      "  subcontrol-origin: margin;"
      "  width: 16px; height: 16px;"
      "  icon-size: 10px;"
      "}"
      "QDockWidget::float-button  { subcontrol-position: top right; margin-right: 18px; }"
      "QDockWidget::close-button  { subcontrol-position: top right; margin-right:  2px; }"
      "QDockWidget::float-button:hover, QDockWidget::close-button:hover {"
      "  background: #2f3650; border-radius: 3px;"
      "}"

      // ── Native dock tab bars — hidden ────────────────────────────────
      // DockTitleBar renders its own tab buttons (Photoshop/CLIP style).
      // The underlying tabifyDockWidget() structure is kept; only the visual
      // QTabBar is suppressed so the title bar row is a single 22px strip.
      "QTabBar[dockTabBar=\"true\"] {"
      "  max-height: 0px; min-height: 0px;"
      "  border: none; margin: 0; padding: 0;"
      "}"
      "QTabBar[dockTabBar=\"true\"]::tab { max-height: 0px; }"

      // ── Group boxes ───────────────────────────────────────────────
      "QGroupBox {"
      "  border: 1px solid #2a2e3e;"
      "  border-radius: 4px;"
      "  margin-top: 14px;"
      "  padding: 8px 6px 6px 6px;"
      "  background: #1a1d27;"
      "}"
      "QGroupBox::title {"
      "  subcontrol-origin: margin;"
      "  left: 10px;"
      "  padding: 0 4px;"
      "  color: #5a6480;"
      "  font-weight: 700;"
      "  font-size: 10px;"
      "  letter-spacing: 0.8px;"
      "  text-transform: uppercase;"
      "}"

      // ── List / tree widgets ───────────────────────────────────────
      "QListWidget, QTreeWidget {"
      "  background: #1a1d27;"
      "  border: 1px solid #2a2e3e;"
      "  border-radius: 3px;"
      "  color: #c5cde0;"
      "  outline: none;"
      "}"
      "QListWidget::item { min-height: 34px; padding-left: 6px; border-bottom: 1px solid #1e2230; }"
      "QListWidget::item:hover { background: #272c3c; }"
      "QListWidget::item:selected {"
      "  background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #1d3a7a, stop:1 #1e3566);"
      "  color: #edf0f9;"
      "  border-left: 2px solid #4e8ef7;"
      "}"

      // ── Push / tool buttons ───────────────────────────────────────
      "QPushButton {"
      "  background: #272c3c;"
      "  border: 1px solid #363d54;"
      "  border-radius: 4px;"
      "  padding: 4px 10px;"
      "  min-height: 24px;"
      "  color: #c5cde0;"
      "}"
      "QPushButton:hover {"
      "  background: #2f3447;"
      "  border-color: #4a5370;"
      "  color: #edf0f9;"
      "}"
      "QPushButton:pressed {"
      "  background: #3a75e0;"
      "  border-color: #4e8ef7;"
      "  color: #ffffff;"
      "}"
      "QPushButton:checked {"
      "  background: #1d3a7a;"
      "  border-color: #4e8ef7;"
      "  color: #edf0f9;"
      "}"
      "QPushButton:disabled {"
      "  background: #1a1d27;"
      "  color: #4a5268;"
      "  border-color: #252a38;"
      "}"
      "QToolButton {"
      "  background: transparent;"
      "  border: 1px solid transparent;"
      "  border-radius: 4px;"
      "  padding: 3px 5px;"
      "  min-height: 22px;"
      "  color: #c5cde0;"
      "}"
      "QToolButton:hover {"
      "  background: #2f3447;"
      "  border-color: #363d54;"
      "  color: #edf0f9;"
      "}"
      "QToolButton:pressed, QToolButton:checked {"
      "  background: #1d3a7a;"
      "  border-color: #4e8ef7;"
      "  color: #edf0f9;"
      "}"

      // ── Menu bar / toolbar ────────────────────────────────────────
      "QMenuBar {"
      "  background: #13151c;"
      "  color: #c5cde0;"
      "  border-bottom: 1px solid #2a2e3e;"
      "  padding: 2px 4px;"
      "}"
      "QMenuBar::item { padding: 4px 8px; border-radius: 3px; background: transparent; }"
      "QMenuBar::item:selected { background: #272c3c; color: #edf0f9; }"
      "QMenuBar::item:pressed { background: #1d3a7a; }"
      "QToolBar {"
      "  background: #13151c;"
      "  border-bottom: 1px solid #2a2e3e;"
      "  spacing: 2px;"
      "  padding: 2px 4px;"
      "}"
      "QToolBar::separator { width: 1px; background: #2a2e3e; margin: 4px 2px; }"
      "QMenu {"
      "  background: #21253a;"
      "  color: #c5cde0;"
      "  border: 1px solid #363d54;"
      "  border-radius: 6px;"
      "  padding: 4px 0;"
      "}"
      "QMenu::item { padding: 6px 28px 6px 14px; border-radius: 3px; margin: 1px 4px; }"
      "QMenu::item:selected { background: #1d3a7a; color: #edf0f9; }"
      "QMenu::item:disabled { color: #4a5268; }"
      "QMenu::separator { height: 1px; background: #2a2e3e; margin: 4px 8px; }"
      "QMenu::indicator { width: 14px; height: 14px; margin-left: 4px; }"

      // ── Input fields ──────────────────────────────────────────────
      "QLineEdit, QSpinBox, QDoubleSpinBox {"
      "  background: #13151c;"
      "  border: 1px solid #363d54;"
      "  border-radius: 4px;"
      "  color: #c5cde0;"
      "  min-height: 22px;"
      "  padding: 0 6px;"
      "  selection-background-color: #1d3a7a;"
      "}"
      "QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus {"
      "  border-color: #4e8ef7;"
      "  background: #191c27;"
      "}"
      "QLineEdit:disabled, QSpinBox:disabled { color: #4a5268; border-color: #252a38; }"
      "QSpinBox::up-button, QSpinBox::down-button {"
      "  background: #272c3c; border: none; border-radius: 2px; width: 14px;"
      "}"
      "QSpinBox::up-button:hover, QSpinBox::down-button:hover { background: #363d54; }"
      "QSpinBox::up-arrow { image: none; width: 0; height: 0;"
      "  border-left: 4px solid transparent; border-right: 4px solid transparent;"
      "  border-bottom: 5px solid #7a86a3; }"
      "QSpinBox::down-arrow { image: none; width: 0; height: 0;"
      "  border-left: 4px solid transparent; border-right: 4px solid transparent;"
      "  border-top: 5px solid #7a86a3; }"
      "QComboBox {"
      "  background: #13151c;"
      "  border: 1px solid #363d54;"
      "  border-radius: 4px;"
      "  color: #c5cde0;"
      "  min-height: 24px;"
      "  padding: 0 8px;"
      "}"
      "QComboBox:focus { border-color: #4e8ef7; }"
      "QComboBox::drop-down { border: none; width: 20px; }"
      "QComboBox::down-arrow {"
      "  image: none; width: 0; height: 0;"
      "  border-left: 4px solid transparent; border-right: 4px solid transparent;"
      "  border-top: 5px solid #7a86a3;"
      "}"
      "QComboBox QAbstractItemView {"
      "  background: #21253a; border: 1px solid #363d54; color: #c5cde0;"
      "  selection-background-color: #1d3a7a; selection-color: #edf0f9;"
      "  outline: none; padding: 2px;"
      "}"

      // ── Check boxes ───────────────────────────────────────────────
      "QCheckBox { color: #c5cde0; spacing: 6px; }"
      "QCheckBox::indicator {"
      "  width: 14px; height: 14px;"
      "  background: #13151c; border: 1px solid #363d54; border-radius: 3px;"
      "}"
      "QCheckBox::indicator:hover { border-color: #4e8ef7; background: #1e2230; }"
      "QCheckBox::indicator:checked {"
      "  background: #4e8ef7; border-color: #4e8ef7;"
      "  image: none;"
      "}"
      "QCheckBox::indicator:checked::after { color: white; }"
      "QCheckBox:disabled { color: #4a5268; }"

      // ── Sliders ───────────────────────────────────────────────────
      // groove margin reserves handle overhang — avoids fragile negative widget margins
      "QSlider { min-height: 20px; padding: 0; }"
      "QSlider::groove:horizontal {"
      "  background: #13151c;"
      "  height: 4px;"
      "  border-radius: 2px;"
      "  border: 1px solid #2a2e3e;"
      "  margin: 0 7px;"
      "}"
      "QSlider::sub-page:horizontal {"
      "  background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #2a5cbf, stop:1 #4e8ef7);"
      "  border-radius: 2px;"
      "  margin: 0 7px;"
      "}"
      "QSlider::handle:horizontal {"
      "  background: #edf0f9;"
      "  border: 1px solid #3a4252;"
      "  width: 14px; height: 14px;"
      "  border-radius: 7px;"
      "  margin: -5px -7px;"
      "}"
      "QSlider::handle:horizontal:hover { background: #ffffff; border-color: #6da6ff; }"
      "QSlider::handle:horizontal:pressed { background: #4e8ef7; border-color: #4e8ef7; }"
      "QSlider::groove:horizontal:disabled { background: #0e1016; border-color: #1a1d24; }"
      "QSlider::handle:horizontal:disabled { background: #2a2e3a; border-color: #1e2230; }"

      // ── Status bar ────────────────────────────────────────────────
      "QStatusBar {"
      "  background: #13151c;"
      "  border-top: 1px solid #2a2e3e;"
      "  color: #5a6480;"
      "  font-size: 10px;"
      "  padding: 0 8px;"
      "}"
      "QStatusBar::item { border: none; }"

      // ── Scroll bars ───────────────────────────────────────────────
      "QScrollBar:vertical {"
      "  background: #13151c; width: 6px; border: none; margin: 0;"
      "}"
      "QScrollBar::handle:vertical {"
      "  background: #363d54; border-radius: 3px; min-height: 24px;"
      "}"
      "QScrollBar::handle:vertical:hover { background: #4a5370; }"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
      "QScrollBar:horizontal {"
      "  background: #13151c; height: 6px; border: none; margin: 0;"
      "}"
      "QScrollBar::handle:horizontal {"
      "  background: #363d54; border-radius: 3px; min-width: 24px;"
      "}"
      "QScrollBar::handle:horizontal:hover { background: #4a5370; }"
      "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"

      // ── Tab widget ────────────────────────────────────────────────
      "QTabWidget::pane {"
      "  border: 1px solid #2a2e3e; border-radius: 4px; background: #1a1d27;"
      "  top: -1px;"
      "}"
      "QTabBar { background: transparent; }"
      "QTabBar::tab {"
      "  background: #13151c;"
      "  color: #5a6480;"
      "  border: 1px solid #2a2e3e;"
      "  border-bottom: none;"
      "  padding: 5px 8px;"
      "  min-width: 72px;"
      "  margin-right: 1px;"
      "  border-radius: 4px 4px 0 0;"
      "  font-size: 10px;"
      "}"
      "QTabBar::tab:selected {"
      "  background: #1a1d27;"
      "  color: #edf0f9;"
      "  border-color: #363d54;"
      "  border-bottom: 1px solid #1a1d27;"
      "}"
      "QTabBar::tab:hover:!selected { background: #1e2230; color: #c5cde0; }"

      // ── Frame / separator ─────────────────────────────────────────
      "QFrame[frameShape='4'], QFrame[frameShape='5'] { color: #2a2e3e; }"

      // ── Tooltip ───────────────────────────────────────────────────
      "QToolTip {"
      "  background: #21253a;"
      "  color: #edf0f9;"
      "  border: 1px solid #363d54;"
      "  border-radius: 4px;"
      "  padding: 4px 8px;"
      "  font-size: 11px;"
      "}"

      // ── Scroll area ───────────────────────────────────────────────
      "QScrollArea { border: none; background: transparent; }"
      "QScrollArea > QWidget > QWidget { background: transparent; }"

      // ── Splitter handles — thin, barely-visible dividers ──────────
      "QSplitter::handle { background: #2a2e3e; }"
      "QSplitter::handle:horizontal { width: 2px; }"
      "QSplitter::handle:vertical   { height: 2px; }"
      "QSplitter::handle:hover { background: #4e8ef7; }"

      // ── Header view (layer panel table headers etc.) ──────────────
      "QHeaderView::section {"
      "  background: #1a1d27; color: #5a6480; font-size: 10px;"
      "  border: none; border-bottom: 1px solid #2a2e3e; padding: 3px 6px;"
      "}"

      // ── Labels ────────────────────────────────────────────────────
      "QLabel { color: #c5cde0; background: transparent; }"

      // ── Progress bar ──────────────────────────────────────────────
      "QProgressBar {"
      "  background: #13151c; border: 1px solid #2a2e3e; border-radius: 3px;"
      "  color: #c5cde0; text-align: center; height: 8px;"
      "}"
      "QProgressBar::chunk { background: #4e8ef7; border-radius: 2px; }"
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
  const QSize labelSz = m_navigatorImageLabel->size().expandedTo(QSize(1, 1));
  QPixmap pixmap = QPixmap::fromImage(image).scaled(labelSz, Qt::KeepAspectRatio, Qt::SmoothTransformation);

  // Draw red viewport rectangle showing visible canvas area
  if (m_canvasWidget != nullptr) {
    const QRectF frac = m_canvasWidget->visibleCanvasFractionF();
    // The pixmap is centered in the label; compute actual pixmap placement
    const QRect pixRect(
        (labelSz.width()  - pixmap.width())  / 2,
        (labelSz.height() - pixmap.height()) / 2,
        pixmap.width(), pixmap.height());
    // Map fraction to pixmap coordinates
    const QRectF viewRect(
        frac.x() * pixmap.width(),
        frac.y() * pixmap.height(),
        frac.width() * pixmap.width(),
        frac.height() * pixmap.height());
    if (viewRect.width() < pixmap.width() - 2 || viewRect.height() < pixmap.height() - 2) {
      QPainter p(&pixmap);
      p.setPen(QPen(QColor(255, 50, 50), 1.5));
      p.setBrush(Qt::NoBrush);
      p.drawRect(viewRect.adjusted(1, 1, -1, -1));
    }
  }

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
  if (m_controller && m_controller->isDirty()) {
    const int ret = QMessageBox::question(
        this,
        "未保存の変更",
        "保存されていない変更があります。新規キャンバスを作成しますか？",
        QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (ret != QMessageBox::Discard) {
      return;
    }
  }
  QDialog dialog(this);
  dialog.setWindowTitle("新規キャンバス");
  dialog.setMinimumWidth(360);

  auto* root = new QVBoxLayout(&dialog);
  root->setSpacing(10);
  root->setContentsMargins(16, 14, 16, 14);

  // ── プリセット ─────────────────────────────────────────────────────────
  struct Preset { const char* name; int w; int h; int dpi; };
  static const Preset kPresets[] = {
    {"カスタム",         0,    0,   72},
    {"HD  1280×720",  1280,  720,   72},
    {"FHD 1920×1080", 1920, 1080,   72},
    {"4K  3840×2160", 3840, 2160,   72},
    {"A4 (72dpi)",    595,  842,   72},
    {"A4 (300dpi)",  2480, 3508,  300},
    {"B5 (72dpi)",   516,  729,   72},
    {"正方形 1000",   1000, 1000,   72},
    {"正方形 2000",   2000, 2000,   72},
  };
  auto* presetCombo = new QComboBox(&dialog);
  for (auto& p : kPresets) presetCombo->addItem(p.name);

  // ── サイズ入力 ─────────────────────────────────────────────────────────
  auto* sizeBox  = new QGroupBox("キャンバスサイズ", &dialog);
  auto* sizeGrid = new QGridLayout(sizeBox);
  sizeGrid->setSpacing(6);

  auto* widthSpin  = new QSpinBox(&dialog);
  auto* heightSpin = new QSpinBox(&dialog);
  auto* dpiSpin    = new QSpinBox(&dialog);
  auto* lockBtn    = new QPushButton("🔒", &dialog);
  widthSpin->setRange(1, 16384);  widthSpin->setSuffix(" px");
  heightSpin->setRange(1, 16384); heightSpin->setSuffix(" px");
  dpiSpin->setRange(1, 1200);     dpiSpin->setSuffix(" dpi");
  widthSpin->setValue(m_lastCanvasWidth);
  heightSpin->setValue(m_lastCanvasHeight);
  dpiSpin->setValue(m_lastCanvasDpi);
  lockBtn->setFixedSize(28, 28);
  lockBtn->setCheckable(true);
  lockBtn->setChecked(false);
  lockBtn->setToolTip("縦横比をロック");
  lockBtn->setStyleSheet(
    "QPushButton{border:1px solid #3a4460;border-radius:4px;background:#1e2338;font-size:13px;}"
    "QPushButton:checked{background:#2a3a5a;border-color:#4e8ef7;}"
    "QPushButton:hover{background:#262c48;}");

  // プレビューラベル（ピクセル数・印刷サイズ）
  auto* infoLabel = new QLabel(&dialog);
  infoLabel->setStyleSheet("color:#7a8aaa;font-size:10px;");

  auto updateInfo = [&]() {
    int w = widthSpin->value(), h = heightSpin->value(), d = dpiSpin->value();
    double mmW = w / (d / 25.4);
    double mmH = h / (d / 25.4);
    infoLabel->setText(QString("%1 × %2 px  (%3 × %4 mm @ %5dpi)")
      .arg(w).arg(h)
      .arg(mmW, 0, 'f', 1).arg(mmH, 0, 'f', 1).arg(d));
  };
  updateInfo();

  bool updatingSize = false;
  auto onWidthChanged = [&](int val) {
    if (updatingSize) return;
    if (lockBtn->isChecked() && heightSpin->value() > 0) {
      updatingSize = true;
      double ratio = static_cast<double>(heightSpin->value()) / widthSpin->value();
      if (val > 0) heightSpin->setValue(qRound(val * ratio));
      updatingSize = false;
    }
    updateInfo();
  };
  auto onHeightChanged = [&](int val) {
    if (updatingSize) return;
    if (lockBtn->isChecked() && widthSpin->value() > 0) {
      updatingSize = true;
      double ratio = static_cast<double>(widthSpin->value()) / heightSpin->value();
      if (val > 0) widthSpin->setValue(qRound(val * ratio));
      updatingSize = false;
    }
    updateInfo();
  };
  connect(widthSpin,  QOverload<int>::of(&QSpinBox::valueChanged), &dialog, onWidthChanged);
  connect(heightSpin, QOverload<int>::of(&QSpinBox::valueChanged), &dialog, onHeightChanged);
  connect(dpiSpin,    QOverload<int>::of(&QSpinBox::valueChanged), &dialog, [&](int){ updateInfo(); });

  // 向き切り替えボタン
  auto* orientRow = new QHBoxLayout();
  auto* portraitBtn  = new QPushButton("縦", &dialog);
  auto* landscapeBtn = new QPushButton("横", &dialog);
  portraitBtn->setCheckable(true);  portraitBtn->setChecked(true);
  landscapeBtn->setCheckable(true);
  const QString orientStyle =
    "QPushButton{border:1px solid #3a4460;border-radius:4px;background:#1e2338;padding:3px 14px;}"
    "QPushButton:checked{background:#2a3a5a;border-color:#4e8ef7;color:#c5d8ff;}"
    "QPushButton:hover{background:#262c48;}";
  portraitBtn->setStyleSheet(orientStyle);
  landscapeBtn->setStyleSheet(orientStyle);
  auto swapIfNeeded = [&](bool portrait) {
    int w = widthSpin->value(), h = heightSpin->value();
    if (portrait && w > h) { widthSpin->setValue(h); heightSpin->setValue(w); }
    else if (!portrait && w < h) { widthSpin->setValue(h); heightSpin->setValue(w); }
  };
  connect(portraitBtn,  &QPushButton::clicked, &dialog, [&](){ portraitBtn->setChecked(true);  landscapeBtn->setChecked(false); swapIfNeeded(true); });
  connect(landscapeBtn, &QPushButton::clicked, &dialog, [&](){ landscapeBtn->setChecked(true); portraitBtn->setChecked(false);  swapIfNeeded(false); });
  orientRow->addWidget(portraitBtn);
  orientRow->addWidget(landscapeBtn);
  orientRow->addStretch();

  // プリセット選択時
  connect(presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog, [&](int idx){
    if (idx <= 0 || kPresets[idx].w == 0) return;
    updatingSize = true;
    widthSpin->setValue(kPresets[idx].w);
    heightSpin->setValue(kPresets[idx].h);
    dpiSpin->setValue(kPresets[idx].dpi);
    updatingSize = false;
    updateInfo();
  });

  sizeGrid->addWidget(new QLabel("幅",  &dialog), 0, 0);
  sizeGrid->addWidget(widthSpin, 0, 1);
  sizeGrid->addWidget(lockBtn,   0, 2, 2, 1, Qt::AlignVCenter);
  sizeGrid->addWidget(new QLabel("高さ", &dialog), 1, 0);
  sizeGrid->addWidget(heightSpin, 1, 1);
  sizeGrid->addWidget(new QLabel("解像度", &dialog), 2, 0);
  sizeGrid->addWidget(dpiSpin, 2, 1);
  sizeGrid->addLayout(orientRow, 3, 0, 1, 3);
  sizeGrid->addWidget(infoLabel, 4, 0, 1, 3);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  buttons->button(QDialogButtonBox::Ok)->setText("作成");

  root->addWidget(new QLabel("プリセット", &dialog));
  root->addWidget(presetCombo);
  root->addWidget(sizeBox);
  root->addWidget(buttons);

  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  if (dialog.exec() != QDialog::Accepted) return;

  m_lastCanvasWidth  = widthSpin->value();
  m_lastCanvasHeight = heightSpin->value();
  m_lastCanvasDpi    = dpiSpin->value();
  m_controller->newDocument(m_lastCanvasWidth, m_lastCanvasHeight, m_lastCanvasDpi);
}

void MainWindow::onResizeCanvas() {
  if (!m_controller) return;
  const core::Size cur = m_controller->document().canvasSize();
  const int curW = cur.width, curH = cur.height;

  QDialog dialog(this);
  dialog.setWindowTitle("キャンバスサイズを変更");
  dialog.setMinimumWidth(380);

  auto* root = new QVBoxLayout(&dialog);
  root->setSpacing(10);
  root->setContentsMargins(16, 14, 16, 14);

  // 現在サイズ表示
  auto* curLabel = new QLabel(
    QString("現在のサイズ: %1 × %2 px").arg(curW).arg(curH), &dialog);
  curLabel->setStyleSheet("color:#7a8aaa;font-size:10px;");

  // 新サイズ入力
  auto* sizeBox  = new QGroupBox("新しいサイズ", &dialog);
  auto* sizeGrid = new QGridLayout(sizeBox);
  sizeGrid->setSpacing(6);
  auto* newW    = new QSpinBox(&dialog);
  auto* newH    = new QSpinBox(&dialog);
  auto* lockBtn = new QPushButton("🔒", &dialog);
  newW->setRange(1, 16384);  newW->setSuffix(" px");  newW->setValue(curW);
  newH->setRange(1, 16384);  newH->setSuffix(" px");  newH->setValue(curH);
  lockBtn->setFixedSize(28, 28);
  lockBtn->setCheckable(true);
  lockBtn->setToolTip("縦横比をロック");
  lockBtn->setStyleSheet(
    "QPushButton{border:1px solid #3a4460;border-radius:4px;background:#1e2338;font-size:13px;}"
    "QPushButton:checked{background:#2a3a5a;border-color:#4e8ef7;}"
    "QPushButton:hover{background:#262c48;}");

  bool updLock = false;
  connect(newW, QOverload<int>::of(&QSpinBox::valueChanged), &dialog, [&](int v){
    if (updLock || !lockBtn->isChecked()) return;
    updLock = true;
    if (curW > 0) newH->setValue(qRound(static_cast<double>(curH) / curW * v));
    updLock = false;
  });
  connect(newH, QOverload<int>::of(&QSpinBox::valueChanged), &dialog, [&](int v){
    if (updLock || !lockBtn->isChecked()) return;
    updLock = true;
    if (curH > 0) newW->setValue(qRound(static_cast<double>(curW) / curH * v));
    updLock = false;
  });

  sizeGrid->addWidget(new QLabel("幅",  &dialog), 0, 0);
  sizeGrid->addWidget(newW, 0, 1);
  sizeGrid->addWidget(lockBtn, 0, 2, 2, 1, Qt::AlignVCenter);
  sizeGrid->addWidget(new QLabel("高さ", &dialog), 1, 0);
  sizeGrid->addWidget(newH, 1, 1);

  // アンカーポイント（3×3グリッド）
  auto* anchorBox = new QGroupBox("配置（既存コンテンツの位置）", &dialog);
  auto* anchorGrid = new QGridLayout(anchorBox);
  anchorGrid->setSpacing(2);
  anchorGrid->setContentsMargins(8, 8, 8, 8);

  int anchorCol = 1, anchorRow = 1;  // デフォルト: 中央
  QVector<QPushButton*> anchorBtns;
  const QString anchorActive =
    "QPushButton{background:#2a3a5a;border:2px solid #4e8ef7;border-radius:3px;min-width:26px;min-height:26px;}";
  const QString anchorNormal =
    "QPushButton{background:#1e2338;border:1px solid #3a4460;border-radius:3px;min-width:26px;min-height:26px;}"
    "QPushButton:hover{background:#262c48;border-color:#5a7ab0;}";

  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      auto* btn = new QPushButton("", &dialog);
      btn->setFixedSize(28, 28);
      btn->setStyleSheet(r == 1 && c == 1 ? anchorActive : anchorNormal);
      const int rr = r, cc = c;
      connect(btn, &QPushButton::clicked, &dialog, [&, rr, cc](){
        anchorRow = rr; anchorCol = cc;
        for (int i = 0; i < anchorBtns.size(); ++i)
          anchorBtns[i]->setStyleSheet(
            (i / 3 == anchorRow && i % 3 == anchorCol) ? anchorActive : anchorNormal);
      });
      anchorBtns.append(btn);
      anchorGrid->addWidget(btn, r, c);
    }
  }
  // オフセット表示ラベル
  auto* offsetLabel = new QLabel(&dialog);
  offsetLabel->setStyleSheet("color:#7a8aaa;font-size:10px;");
  auto updateOffset = [&](){
    int ow = newW->value() - curW;
    int oh = newH->value() - curH;
    int ox = anchorCol == 0 ? 0 : (anchorCol == 1 ? ow/2 : ow);
    int oy = anchorRow == 0 ? 0 : (anchorRow == 1 ? oh/2 : oh);
    offsetLabel->setText(QString("オフセット: (%1, %2) px").arg(ox).arg(oy));
  };
  connect(newW, QOverload<int>::of(&QSpinBox::valueChanged), &dialog, [&](int){ updateOffset(); });
  connect(newH, QOverload<int>::of(&QSpinBox::valueChanged), &dialog, [&](int){ updateOffset(); });
  updateOffset();

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  buttons->button(QDialogButtonBox::Ok)->setText("変更");

  root->addWidget(curLabel);
  root->addWidget(sizeBox);
  root->addWidget(anchorBox);
  root->addWidget(offsetLabel);
  root->addWidget(buttons);

  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  if (dialog.exec() != QDialog::Accepted) return;

  // アンカーポイントからオフセット計算
  // anchorCol/Row: 0=左上, 1=中央, 2=右下 → 既存コンテンツの左上が移動先
  const int dw = newW->value() - curW;
  const int dh = newH->value() - curH;
  const int offX = anchorCol == 0 ? 0 : (anchorCol == 1 ? dw / 2 : dw);
  const int offY = anchorRow == 0 ? 0 : (anchorRow == 1 ? dh / 2 : dh);

  m_controller->resizeCanvas(newW->value(), newH->value(), offX, offY);
}

void MainWindow::onToolStateChanged() {
  const app::bridge::ToolStateViewModel state = m_controller->toolState();
  // 透明色以外のときだけ「最後の有色描画色」を更新（X キーで復帰するために保持）
  if (state.color.a > 0) {
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
  if (m_controller && m_controller->isDirty()) {
    const int ret = QMessageBox::question(
        this,
        "未保存の変更",
        "保存されていない変更があります。ファイルを開きますか？",
        QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (ret != QMessageBox::Discard) {
      return;
    }
  }
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
  if (m_controller && m_controller->isDirty()) {
    const int ret = QMessageBox::question(
        this,
        "未保存の変更",
        "保存されていない変更があります。クリップボードから新規キャンバスを作成しますか？",
        QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (ret != QMessageBox::Discard) {
      return;
    }
  }
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
    QLabel* label {nullptr};
    QString category;
  };

  // カテゴリ表示名マップ (commandId prefix → 日本語)
  static const QMap<QString, QString> kCategoryNames {
      {"file",   "ファイル"},
      {"edit",   "編集"},
      {"layer",  "レイヤー"},
      {"select", "選択"},
      {"view",   "表示"},
      {"tool",   "ツール"},
      {"window", "ウィンドウ"},
      {"color",  "カラー"},
      {"help",   "ヘルプ"},
  };

  QList<QAction*> configurable;
  const QList<QAction*> allActions = findChildren<QAction*>();
  for (QAction* action : allActions) {
    if (action != nullptr && action->property("commandId").isValid()) {
      configurable.push_back(action);
    }
  }
  // カテゴリ→テキスト順でソート
  std::sort(configurable.begin(), configurable.end(), [](const QAction* lhs, const QAction* rhs) {
    const QString lCat = lhs->property("commandId").toString().section('.', 0, 0);
    const QString rCat = rhs->property("commandId").toString().section('.', 0, 0);
    if (lCat != rCat) return lCat < rCat;
    return lhs->text() < rhs->text();
  });

  QDialog dialog(this);
  dialog.setWindowTitle("ショートカット設定");
  dialog.resize(680, 680);
  auto* root = new QVBoxLayout(&dialog);
  root->setContentsMargins(10, 10, 10, 10);
  root->setSpacing(6);

  // ── 検索バー ─────────────────────────────────────────────────────────────
  auto* searchRow = new QHBoxLayout();
  auto* searchLabel = new QLabel("検索:", &dialog);
  auto* searchEdit = new QLineEdit(&dialog);
  searchEdit->setPlaceholderText("コマンド名またはショートカットキーで絞り込み...");
  searchEdit->setClearButtonEnabled(true);
  searchRow->addWidget(searchLabel);
  searchRow->addWidget(searchEdit, 1);
  root->addLayout(searchRow);

  auto* help = new QLabel(
      "空欄 = 割り当て解除。変更後「OK」で適用・保存されます。重複は警告されます。",
      &dialog);
  help->setWordWrap(true);
  help->setStyleSheet("color: #8a9ab5; font-size: 11px;");
  root->addWidget(help);

  // ── スクロールエリア + フォームレイアウト ─────────────────────────────────
  auto* scroll = new QScrollArea(&dialog);
  scroll->setWidgetResizable(true);
  auto* host = new QWidget(scroll);
  auto* form = new QVBoxLayout(host);
  form->setContentsMargins(4, 4, 4, 4);
  form->setSpacing(2);

  std::vector<ShortcutRow> rows;
  rows.reserve(static_cast<std::size_t>(configurable.size()));
  QString lastCat;

  for (QAction* action : configurable) {
    const QString commandId = action->property("commandId").toString();
    const QString cat = commandId.section('.', 0, 0);
    const QString catName = kCategoryNames.value(cat, cat);

    // カテゴリヘッダー
    if (cat != lastCat) {
      lastCat = cat;
      auto* catHeader = new QLabel(catName, host);
      catHeader->setStyleSheet(
          "font-weight: bold; font-size: 12px; color: #4e8ef7;"
          "padding: 6px 2px 2px 2px; border-bottom: 1px solid #2a2e3e;");
      form->addWidget(catHeader);
    }

    // 行ウィジェット
    auto* rowWidget = new QWidget(host);
    auto* rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(4, 1, 4, 1);
    rowLayout->setSpacing(8);

    auto* lbl = new QLabel(action->text().remove('&'), rowWidget);
    lbl->setMinimumWidth(220);
    lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto* edit = new QKeySequenceEdit(action->shortcut(), rowWidget);
    edit->setClearButtonEnabled(true);
    edit->setFixedWidth(200);

    rowLayout->addWidget(lbl, 1);
    rowLayout->addWidget(edit);
    form->addWidget(rowWidget);
    rows.push_back(ShortcutRow {action, edit, lbl, cat});
  }
  form->addStretch();
  host->setLayout(form);
  scroll->setWidget(host);
  root->addWidget(scroll, 1);

  // ── 検索フィルタ接続 ───────────────────────────────────────────────────
  connect(searchEdit, &QLineEdit::textChanged, &dialog, [&rows](const QString& text) {
    const QString lower = text.toLower();
    for (const ShortcutRow& row : rows) {
      if (row.label == nullptr || row.edit == nullptr) continue;
      const bool match = lower.isEmpty()
          || row.label->text().toLower().contains(lower)
          || row.edit->keySequence().toString().toLower().contains(lower);
      // 行ウィジェット(parentWidget)の表示を切り替え
      if (row.edit->parentWidget() != nullptr) {
        row.edit->parentWidget()->setVisible(match);
      }
    }
  });

  // ── ボタン ────────────────────────────────────────────────────────────
  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  auto* resetButton = buttons->addButton("初期値に戻す", QDialogButtonBox::ResetRole);
  root->addWidget(buttons);

  connect(resetButton, &QPushButton::clicked, &dialog, [&rows]() {
    for (const ShortcutRow& row : rows) {
      if (row.action == nullptr || row.edit == nullptr) continue;
      const QString defaultText = row.action->property("defaultShortcut").toString();
      row.edit->setKeySequence(QKeySequence::fromString(defaultText, QKeySequence::PortableText));
    }
  });
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  // ── 重複チェック ────────────────────────────────────────────────────────
  QMap<QString, QStringList> duplicates;
  for (const ShortcutRow& row : rows) {
    if (row.action == nullptr || row.edit == nullptr) continue;
    const QString key = row.edit->keySequence().toString(QKeySequence::PortableText);
    if (!key.isEmpty()) {
      duplicates[key].push_back(row.label != nullptr ? row.label->text() : row.action->text().remove('&'));
    }
  }
  QStringList conflictLines;
  for (auto it = duplicates.cbegin(); it != duplicates.cend(); ++it) {
    if (it.value().size() > 1) {
      conflictLines.push_back(QString("  %1  →  %2").arg(it.key(), it.value().join(" / ")));
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

  // ── 適用・保存 ─────────────────────────────────────────────────────────
  for (const ShortcutRow& row : rows) {
    if (row.action == nullptr || row.edit == nullptr) continue;
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
  m_controller->setSecondaryColor(m_backgroundColor);
  updateColorPanel();
}

void MainWindow::onSwapColors() {
  // Swap ONLY the active drawing colors — must not touch layer pixels or paper color.
  // setPaperColor() is forbidden here: it changes the document compositing background
  // which makes the canvas appear to change (BUG). Paper color is independent.
  const core::Color foreground = m_controller->toolState().color;
  const core::Color background = m_backgroundColor;
  m_controller->setBrushColor(background);   // new foreground = old background
  m_backgroundColor = foreground;             // new background = old foreground
  m_controller->setSecondaryColor(m_backgroundColor); // update tool secondary
  // DO NOT call setPaperColor — paper stays unchanged, canvas pixels stay unchanged
  updateColorPanel();
}

void MainWindow::onResetBlackWhiteColors() {
  m_controller->setBrushColor(core::Color::OpaqueBlack());
  m_backgroundColor = core::Color {255, 255, 255, 255};
  m_controller->setPaperColor(m_backgroundColor);
  m_controller->setSecondaryColor(m_backgroundColor);
  updateColorPanel();
}

void MainWindow::onConnectComfyUiTriggered() {
  bool ok = false;
  const QString url = QInputDialog::getText(
      this,
      "ComfyUI 接続",
      "ComfyUI サーバー URL:",
      QLineEdit::Normal,
      "http://localhost:8188",
      &ok);
  if (!ok || url.trimmed().isEmpty()) {
    return;
  }
  statusBar()->showMessage(QString("ComfyUI に接続中: %1").arg(url.trimmed()), 2000);
  m_controller->connectComfyUi(url.trimmed());
}

void MainWindow::onComfyUiStateChanged(bool connected) {
  if (m_comfyUiStatusLabel == nullptr) {
    return;
  }
  if (connected) {
    m_comfyUiStatusLabel->setText("ComfyUI: 接続済み ●");
    m_comfyUiStatusLabel->setStyleSheet("color: #4caf50; font-size: 10px; font-weight: 600;");
    statusBar()->showMessage("ComfyUI に接続しました。AI 選択ツールで高精度選択が利用可能です。", 3000);
  } else {
    m_comfyUiStatusLabel->setText("ComfyUI: 未接続");
    m_comfyUiStatusLabel->setStyleSheet("color: #6a7484; font-size: 10px;");
  }
}

void MainWindow::onGenerativeFillTriggered() {
  const core::PixelBuffer& canvas = m_controller->compositedBuffer();
  if (canvas.width() <= 0 || canvas.height() <= 0) {
    statusBar()->showMessage("キャンバスが空です", 1800);
    return;
  }
  const core::SelectionMask& mask = m_controller->document().selection();

  app::panels::GenerativeFillDialog dlg(canvas, mask, this);
  if (dlg.exec() != QDialog::Accepted || !dlg.hasResult()) {
    return;
  }

  // 生成結果を新規ラスターレイヤーとして追加
  if (m_controller->pasteBufferAsNewRasterLayer(dlg.result(), "AI 生成塗りつぶし")) {
    updateUndoRedoState();
    statusBar()->showMessage("AI 生成結果を新規レイヤーとして追加しました", 2500);
  }
}

// ── 画像調整ダイアログ ────────────────────────────────────────────────────

void MainWindow::onBrightnessContrastTriggered() {
  if (m_controller == nullptr) {
    return;
  }
  QDialog dlg(this);
  dlg.setWindowTitle("明るさ・コントラスト");
  dlg.setFixedWidth(320);

  auto* layout = new QVBoxLayout(&dlg);
  auto* form   = new QFormLayout();

  auto* brightnessSlider = new QSlider(Qt::Horizontal, &dlg);
  brightnessSlider->setRange(-100, 100);
  brightnessSlider->setValue(0);
  auto* brightnessSpin   = new QSpinBox(&dlg);
  brightnessSpin->setRange(-100, 100);
  brightnessSpin->setValue(0);
  auto* brightnessRow = new QHBoxLayout();
  brightnessRow->addWidget(brightnessSlider);
  brightnessRow->addWidget(brightnessSpin);

  auto* contrastSlider = new QSlider(Qt::Horizontal, &dlg);
  contrastSlider->setRange(-100, 100);
  contrastSlider->setValue(0);
  auto* contrastSpin   = new QSpinBox(&dlg);
  contrastSpin->setRange(-100, 100);
  contrastSpin->setValue(0);
  auto* contrastRow = new QHBoxLayout();
  contrastRow->addWidget(contrastSlider);
  contrastRow->addWidget(contrastSpin);

  form->addRow("明るさ:", brightnessRow);
  form->addRow("コントラスト:", contrastRow);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
  layout->addLayout(form);
  layout->addWidget(buttons);

  // 双方向バインド
  QObject::connect(brightnessSlider, &QSlider::valueChanged, brightnessSpin, &QSpinBox::setValue);
  QObject::connect(brightnessSpin, QOverload<int>::of(&QSpinBox::valueChanged), brightnessSlider, &QSlider::setValue);
  QObject::connect(contrastSlider, &QSlider::valueChanged, contrastSpin, &QSpinBox::setValue);
  QObject::connect(contrastSpin, QOverload<int>::of(&QSpinBox::valueChanged), contrastSlider, &QSlider::setValue);

  QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

  if (dlg.exec() != QDialog::Accepted) {
    return;
  }
  if (m_controller->adjustBrightnessContrast(brightnessSpin->value(), contrastSpin->value())) {
    updateUndoRedoState();
    statusBar()->showMessage("明るさ・コントラストを調整しました", 1800);
  }
}

void MainWindow::onHueSatLightTriggered() {
  if (m_controller == nullptr) {
    return;
  }
  QDialog dlg(this);
  dlg.setWindowTitle("色相・彩度・明度");
  dlg.setFixedWidth(340);

  auto* layout = new QVBoxLayout(&dlg);
  auto* form   = new QFormLayout();

  auto makeRow = [&](QSlider*& slider, QSpinBox*& spin, int lo, int hi) {
    slider = new QSlider(Qt::Horizontal, &dlg);
    slider->setRange(lo, hi);
    slider->setValue(0);
    spin = new QSpinBox(&dlg);
    spin->setRange(lo, hi);
    spin->setValue(0);
    auto* row = new QHBoxLayout();
    row->addWidget(slider);
    row->addWidget(spin);
    QObject::connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
    QObject::connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), slider, &QSlider::setValue);
    return row;
  };

  QSlider *hueSlider {}, *satSlider {}, *lightSlider {};
  QSpinBox *hueSpin  {}, *satSpin  {}, *lightSpin  {};
  form->addRow("色相:", makeRow(hueSlider, hueSpin, -180, 180));
  form->addRow("彩度:", makeRow(satSlider, satSpin, -100, 100));
  form->addRow("明度:", makeRow(lightSlider, lightSpin, -100, 100));

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
  layout->addLayout(form);
  layout->addWidget(buttons);

  QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

  if (dlg.exec() != QDialog::Accepted) {
    return;
  }
  if (m_controller->adjustHueSaturationLightness(hueSpin->value(), satSpin->value(), lightSpin->value())) {
    updateUndoRedoState();
    statusBar()->showMessage("色相・彩度・明度を調整しました", 1800);
  }
}

void MainWindow::onUseTransparentColor() {
  const core::Color current = m_controller->toolState().color;
  if (current.a == 0) {
    // すでに透明 → 最後の有色描画色に戻す
    m_controller->setBrushColor(m_lastForegroundColor);
  } else {
    // 有色 → 透明色に切り替え（RGBはそのまま、alpha=0）
    core::Color transparent = current;
    transparent.a = 0;
    m_controller->setBrushColor(transparent);
  }
  updateColorPanel();
}

void MainWindow::updateColorPanel() {
  if (m_colorSwatchWidget == nullptr) {
    return;
  }

  const QColor fg = toQColor(m_controller->toolState().color);
  const QColor bg = toQColor(m_backgroundColor);

  auto* swatchWidget = static_cast<ColorSwatchWidget*>(m_colorSwatchWidget);
  swatchWidget->setForegroundColor(fg);
  swatchWidget->setBackgroundColor(bg);

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

void MainWindow::auditUIMetrics() {
  // audit removed
  return;
  std::ofstream f;
  const char* paths[] = {
    "ui_audit.txt"
  };
  for (auto* p : paths) {
    f.open(p, std::ios::out | std::ios::trunc);
    if (f.is_open()) { f << "Path: " << p << "\n"; break; }
  }
  if (!f.is_open()) { qDebug() << "AUDIT: cannot open any file"; return; }

  f << "========== UI METRICS AUDIT ==========\n";
  f << "Function called OK\n";
  f.flush();

  // Title rule check
  int duplicateTitles = 0;
  for (auto* dock : findChildren<QDockWidget*>()) {
    QString dockTitle = dock->windowTitle();
    for (auto* label : dock->findChildren<QLabel*>()) {
      if (label->isVisible() && label->text() == dockTitle) {
        f << "DUPLICATE TITLE: " << dockTitle.toStdString() << "\n";
        duplicateTitles++;
      }
    }
  }
  f << "Duplicate titles found: " << duplicateTitles << "\n";

  // SubTool panel (m_subToolPanel, target 200-260px wide)
  if (m_subToolPanel) {
    f << "\nSUBTOOL PANEL:\n";
    f << "  Width: " << m_subToolPanel->width() << "\n";
    f << "  Height: " << m_subToolPanel->height() << "\n";
    for (auto* lw : m_subToolPanel->findChildren<QListWidget*>()) {
      f << "  ListWidget rows: " << lw->count() << "\n";
      if (lw->count() > 0) {
        auto* iw = lw->itemWidget(lw->item(0));
        if (iw) f << "  Item widget height: " << iw->height() << "\n";
        else    f << "  Item sizeHintForRow: " << lw->sizeHintForRow(0) << "\n";
      }
    }
  }
  // Tool button panel (m_toolPanel, ButtonsOnly, ~68px)
  if (m_toolPanel) {
    f << "\nTOOL BUTTON PANEL:\n";
    f << "  Width: " << m_toolPanel->width() << "\n";
  }

  // Color swatch
  if (m_colorSwatchWidget) {
    f << "\nCOLOR SWATCH:\n";
    f << "  Size: " << m_colorSwatchWidget->width() << "x" << m_colorSwatchWidget->height() << "\n";
    f << "  SizeHint: " << m_colorSwatchWidget->sizeHint().width() << "x" << m_colorSwatchWidget->sizeHint().height() << "\n";
    f << "  Min: " << m_colorSwatchWidget->minimumWidth() << "x" << m_colorSwatchWidget->minimumHeight() << "\n";
    f << "  Max: " << m_colorSwatchWidget->maximumWidth() << "x" << m_colorSwatchWidget->maximumHeight() << "\n";
  }

  // Sliders
  if (m_hueSlider && m_satSlider && m_valSlider && m_alphaSlider) {
    f << "\nSLIDER ROWS:\n";
    f << "  Hue: " << m_hueSlider->height() << "\n";
    f << "  Sat: " << m_satSlider->height() << "\n";
    f << "  Val: " << m_valSlider->height() << "\n";
    f << "  Alpha: " << m_alphaSlider->height() << "\n";
    if (m_hueSpin) f << "  SpinBox: " << m_hueSpin->height() << "\n";
  }

  // Layer panel
  if (m_layerPanel) {
    f << "\nLAYER PANEL:\n";
    f << "  Width: " << m_layerPanel->width() << "\n";
    f << "  Height: " << m_layerPanel->height() << "\n";
    for (auto* lw : m_layerPanel->findChildren<QListWidget*>()) {
      f << "  Layer list rows: " << lw->count() << "\n";
      if (lw->count() > 0) f << "  Layer sizeHintForRow: " << lw->sizeHintForRow(0) << "\n";
    }
  }

  // ── Transparent button geometry ─────────────────────────────────────────
  f << "\nTRANSPARENT BUTTON:\n";
  // Find transparentColorButton by property (can't store as member without header change)
  QWidget* transparentBtn = nullptr;
  for (auto* btn : findChildren<QPushButton*>()) {
    if (btn->toolTip() == "透明色で描画（アルファ消去）") { transparentBtn = btn; break; }
  }
  if (transparentBtn) {
    f << "  Width: " << transparentBtn->width() << "  Height: " << transparentBtn->height() << "\n";
    f << "  " << (transparentBtn->width() <= 24 && transparentBtn->height() <= 24 ? "PASS" : "FAIL") << " (target <= 24x24)\n";
  } else {
    f << "  NOT FOUND\n";
  }

  // ── Swap test: verify canvas pixel NOT changed ────────────────────────────
  f << "\nSWAP PIXEL INVARIANCE TEST:\n";
  if (m_controller) {
    const core::PixelBuffer& buf = m_controller->compositedBuffer();
    if (buf.width() > 0 && buf.height() > 0) {
      // Sample center pixel before swap using PixelBuffer::pixel(x,y) API
      const int cx = buf.width() / 2, cy = buf.height() / 2;
      const core::Color px0 = buf.pixel(cx, cy);
      // Execute swap
      const core::Color preFg = m_controller->toolState().color;
      const core::Color preBg = m_backgroundColor;
      onSwapColors();
      // Sample after — composited buffer is re-read (no rerender called → same pixels)
      const core::Color px1 = m_controller->compositedBuffer().pixel(cx, cy);
      f << "  Before pixel [" << cx << "," << cy << "]: rgba("
        << (int)px0.r << "," << (int)px0.g << "," << (int)px0.b << "," << (int)px0.a << ")\n";
      f << "  After swap pixel: rgba("
        << (int)px1.r << "," << (int)px1.g << "," << (int)px1.b << "," << (int)px1.a << ")\n";
      const bool pixelUnchanged = (px0.r == px1.r && px0.g == px1.g && px0.b == px1.b && px0.a == px1.a);
      f << "  Pixel unchanged: " << (pixelUnchanged ? "PASS" : "FAIL") << "\n";
      // Restore original colors
      m_controller->setBrushColor(preFg);
      m_backgroundColor = preBg;
      m_controller->setSecondaryColor(m_backgroundColor);
      updateColorPanel();
    } else {
      f << "  Canvas empty — cannot test pixel invariance\n";
    }
  }

  // ── Density audit: occupied child area vs total panel area ──────────────
  f << "\n========== DENSITY AUDIT ==========\n";
  auto measureDensity = [&f](QWidget* panel, const std::string& name) {
    if (!panel) return;
    const int totalArea = panel->width() * panel->height();
    if (totalArea <= 0) return;
    int occupiedArea = 0;
    for (auto* child : panel->findChildren<QWidget*>()) {
      if (!child->isVisible()) continue;
      if (child == panel) continue;
      // Only count direct layout members (mapped to panel coords)
      QRect r = child->rect().translated(child->mapTo(panel, QPoint(0,0)));
      QRect clipped = r.intersected(panel->rect());
      occupiedArea += clipped.width() * clipped.height();
    }
    // Cap to avoid overlap overcounting (children can overlap)
    occupiedArea = std::min(occupiedArea, totalArea);
    const int emptyArea = totalArea - occupiedArea;
    const double density = 100.0 * occupiedArea / totalArea;
    const double emptyPct = 100.0 * emptyArea / totalArea;
    f << name << ":\n";
    f << "  Total: " << totalArea << "px2  Occupied: " << occupiedArea << "px2\n";
    f << "  Density: " << int(density) << "%  Empty: " << int(emptyPct) << "%\n";
    f << "  " << (density >= 85.0 ? "PASS" : "FAIL") << " (target >= 85%)\n";
  };

  // SubTool dock content
  if (m_subToolDock) measureDensity(m_subToolDock->widget(), "SubTool");
  // Color dock content
  if (m_colorDock) measureDensity(m_colorDock->widget(), "Color");
  // Layer dock content
  measureDensity(m_layerPanel, "Layer");
  // Info dock content
  if (m_infoDock) measureDensity(m_infoDock->widget(), "Info/Navigator");

  // ── Layout margin/spacing violations ────────────────────────────────────
  f << "\n========== LAYOUT VIOLATIONS ==========\n";
  int violations = 0;
  auto checkLayout = [&f, &violations](QLayout* layout, const std::string& ctx) {
    if (!layout) return;
    QMargins m = layout->contentsMargins();
    int sp = layout->spacing();
    bool bad = (m.left() > 2 || m.right() > 2 || m.top() > 2 || m.bottom() > 2 || sp > 2);
    if (bad) {
      f << "VIOLATION " << ctx << ": margins(" << m.left() << "," << m.top() << "," << m.right() << "," << m.bottom() << ") spacing=" << sp << "\n";
      violations++;
    }
  };
  if (m_colorPanelWidget) {
    for (auto* child : m_colorPanelWidget->findChildren<QLayout*>()) {
      checkLayout(child, "ColorPanel." + child->objectName().toStdString());
    }
    checkLayout(m_colorPanelWidget->layout(), "ColorPanel.root");
  }
  if (m_subToolPanel) {
    checkLayout(m_subToolPanel->layout(), "SubToolPanel.root");
  }
  if (m_infoDock && m_infoDock->widget()) {
    checkLayout(m_infoDock->widget()->layout(), "InfoPanel.root");
  }
  f << "Layout violations: " << violations << " (target: 0)\n";

  // ── SubTool list visible item count in 300px ────────────────────────────
  f << "\n========== SUBTOOL ITEM COUNT ==========\n";
  if (m_subToolPanel) {
    for (auto* lw : m_subToolPanel->findChildren<QListWidget*>()) {
      const int rowH = lw->sizeHintForRow(0);
      const int visibleIn300 = rowH > 0 ? (300 / rowH) : 0;
      f << "  Row height: " << rowH << "px\n";
      f << "  Visible in 300px: " << visibleIn300 << "\n";
      f << "  " << (visibleIn300 >= 9 ? "PASS" : "FAIL") << " (target >= 9)\n";
    }
  }

  f << "\n========== AUDIT END ==========\n";
  f.close();
  qDebug() << "Audit written to C:\\Portfolio\\Paint_App\\ui_audit.txt";
}

} // namespace app::mainwindow



