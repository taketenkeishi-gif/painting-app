#include "app/canvasview/CanvasWidget.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include <QApplication>
#include <QCursor>
#include <QPixmap>
#include <QElapsedTimer>
#include <QEvent>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QTimerEvent>
#include <QFontDatabase>
#include <QWheelEvent>

#include "app/bridge/AppController.h"
#include "core/tools/ToolType.h"
#include "platform/qt/QtImageConverter.h"

namespace app::canvasview {

namespace {

struct CanvasInteractionState {
  double zoom {1.0};
  double rotationDegrees {0.0};
  QPointF panOffset {0.0, 0.0};
  bool panning {false};
  bool panDragging {false};
  bool temporaryMiddlePan {false};
  QPoint lastPanPos {0, 0};
  QPoint lastMousePos {0, 0};
  bool hasMousePos {false};
  QPoint lastStrokeDispatchWidgetPos {0, 0};
  bool hasLastStrokeDispatchPos {false};
  qint64 lastStrokeDispatchNs {0};
};

std::unordered_map<const CanvasWidget*, CanvasInteractionState> g_canvasStates;
QElapsedTimer g_eventTimer;

CanvasInteractionState& stateFor(const CanvasWidget* widget) {
  if (!g_eventTimer.isValid()) {
    g_eventTimer.start();
  }
  return g_canvasStates[widget];
}

void updateZoomStatusLabel(const CanvasWidget* widget) {
  if (widget == nullptr || widget->window() == nullptr) {
    return;
  }
  auto* zoomLabel = widget->window()->findChild<QLabel*>("ZoomStatusLabel");
  if (zoomLabel == nullptr) {
    return;
  }
  const int percent = static_cast<int>(std::lround(stateFor(widget).zoom * 100.0));
  zoomLabel->setText(QString("ズーム: %1%").arg(percent));
}

void drawCheckerboard(QPainter& painter, const QRect& rect, int cellSize) {
  if (rect.width() <= 0 || rect.height() <= 0) {
    return;
  }
  const int cell = std::max(4, cellSize);
  const QColor a(112, 116, 124);
  const QColor b(86, 90, 98);
  for (int y = rect.top(); y <= rect.bottom(); y += cell) {
    for (int x = rect.left(); x <= rect.right(); x += cell) {
      const bool useA = ((x / cell) + (y / cell)) % 2 == 0;
      const QRect tile(x, y, std::min(cell, rect.right() - x + 1), std::min(cell, rect.bottom() - y + 1));
      painter.fillRect(tile, useA ? a : b);
    }
  }
}

void drawSelectionHandles(QPainter& painter, const QRectF& selectionRect, const QColor& color, double handleSize) {
  painter.setBrush(Qt::NoBrush);
  painter.setPen(QPen(color, 1.4, Qt::DashLine));
  painter.drawRect(selectionRect);
  painter.setBrush(color);
  painter.setPen(Qt::NoPen);
  const double h = handleSize;
  painter.drawRect(QRectF(selectionRect.topLeft().x() - h / 2.0, selectionRect.topLeft().y() - h / 2.0, h, h));
  painter.drawRect(QRectF(selectionRect.topRight().x() - h / 2.0, selectionRect.topRight().y() - h / 2.0, h, h));
  painter.drawRect(QRectF(selectionRect.bottomLeft().x() - h / 2.0, selectionRect.bottomLeft().y() - h / 2.0, h, h));
  painter.drawRect(QRectF(selectionRect.bottomRight().x() - h / 2.0, selectionRect.bottomRight().y() - h / 2.0, h, h));
}

Qt::CursorShape cursorForTool(core::ToolKind tool, bool dragging) {
  switch (tool) {
    case core::ToolKind::Brush:
    case core::ToolKind::Eraser:
      return Qt::BlankCursor;
    case core::ToolKind::Eyedropper:
      return Qt::CrossCursor;
    case core::ToolKind::Fill:
      return Qt::CrossCursor;
    case core::ToolKind::Line:
    case core::ToolKind::RectSelection:
      return Qt::CrossCursor;
    case core::ToolKind::MoveLayer:
      return dragging ? Qt::ClosedHandCursor : Qt::OpenHandCursor;
    case core::ToolKind::Hand:
      return dragging ? Qt::ClosedHandCursor : Qt::OpenHandCursor;
    case core::ToolKind::Zoom:
      return Qt::SizeVerCursor;
    default:
      return Qt::ArrowCursor;
  }
}

// text-editor-cursor-hit-helpers
int distanceSquared(const core::Point& a, const core::Point& b) {
  const int dx = a.x - b.x;
  const int dy = a.y - b.y;
  return dx * dx + dy * dy;
}

bool inRect(const core::Rect& rect, const core::Point& point, int inflate) {
  return point.x >= rect.x - inflate &&
         point.y >= rect.y - inflate &&
         point.x <= rect.x + rect.width + inflate &&
         point.y <= rect.y + rect.height + inflate;
}

int canvasCursorDistanceSquared(core::Point a, core::Point b) {
  const int dx = a.x - b.x;
  const int dy = a.y - b.y;
  return dx * dx + dy * dy;
}

bool canvasCursorNearPoint(core::Point a, core::Point b, int radius = 12) {
  return canvasCursorDistanceSquared(a, b) <= radius * radius;
}

bool canvasCursorInRect(const core::Rect& r, core::Point p, int pad = 6) {
  return p.x >= r.x - pad && p.y >= r.y - pad && p.x <= r.x + r.width + pad && p.y <= r.y + r.height + pad;
}

bool canvasCursorOnRectFrame(const core::Rect& r, core::Point p, int pad = 6) {
  if (!canvasCursorInRect(r, p, pad)) {
    return false;
  }
  const bool nearLeft = std::abs(p.x - r.x) <= pad;
  const bool nearRight = std::abs(p.x - (r.x + r.width)) <= pad;
  const bool nearTop = std::abs(p.y - r.y) <= pad;
  const bool nearBottom = std::abs(p.y - (r.y + r.height)) <= pad;
  return nearLeft || nearRight || nearTop || nearBottom;
}

int operationCursorModeForObjectOverlay(const app::bridge::CanvasOverlayViewModel& overlay, core::Point point) {
  if (!overlay.objectSelectionRect.has_value()) {
    return 0;
  }
  const core::Rect b = *overlay.objectSelectionRect;
  const core::Point tl {b.x, b.y};
  const core::Point t {b.x + b.width / 2, b.y};
  const core::Point tr {b.x + b.width, b.y};
  const core::Point l {b.x, b.y + b.height / 2};
  const core::Point r {b.x + b.width, b.y + b.height / 2};
  const core::Point bl {b.x, b.y + b.height};
  const core::Point btm {b.x + b.width / 2, b.y + b.height};
  const core::Point br {b.x + b.width, b.y + b.height};
  const core::Point rot {b.x + b.width / 2, b.y - 22};
  if (canvasCursorNearPoint(point, rot, 14)) return 4;
  if (canvasCursorNearPoint(point, tl, 12) || canvasCursorNearPoint(point, br, 12)) return 2;
  if (canvasCursorNearPoint(point, tr, 12) || canvasCursorNearPoint(point, bl, 12)) return 3;
  if (canvasCursorNearPoint(point, l, 12) || canvasCursorNearPoint(point, r, 12)) return 5;
  if (canvasCursorNearPoint(point, t, 12) || canvasCursorNearPoint(point, btm, 12)) return 6;
  if (canvasCursorOnRectFrame(b, point, 6)) return 1;
  return 0;
}

QCursor textRotateCursor() {
  QPixmap pixmap(24, 24);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);
  QPen pen(Qt::black, 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  painter.setPen(pen);
  painter.drawArc(QRectF(4.0, 4.0, 16.0, 16.0), 25 * 16, 280 * 16);
  painter.drawLine(QPointF(17.5, 5.0), QPointF(20.0, 9.0));
  painter.drawLine(QPointF(17.5, 5.0), QPointF(13.5, 5.2));
  painter.setPen(QPen(Qt::white, 0.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.drawArc(QRectF(4.0, 4.0, 16.0, 16.0), 25 * 16, 280 * 16);
  painter.end();
  return QCursor(pixmap, 12, 12);
}

void applyOperationCursorMode(QWidget* widget, int mode) {
  if (widget == nullptr) {
    return;
  }
  if (mode == 4) {
    widget->setCursor(textRotateCursor());
  } else if (mode == 3) {
    widget->setCursor(Qt::SizeBDiagCursor);
  } else if (mode == 2) {
    widget->setCursor(Qt::SizeFDiagCursor);
  } else if (mode == 5) {
    widget->setCursor(Qt::SizeHorCursor);
  } else if (mode == 6) {
    widget->setCursor(Qt::SizeVerCursor);
  } else if (mode == 1) {
    widget->setCursor(Qt::SizeAllCursor);
  }
}
} // namespace

CanvasWidget::CanvasWidget(QWidget* parent)
    : QWidget(parent) {
  stateFor(this);
  connect(this, &QObject::destroyed, this, [this]() {
    g_canvasStates.erase(this);
  });

  setFocusPolicy(Qt::StrongFocus);
  setAttribute(Qt::WA_InputMethodEnabled, true);
  setMouseTracking(true);
  setMinimumSize(400, 300);
  updateZoomStatusLabel(this);
}

void CanvasWidget::setController(app::bridge::AppController* controller) {
  if (m_controller != nullptr) {
    disconnect(m_controller, nullptr, this, nullptr);
  }

  m_controller = controller;
  if (m_controller == nullptr) {
    return;
  }

  connect(m_controller, &app::bridge::AppController::documentChanged, this, &CanvasWidget::refreshFromController);
  connect(m_controller, &app::bridge::AppController::canvasChanged, this, &CanvasWidget::refreshFromController);
  connect(m_controller, &app::bridge::AppController::overlayChanged, this, [this]() {
    update();
  });
  connect(m_controller, &app::bridge::AppController::toolStateChanged, this, [this]() {
    const auto& state = stateFor(this);
    if (state.hasMousePos) {
      updateCursorForState(mapToCanvas(state.lastMousePos));
    } else {
      updateCursorForState(std::nullopt);
    }
    update();
  });
  refreshFromController();
}

void CanvasWidget::zoomIn() {
  auto& state = stateFor(this);
  state.zoom = std::clamp(state.zoom * 1.1, 0.1, 16.0);
  updateZoomStatusLabel(this);
  update();
}

void CanvasWidget::zoomOut() {
  auto& state = stateFor(this);
  state.zoom = std::clamp(state.zoom / 1.1, 0.1, 16.0);
  updateZoomStatusLabel(this);
  update();
}

void CanvasWidget::resetZoom() {
  auto& state = stateFor(this);
  state.zoom = 1.0;
  state.panOffset = QPointF(0.0, 0.0);
  updateZoomStatusLabel(this);
  update();
}

void CanvasWidget::rotateViewLeft() {
  auto& state = stateFor(this);
  state.rotationDegrees = std::fmod(state.rotationDegrees - 15.0 + 360.0, 360.0);
  updateCursorForState(state.hasMousePos ? mapToCanvas(state.lastMousePos) : std::optional<core::Point> {});
  update();
}

void CanvasWidget::rotateViewRight() {
  auto& state = stateFor(this);
  state.rotationDegrees = std::fmod(state.rotationDegrees + 15.0, 360.0);
  updateCursorForState(state.hasMousePos ? mapToCanvas(state.lastMousePos) : std::optional<core::Point> {});
  update();
}

void CanvasWidget::resetViewRotation() {
  auto& state = stateFor(this);
  state.rotationDegrees = 0.0;
  updateCursorForState(state.hasMousePos ? mapToCanvas(state.lastMousePos) : std::optional<core::Point> {});
  update();
}

void CanvasWidget::fitToScreen() {
  if (m_image.isNull()) {
    return;
  }
  auto& state = stateFor(this);
  const double zoomX = static_cast<double>(width()) / static_cast<double>(m_image.width());
  const double zoomY = static_cast<double>(height()) / static_cast<double>(m_image.height());
  state.zoom = std::clamp(std::min(zoomX, zoomY), 0.1, 16.0);
  state.panOffset = QPointF(0.0, 0.0);
  updateZoomStatusLabel(this);
  update();
}

int CanvasWidget::zoomPercent() const {
  return static_cast<int>(std::lround(stateFor(this).zoom * 100.0));
}

void CanvasWidget::setGridVisible(bool visible) {
  if (m_showGrid == visible) {
    return;
  }
  m_showGrid = visible;
  update();
}

void CanvasWidget::setOverlayVisible(bool visible) {
  if (m_showOverlay == visible) {
    return;
  }
  m_showOverlay = visible;
  update();
}

bool CanvasWidget::event(QEvent* event) {
  if (m_textEditActive && event != nullptr && event->type() == QEvent::ShortcutOverride) {
    event->accept();
    return true;
  }
  return QWidget::event(event);
}

void CanvasWidget::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);
  auto& state = stateFor(this);
  QPainter painter(this);
  painter.fillRect(rect(), QColor(24, 27, 32));

  if (m_image.isNull()) {
    return;
  }

  const QRect target = canvasRect();
  painter.save();
  painter.translate(target.center());
  painter.rotate(state.rotationDegrees);
  painter.translate(-target.center());
  drawCheckerboard(painter, target, static_cast<int>(std::lround(std::clamp(state.zoom * 10.0, 8.0, 24.0))));
  painter.drawImage(target, m_image);
  painter.setPen(QPen(QColor(88, 96, 108), 1.0));
  painter.drawRect(target.adjusted(0, 0, -1, -1));

  if (m_controller != nullptr && m_showOverlay) {
    const app::bridge::CanvasOverlayViewModel overlay = m_controller->canvasOverlay();
    const core::ToolKind activeTool = m_controller->currentTool();
    painter.setRenderHint(QPainter::Antialiasing, true);

    for (const core::VectorPath& guide : overlay.guides) {
      if (guide.points.size() < 2) {
        continue;
      }
      const QPointF p1(
          target.x() + (static_cast<double>(guide.points[0].x) + 0.5) * state.zoom,
          target.y() + (static_cast<double>(guide.points[0].y) + 0.5) * state.zoom);
      const QPointF p2(
          target.x() + (static_cast<double>(guide.points[1].x) + 0.5) * state.zoom,
          target.y() + (static_cast<double>(guide.points[1].y) + 0.5) * state.zoom);
      painter.setPen(QPen(QColor(45, 130, 255, 190), 4.0, Qt::SolidLine, Qt::RoundCap));
      painter.drawLine(p1, p2);
      painter.setPen(QPen(QColor(245, 250, 255, 235), 1.2, Qt::DashLine, Qt::RoundCap));
      painter.drawLine(p1, p2);
    }

    for (const auto& textObj : overlay.textObjects) {
      const QPointF p(
          target.x() + (static_cast<double>(textObj.point.x) + 0.5) * state.zoom,
          target.y() + (static_cast<double>(textObj.point.y) + 0.5) * state.zoom);
      QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
      font.setFamilies(QStringList {QStringLiteral("Yu Gothic UI"), QStringLiteral("Meiryo"), QStringLiteral("Noto Sans CJK JP"), font.family()});
      if (!textObj.fontFamily.empty()) {
        font.setFamily(QString::fromUtf8(textObj.fontFamily.data(), static_cast<int>(textObj.fontFamily.size())));
      }
      font.setPointSize(std::max(8, textObj.size));
      font.setBold(textObj.bold);
      font.setItalic(textObj.italic);
      font.setUnderline(textObj.underline);
      font.setStrikeOut(textObj.strikeOut);
      painter.save();
      painter.translate(p);
      painter.rotate(textObj.rotationDeg);
      painter.scale(std::max(0.1F, textObj.scaleX), std::max(0.1F, textObj.scaleY));
      painter.setFont(font);
      painter.setPen(QColor(textObj.color.r, textObj.color.g, textObj.color.b, textObj.color.a));
      painter.drawText(QPointF(0.0, 0.0), QString::fromUtf8(textObj.text.c_str()));
      painter.restore();
    }
    // Text edit session is rendered by the same TextObject overlay. No duplicate editor overlay here.

    
if (overlay.toolOverlay.hasLine) {
      const QPointF p1(
          target.x() + (static_cast<double>(overlay.toolOverlay.lineStart.x) + 0.5) * state.zoom,
          target.y() + (static_cast<double>(overlay.toolOverlay.lineStart.y) + 0.5) * state.zoom);
      const QPointF p2(
          target.x() + (static_cast<double>(overlay.toolOverlay.lineEnd.x) + 0.5) * state.zoom,
          target.y() + (static_cast<double>(overlay.toolOverlay.lineEnd.y) + 0.5) * state.zoom);
      const QColor accent = activeTool == core::ToolKind::MoveLayer ? QColor(72, 195, 255, 230) : QColor(255, 255, 255, 230);
      painter.setPen(QPen(QColor(0, 0, 0, 180), 3.0, Qt::SolidLine, Qt::RoundCap));
      painter.drawLine(p1, p2);
      painter.setPen(QPen(accent, 1.8, Qt::DashLine, Qt::RoundCap));
      painter.drawLine(p1, p2);
      painter.setBrush(QBrush(accent));
      painter.setPen(QPen(QColor(0, 0, 0, 180), 1.0));
      painter.drawEllipse(p2, 4.0, 4.0);
    }

    if (overlay.toolOverlay.hasRect) {
      const QRectF previewRect(
          target.x() + static_cast<double>(overlay.toolOverlay.rect.x) * state.zoom,
          target.y() + static_cast<double>(overlay.toolOverlay.rect.y) * state.zoom,
          std::max(1.0, static_cast<double>(overlay.toolOverlay.rect.width) * state.zoom),
          std::max(1.0, static_cast<double>(overlay.toolOverlay.rect.height) * state.zoom));
      painter.setBrush(Qt::NoBrush);
      painter.setPen(QPen(QColor(88, 188, 255, 220), 1.6, Qt::DashLine));
      painter.drawRect(previewRect);
    }

    const bool hasTextRangeSelection = overlay.textRangeSelectionRect.has_value();

    if (overlay.selectionRect.has_value()) {
      const core::Rect rect = *overlay.selectionRect;
      const QRectF selectionRect(
          target.x() + static_cast<double>(rect.x) * state.zoom,
          target.y() + static_cast<double>(rect.y) * state.zoom,
          std::max(1.0, static_cast<double>(rect.width) * state.zoom),
          std::max(1.0, static_cast<double>(rect.height) * state.zoom));
      drawSelectionHandles(painter, selectionRect, QColor(255, 210, 80, 210), 4.0);
    }

    if (hasTextRangeSelection) { // text-range-highlight-fill-ui-v14
      const core::Rect rect = *overlay.textRangeSelectionRect;
      const QRectF textRangeRect(
          target.x() + static_cast<double>(rect.x) * state.zoom,
          target.y() + static_cast<double>(rect.y) * state.zoom,
          std::max(1.0, static_cast<double>(rect.width) * state.zoom),
          std::max(1.0, static_cast<double>(rect.height) * state.zoom));
      painter.save();
      painter.setRenderHint(QPainter::Antialiasing, true);
      painter.setPen(Qt::NoPen);
      painter.setBrush(QColor(85, 145, 255, 96));
      painter.drawRoundedRect(textRangeRect.adjusted(-1.0, -1.0, 1.0, 1.0), 2.0, 2.0);
      painter.setBrush(Qt::NoBrush);
      painter.setPen(QPen(QColor(255, 255, 255, 84), 1.0));
      painter.drawRoundedRect(textRangeRect.adjusted(-0.5, -0.5, 0.5, 0.5), 2.0, 2.0);
      painter.restore();
    }

    if (overlay.objectSelectionRect.has_value()) {
      const core::Rect rect = *overlay.objectSelectionRect;
      const QRectF selectionRect(
          target.x() + static_cast<double>(rect.x) * state.zoom,
          target.y() + static_cast<double>(rect.y) * state.zoom,
          std::max(1.0, static_cast<double>(rect.width) * state.zoom),
          std::max(1.0, static_cast<double>(rect.height) * state.zoom));
      if (hasTextRangeSelection) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(255, 64, 64, 110), 1.0, Qt::DashLine));
        painter.drawRect(selectionRect);
      } else {
        drawSelectionHandles(painter, selectionRect, QColor(255, 64, 64, 235), 7.0);
      }
    }
    if (!hasTextRangeSelection) {
      for (const auto& primitive : overlay.objectOverlay.primitives) {
      if (primitive.kind == features::object_editing::OverlayPrimitive::Kind::Line) {
        const QPointF p1(
            target.x() + (static_cast<double>(primitive.p1.x) + 0.5) * state.zoom,
            target.y() + (static_cast<double>(primitive.p1.y) + 0.5) * state.zoom);
        const QPointF p2(
            target.x() + (static_cast<double>(primitive.p2.x) + 0.5) * state.zoom,
            target.y() + (static_cast<double>(primitive.p2.y) + 0.5) * state.zoom);
        painter.setPen(QPen(QColor(255, 64, 64, 235), 1.3));
        painter.drawLine(p1, p2);
      } else if (primitive.kind == features::object_editing::OverlayPrimitive::Kind::HandlePoint) {
        const QPointF p(
            target.x() + (static_cast<double>(primitive.p1.x) + 0.5) * state.zoom,
            target.y() + (static_cast<double>(primitive.p1.y) + 0.5) * state.zoom);
        painter.setBrush(QColor(255, 64, 64, 235));
        painter.setPen(QPen(QColor(20, 20, 20, 220), 1.0));
        painter.drawEllipse(p, 4.0, 4.0);
      }
    }
    }

    if (overlay.cloneSamplePoint.has_value()) {
      const core::Point p = *overlay.cloneSamplePoint;
      const QPointF center(
          target.x() + (static_cast<double>(p.x) + 0.5) * state.zoom,
          target.y() + (static_cast<double>(p.y) + 0.5) * state.zoom);
      painter.setBrush(QBrush(QColor(90, 200, 255, 220)));
      painter.setPen(QPen(QColor(0, 0, 0, 220), 1.0));
      painter.drawEllipse(center, 4.0, 4.0);
      painter.drawLine(QPointF(center.x() - 8.0, center.y()), QPointF(center.x() + 8.0, center.y()));
      painter.drawLine(QPointF(center.x(), center.y() - 8.0), QPointF(center.x(), center.y() + 8.0));
    }

  }

  if (m_showGrid && state.zoom >= 8.0) {
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(QColor(255, 255, 255, 26), 1.0));
    for (int x = 0; x <= m_image.width(); ++x) {
      const int sx = target.x() + static_cast<int>(std::lround(static_cast<double>(x) * state.zoom));
      painter.drawLine(sx, target.y(), sx, target.y() + target.height());
    }
    for (int y = 0; y <= m_image.height(); ++y) {
      const int sy = target.y() + static_cast<int>(std::lround(static_cast<double>(y) * state.zoom));
      painter.drawLine(target.x(), sy, target.x() + target.width(), sy);
    }
  }

  if (m_showOverlay && !state.panning && !m_spacePressed && state.hasMousePos && m_controller != nullptr) {
    const auto point = mapToCanvas(state.lastMousePos);
    const core::ToolKind activeTool = m_controller->currentTool();
    const std::string subToolId = m_controller->currentSubToolId();
    const bool showSizeCursor = (activeTool == core::ToolKind::Brush || activeTool == core::ToolKind::Eraser ||
        subToolId == "clone_stamp_basic" || subToolId == "color_mix_blend" || subToolId == "liquify_push" ||
        subToolId == "sketch_pencil");
    if (point.has_value() && showSizeCursor) {
      const double radiusPx = std::max(1.0, (m_controller->toolState().size * state.zoom) / 2.0);
      const QPointF center(
          target.x() + (static_cast<double>(point->x) + 0.5) * state.zoom,
          target.y() + (static_cast<double>(point->y) + 0.5) * state.zoom);
      painter.setRenderHint(QPainter::Antialiasing, true);
      painter.setBrush(Qt::NoBrush);
      painter.setPen(QPen(QColor(0, 0, 0, 220), 2.0));
      painter.drawEllipse(center, radiusPx + 1.0, radiusPx + 1.0);
      painter.setPen(QPen(QColor(255, 255, 255, 230), 1.0));
      painter.drawEllipse(center, radiusPx, radiusPx);
    }
  }
  painter.restore();
}

void CanvasWidget::mouseDoubleClickEvent(QMouseEvent* event) {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->setInputModifiers(
      event->modifiers().testFlag(Qt::ShiftModifier),
      event->modifiers().testFlag(Qt::ControlModifier),
      event->modifiers().testFlag(Qt::AltModifier));

  auto& state = stateFor(this);
  state.lastMousePos = event->position().toPoint();
  state.hasMousePos = true;

  if (event->button() != Qt::LeftButton) {
    QWidget::mouseDoubleClickEvent(event);
    return;
  }

  const auto point = mapToCanvas(event->position().toPoint());
  if (!point.has_value()) {
    QWidget::mouseDoubleClickEvent(event);
    return;
  }

  if (!m_controller->textObjectIdAt(point->x, point->y).has_value()) {
    QWidget::mouseDoubleClickEvent(event);
    return;
  }

  switchToTextTool();
  beginTextEditSession(*point);
  updateCursorForState(point);
  update();
  event->accept();
}

void CanvasWidget::mousePressEvent(QMouseEvent* event) {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->setInputModifiers(
      event->modifiers().testFlag(Qt::ShiftModifier),
      event->modifiers().testFlag(Qt::ControlModifier),
      event->modifiers().testFlag(Qt::AltModifier));

  auto& state = stateFor(this);
  state.lastMousePos = event->position().toPoint();
  state.hasMousePos = true;

  if (event->button() == Qt::RightButton) {
    const auto point = mapToCanvas(event->position().toPoint());
    if (!point.has_value()) {
      return;
    }
    if (m_controller->currentSubToolId() == "text_basic") { // text-basic-right-click-picker-guard
      updateCursorForState(point);
      update();
      event->accept();
      return;
    }
    m_controller->pickColorAt(point->x, point->y);
    updateCursorForState(point);
    update();
    return;
  }

  if (event->button() == Qt::MiddleButton) {
    state.panning = true;
    state.panDragging = false;
    state.temporaryMiddlePan = true;
    state.lastPanPos = event->position().toPoint();
    m_mouseDrawing = false;
    updateCursorForState(std::nullopt);
    update();
    return;
  }

  if (event->button() != Qt::LeftButton) {
    return;
  }

  if (state.temporaryMiddlePan) {
    return;
  }

  const bool handPan = m_spacePressed || m_controller->currentTool() == core::ToolKind::Hand;
  if (handPan) {
    state.panning = true;
    state.panDragging = false;
    state.lastPanPos = event->position().toPoint();
    updateCursorForState(std::nullopt);
    return;
  }

  const auto point = mapToCanvas(event->position().toPoint());
  if (!point.has_value()) {
    return;
  }

  {
    const app::bridge::CanvasOverlayViewModel overlay = m_controller->canvasOverlay();
    const int directOperationMode = operationCursorModeForObjectOverlay(overlay, *point);
    if (directOperationMode != 0) {
      if (m_textEditActive) {
        commitTextEditSession();
      }
      switchToOperationObjectTool();
      m_mouseDrawing = true;
      m_operationCursorLockMode = directOperationMode;
      state.lastStrokeDispatchWidgetPos = event->position().toPoint();
      state.hasLastStrokeDispatchPos = true;
      state.lastStrokeDispatchNs = g_eventTimer.nsecsElapsed();
      m_controller->beginStroke(point->x, point->y);
      updateCursorForState(point);
      event->accept();
      return;
    }
  }

  if (m_controller->currentSubToolId() == "text_basic") {
    if (m_controller->textObjectIdAt(point->x, point->y).has_value()) { // text-basic-drag-range-route
      m_mouseDrawing = true;
      m_operationCursorLockMode = 0;
      state.lastStrokeDispatchWidgetPos = event->position().toPoint();
      state.hasLastStrokeDispatchPos = true;
      state.lastStrokeDispatchNs = g_eventTimer.nsecsElapsed();
      m_controller->beginStroke(point->x, point->y);
      updateCursorForState(point);
      event->accept();
      return;
    }
    beginTextEditSession(*point);
    event->accept();
    return;
  }

  m_mouseDrawing = true;
  m_operationCursorLockMode = 0;
  if (m_controller->currentTool() == core::ToolKind::Brush && m_controller->currentSubToolId() == "operation_object") {
    const app::bridge::CanvasOverlayViewModel overlay = m_controller->canvasOverlay();
    m_operationCursorLockMode = operationCursorModeForObjectOverlay(overlay, *point);
  }
  state.lastStrokeDispatchWidgetPos = event->position().toPoint();
  state.hasLastStrokeDispatchPos = true;
  state.lastStrokeDispatchNs = g_eventTimer.nsecsElapsed();
  m_controller->beginStroke(point->x, point->y);
  updateCursorForState(point);
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event) {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->setInputModifiers(
      event->modifiers().testFlag(Qt::ShiftModifier),
      event->modifiers().testFlag(Qt::ControlModifier),
      event->modifiers().testFlag(Qt::AltModifier));

  auto& state = stateFor(this);
  const QPoint previousMousePos = state.lastMousePos;
  state.lastMousePos = event->position().toPoint();
  state.hasMousePos = true;
  const auto canvasPoint = mapToCanvas(state.lastMousePos);
  updateCursorForState(canvasPoint);

  if (state.panning && (event->buttons() & Qt::LeftButton)) {
    const QPoint current = event->position().toPoint();
    const QPoint delta = current - state.lastPanPos;
    state.panDragging = state.panDragging || !delta.isNull();
    state.panOffset += QPointF(delta.x(), delta.y());
    state.lastPanPos = current;
    updateCursorForState(std::nullopt);
    update();
    return;
  }

  if (state.panning && state.temporaryMiddlePan && (event->buttons() & Qt::MiddleButton)) {
    const QPoint current = event->position().toPoint();
    const QPoint delta = current - state.lastPanPos;
    state.panDragging = state.panDragging || !delta.isNull();
    state.panOffset += QPointF(delta.x(), delta.y());
    state.lastPanPos = current;
    updateCursorForState(std::nullopt);
    update();
    return;
  }

  if (!m_mouseDrawing || !(event->buttons() & Qt::LeftButton)) {
    if (!m_spacePressed) {
      constexpr int kHoverRadiusPx = 48;
      const QRect prevRect(
          previousMousePos.x() - kHoverRadiusPx,
          previousMousePos.y() - kHoverRadiusPx,
          kHoverRadiusPx * 2 + 1,
          kHoverRadiusPx * 2 + 1);
      const QRect nextRect(
          event->position().toPoint().x() - kHoverRadiusPx,
          event->position().toPoint().y() - kHoverRadiusPx,
          kHoverRadiusPx * 2 + 1,
          kHoverRadiusPx * 2 + 1);
      update(prevRect.united(nextRect).adjusted(-2, -2, 2, 2));
    }
    return;
  }

  const auto point = canvasPoint;
  if (!point.has_value()) {
    return;
  }

  const QPoint nowPos = event->position().toPoint();
  const qint64 nowNs = g_eventTimer.nsecsElapsed();
  bool shouldDispatch = true;
  if (m_controller->currentSubToolId() == "operation_object") {
    shouldDispatch = true;
  } else
  if (state.hasLastStrokeDispatchPos) {
    const QPoint delta = nowPos - state.lastStrokeDispatchWidgetPos;
    const bool movedEnough = delta.manhattanLength() >= 2;
    const bool elapsedEnough = (nowNs - state.lastStrokeDispatchNs) >= 3'000'000; // ~333fps cap
    shouldDispatch = movedEnough || elapsedEnough;
  }
  if (!shouldDispatch) {
    return;
  }
  state.lastStrokeDispatchWidgetPos = nowPos;
  state.hasLastStrokeDispatchPos = true;
  state.lastStrokeDispatchNs = nowNs;

  m_controller->continueStroke(point->x, point->y);
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->setInputModifiers(
      event->modifiers().testFlag(Qt::ShiftModifier),
      event->modifiers().testFlag(Qt::ControlModifier),
      event->modifiers().testFlag(Qt::AltModifier));

  auto& state = stateFor(this);
  state.lastMousePos = event->position().toPoint();
  state.hasMousePos = true;

  if (event->button() == Qt::MiddleButton && state.temporaryMiddlePan) {
    state.panning = false;
    state.panDragging = false;
    state.temporaryMiddlePan = false;
    updateCursorForState(mapToCanvas(state.lastMousePos));
    update();
    return;
  }

  if (event->button() != Qt::LeftButton) {
    return;
  }

  if (state.panning) {
    state.panning = false;
    state.panDragging = false;
    state.temporaryMiddlePan = false;
    updateCursorForState(mapToCanvas(state.lastMousePos));
    update();
    return;
  }

  if (m_mouseDrawing) {
    m_controller->endStroke();
  }
  m_mouseDrawing = false;
  m_operationCursorLockMode = 0;
  state.hasLastStrokeDispatchPos = false;
  updateCursorForState(mapToCanvas(state.lastMousePos));
  update();
}

void CanvasWidget::beginTextEditSession(const core::Point& canvasPoint) {
  if (m_controller == nullptr) {
    return;
  }
  if (m_textEditActive) {
    commitTextEditSession();
  }
  if (!m_controller->beginTextSessionAt(canvasPoint.x, canvasPoint.y)) {
    return;
  }
  const auto existingId = m_controller->textObjectIdAt(canvasPoint.x, canvasPoint.y);
  m_textEditObjectId = existingId.value_or(std::string {});
  m_textEditActive = !m_textEditObjectId.empty();
  m_textCaretVisible = true;
  if (m_textEditActive && !m_textCaretTimer.isActive()) {
    m_textCaretTimer.start(530, this);
  }
  setFocus(Qt::MouseFocusReason);
  update();
}

void CanvasWidget::commitTextEditSession() {
  if (!m_textEditActive) {
    return;
  }
  m_controller->handleTextSessionKey(Qt::Key_Return, "");
  m_textEditActive = false;
  m_textEditObjectId.clear();
  m_textCaretVisible = true;
  if (m_textCaretTimer.isActive()) {
    m_textCaretTimer.stop();
  }
  update();
}

void CanvasWidget::cancelTextEditSession() {
  if (!m_textEditActive) {
    return;
  }
  m_controller->handleTextSessionKey(Qt::Key_Escape, "");
  m_textEditActive = false;
  m_textEditObjectId.clear();
  m_textCaretVisible = true;
  if (m_textCaretTimer.isActive()) {
    m_textCaretTimer.stop();
  }
  update();
}

bool CanvasWidget::switchToOperationObjectTool() {
  if (m_controller == nullptr) {
    return false;
  }
  const bool toolOk = m_controller->setCurrentTool(core::ToolKind::MoveLayer);
  const bool subToolOk = m_controller->setCurrentSubTool("operation_object");
  return toolOk && subToolOk;
}

bool CanvasWidget::switchToTextTool() {
  if (m_controller == nullptr) {
    return false;
  }
  const bool toolOk = m_controller->setCurrentTool(core::ToolKind::MoveLayer);
  const bool subToolOk = m_controller->setCurrentSubTool("text_basic");
  return toolOk && subToolOk;
}

void CanvasWidget::wheelEvent(QWheelEvent* event) {
  if (m_controller == nullptr) {
    event->ignore();
    return;
  }
  m_controller->setInputModifiers(
      event->modifiers().testFlag(Qt::ShiftModifier),
      event->modifiers().testFlag(Qt::ControlModifier),
      event->modifiers().testFlag(Qt::AltModifier));

  auto& state = stateFor(this);

  const double steps = event->angleDelta().y() / 120.0;
  if (std::abs(steps) < 0.0001) {
    event->ignore();
    return;
  }

  if (event->modifiers() & Qt::ControlModifier) {
    const double oldZoom = state.zoom;
    const double newZoom = std::clamp(oldZoom * std::pow(1.1, steps), 0.1, 16.0);
    if (std::abs(newZoom - oldZoom) < 0.0001) {
      event->accept();
      return;
    }

    const QPoint mousePos = event->position().toPoint();
    const auto beforeCanvas = mapToCanvas(mousePos);
    state.zoom = newZoom;

    if (beforeCanvas.has_value() && !m_image.isNull()) {
      const QPointF baseTopLeft(
          (width() - static_cast<double>(m_image.width()) * newZoom) / 2.0,
          (height() - static_cast<double>(m_image.height()) * newZoom) / 2.0);
      state.panOffset = QPointF(mousePos.x(), mousePos.y()) - baseTopLeft -
                        QPointF(beforeCanvas->x * newZoom, beforeCanvas->y * newZoom);
    }

    updateZoomStatusLabel(this);
    updateCursorForState(mapToCanvas(event->position().toPoint()));
    update();
    event->accept();
    return;
  }

  if (m_controller->currentToolSupportsSize()) {
    const int delta = steps > 0.0 ? static_cast<int>(std::ceil(steps)) : static_cast<int>(std::floor(steps));
    const int currentSize = m_controller->toolState().size;
    const int nextSize = std::clamp(currentSize + delta, 1, 128);
    if (nextSize != currentSize) {
      m_controller->setBrushSize(nextSize);
      update();
    }
  }
  event->accept();
}

void CanvasWidget::inputMethodEvent(QInputMethodEvent* event) {
  if (m_controller == nullptr || event == nullptr || !m_textEditActive) {
    QWidget::inputMethodEvent(event);
    return;
  }

  const QString commitText = event->commitString();
  if (!commitText.isEmpty()) {
    const QByteArray utf8 = commitText.toUtf8();
    const std::string textUtf8(utf8.constData(), static_cast<std::string::size_type>(utf8.size()));
    if (m_controller->handleTextSessionKey(0, textUtf8)) {
      m_textCaretVisible = true;
      const auto& state = stateFor(this);
      if (state.hasMousePos) {
        updateCursorForState(mapToCanvas(state.lastMousePos));
      } else {
        updateCursorForState(std::nullopt);
      }
      update();
      event->accept();
      return;
    }
  }

  const QString preeditText = event->preeditString();
  const QByteArray preeditUtf8 = preeditText.toUtf8();
  const std::string preeditString(preeditUtf8.constData(), static_cast<std::string::size_type>(preeditUtf8.size()));
  if (m_controller->handleTextSessionPreedit(preeditString)) {
    m_textCaretVisible = true;
    const auto& state = stateFor(this);
    if (state.hasMousePos) {
      updateCursorForState(mapToCanvas(state.lastMousePos));
    } else {
      updateCursorForState(std::nullopt);
    }
    update();
    event->accept();
    return;
  }

  QWidget::inputMethodEvent(event);
}
void CanvasWidget::keyPressEvent(QKeyEvent* event) {
  if (m_controller != nullptr) {
    const Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
    m_controller->setInputModifiers(
        modifiers.testFlag(Qt::ShiftModifier),
        modifiers.testFlag(Qt::ControlModifier),
        modifiers.testFlag(Qt::AltModifier));
  }
  if (m_textEditActive) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
      commitTextEditSession();
      event->accept();
      return;
    }
    if (event->key() == Qt::Key_Escape) {
      cancelTextEditSession();
      event->accept();
      return;
    }
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right ||
        event->key() == Qt::Key_Delete || event->key() == Qt::Key_Home ||
        event->key() == Qt::Key_End) {
      m_controller->handleTextSessionKey(event->key(), "");
      update();
      event->accept();
      return;
    }    if (event->key() == Qt::Key_Backspace) {
      m_controller->handleTextSessionKey(Qt::Key_Backspace, "");
      update();
      event->accept();
      return;
    }
    const QString t = event->text();
    if (!t.isEmpty() && t.at(0).isPrint()) {
      m_controller->handleTextSessionKey(event->key(), t.toUtf8().toStdString());
      update();
      event->accept();
      return;
    }
    event->accept();
    return;
  }
  if (event->key() == Qt::Key_Space) {
    m_spacePressed = true;
    updateCursorForState(stateFor(this).hasMousePos ? mapToCanvas(stateFor(this).lastMousePos) : std::optional<core::Point> {});
    update();
    event->accept();
    return;
  }
  QWidget::keyPressEvent(event);
}

void CanvasWidget::keyReleaseEvent(QKeyEvent* event) {
  if (m_controller != nullptr) {
    const Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
    m_controller->setInputModifiers(
        modifiers.testFlag(Qt::ShiftModifier),
        modifiers.testFlag(Qt::ControlModifier),
        modifiers.testFlag(Qt::AltModifier));
  }
  if (event->isAutoRepeat()) {
    QWidget::keyReleaseEvent(event);
    return;
  }
  if (event->key() == Qt::Key_Space) {
    m_spacePressed = false;
    updateCursorForState(stateFor(this).hasMousePos ? mapToCanvas(stateFor(this).lastMousePos) : std::optional<core::Point> {});
    update();
    event->accept();
    return;
  }
  QWidget::keyReleaseEvent(event);
}

void CanvasWidget::timerEvent(QTimerEvent* event) {
  if (event != nullptr && event->timerId() == m_textCaretTimer.timerId()) {
    if (!m_textEditActive) {
      m_textCaretTimer.stop();
      m_textCaretVisible = true;
      event->accept();
      return;
    }
    m_textCaretVisible = !m_textCaretVisible;
    update();
    event->accept();
    return;
  }
  QWidget::timerEvent(event);
}

void CanvasWidget::refreshFromController() {
  if (m_controller == nullptr) {
    return;
  }
  const core::PixelBuffer& composited = m_controller->compositedBuffer();
  const std::optional<core::Rect> dirtyRect = m_controller->consumeDirtyCompositeRect();
  const bool canPatchRegion =
      dirtyRect.has_value() &&
      !m_image.isNull() &&
      m_image.width() == composited.width() &&
      m_image.height() == composited.height();
  if (!canPatchRegion) {
    m_image = platform::qt::QtImageConverter::toQImage(composited);
  } else {
    const core::Rect dirty = *dirtyRect;
    const int x0 = std::clamp(dirty.x, 0, composited.width());
    const int y0 = std::clamp(dirty.y, 0, composited.height());
    const int x1 = std::clamp(dirty.x + dirty.width, 0, composited.width());
    const int y1 = std::clamp(dirty.y + dirty.height, 0, composited.height());
    for (int y = y0; y < y1; ++y) {
      auto* scanLine = m_image.scanLine(y);
      for (int x = x0; x < x1; ++x) {
        const core::Color color = composited.pixel(x, y);
        const int offset = x * 4;
        scanLine[offset + 0] = color.r;
        scanLine[offset + 1] = color.g;
        scanLine[offset + 2] = color.b;
        scanLine[offset + 3] = color.a;
      }
    }
  }
  updateZoomStatusLabel(this);
  const auto& state = stateFor(this);
  updateCursorForState(state.hasMousePos ? mapToCanvas(state.lastMousePos) : std::optional<core::Point> {});
  if (canPatchRegion) {
    const core::Rect dirty = *dirtyRect;
    const QRect target = canvasRect();
    const int x = static_cast<int>(std::floor(target.x() + static_cast<double>(dirty.x) * state.zoom)) - 2;
    const int y = static_cast<int>(std::floor(target.y() + static_cast<double>(dirty.y) * state.zoom)) - 2;
    const int w = static_cast<int>(std::ceil(static_cast<double>(dirty.width) * state.zoom)) + 4;
    const int h = static_cast<int>(std::ceil(static_cast<double>(dirty.height) * state.zoom)) + 4;
    update(QRect(x, y, std::max(1, w), std::max(1, h)));
  } else {
    update();
  }
}

QRect CanvasWidget::canvasRect() const {
  if (m_image.isNull()) {
    return QRect();
  }

  const auto& state = stateFor(this);
  const int scaledW = std::max(1, static_cast<int>(std::lround(static_cast<double>(m_image.width()) * state.zoom)));
  const int scaledH = std::max(1, static_cast<int>(std::lround(static_cast<double>(m_image.height()) * state.zoom)));
  const int x = static_cast<int>(std::lround((width() - scaledW) / 2.0 + state.panOffset.x()));
  const int y = static_cast<int>(std::lround((height() - scaledH) / 2.0 + state.panOffset.y()));
  return QRect(x, y, scaledW, scaledH);
}

std::optional<core::Point> CanvasWidget::mapToCanvas(const QPoint& widgetPos) const {
  if (m_image.isNull()) {
    return std::nullopt;
  }
  const QRect targetRect = canvasRect();
  const auto& state = stateFor(this);
  QPointF localPos = widgetPos;
  if (std::abs(state.rotationDegrees) > 0.0001) {
    const QPointF center = targetRect.center();
    const double radians = -state.rotationDegrees * 3.14159265358979323846 / 180.0;
    const double dx = static_cast<double>(widgetPos.x()) - center.x();
    const double dy = static_cast<double>(widgetPos.y()) - center.y();
    localPos = QPointF(
        center.x() + dx * std::cos(radians) - dy * std::sin(radians),
        center.y() + dx * std::sin(radians) + dy * std::cos(radians));
  }
  if (!targetRect.contains(localPos.toPoint())) {
    return std::nullopt;
  }

  if (state.zoom <= 0.0) {
    return std::nullopt;
  }

  const int cx = static_cast<int>(std::floor((localPos.x() - targetRect.x()) / state.zoom));
  const int cy = static_cast<int>(std::floor((localPos.y() - targetRect.y()) / state.zoom));
  const int clampedX = std::clamp(cx, 0, m_image.width() - 1);
  const int clampedY = std::clamp(cy, 0, m_image.height() - 1);
  return core::Point {clampedX, clampedY};
}

void CanvasWidget::updateCursorForState(const std::optional<core::Point>& canvasPoint) {
  if (m_operationCursorLockMode != 0) {
    applyOperationCursorMode(this, m_operationCursorLockMode);
    return;
  }

  const auto& state = stateFor(this);
  if (state.panning || m_spacePressed) {
    setCursor(state.panDragging ? Qt::ClosedHandCursor : Qt::OpenHandCursor);
    return;
  }

  if (m_controller == nullptr) {
    setCursor(Qt::ArrowCursor);
    return;
  }

  if (canvasPoint.has_value()) {
    const app::bridge::CanvasOverlayViewModel overlay = m_controller->canvasOverlay();
    const int operationMode = operationCursorModeForObjectOverlay(overlay, *canvasPoint);
    if (operationMode != 0) {
      applyOperationCursorMode(this, operationMode);
      return;
    }
  }

  const std::string subToolId = m_controller->currentSubToolId();
  if (subToolId == "text_basic") {
    setCursor(Qt::IBeamCursor);
    return;
  }
  if (subToolId == "operation_object") {
    setCursor(Qt::ArrowCursor);
    return;
  }

  setCursor(cursorForTool(m_controller->currentTool(), m_mouseDrawing));
}
} // namespace app::canvasview





