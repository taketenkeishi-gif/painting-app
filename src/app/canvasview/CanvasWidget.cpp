#include "app/canvasview/CanvasWidget.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include <QApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include "app/bridge/AppController.h"
#include "core/tools/ToolType.h"
#include "platform/qt/QtImageConverter.h"

namespace app::canvasview {

namespace {

struct CanvasInteractionState {
  double zoom {1.0};
  QPointF panOffset {0.0, 0.0};
  bool panning {false};
  bool panDragging {false};
  bool temporaryMiddlePan {false};
  QPoint lastPanPos {0, 0};
  QPoint lastMousePos {0, 0};
  bool hasMousePos {false};
};

std::unordered_map<const CanvasWidget*, CanvasInteractionState> g_canvasStates;

CanvasInteractionState& stateFor(const CanvasWidget* widget) {
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
  const QColor a(58, 62, 70);
  const QColor b(44, 48, 56);
  for (int y = rect.top(); y <= rect.bottom(); y += cell) {
    for (int x = rect.left(); x <= rect.right(); x += cell) {
      const bool useA = ((x / cell) + (y / cell)) % 2 == 0;
      const QRect tile(x, y, std::min(cell, rect.right() - x + 1), std::min(cell, rect.bottom() - y + 1));
      painter.fillRect(tile, useA ? a : b);
    }
  }
}

Qt::CursorShape cursorForTool(core::ToolKind tool, bool dragging) {
  switch (tool) {
    case core::ToolKind::Brush:
    case core::ToolKind::Eraser:
      return Qt::BlankCursor;
    case core::ToolKind::Eyedropper:
      return Qt::CrossCursor;
    case core::ToolKind::Fill:
      return Qt::PointingHandCursor;
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

QString toolNameJa(core::ToolKind tool) {
  switch (tool) {
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

} // namespace

CanvasWidget::CanvasWidget(QWidget* parent)
    : QWidget(parent) {
  stateFor(this);
  connect(this, &QObject::destroyed, this, [this]() {
    g_canvasStates.erase(this);
  });

  setFocusPolicy(Qt::StrongFocus);
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

void CanvasWidget::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);
  auto& state = stateFor(this);
  QPainter painter(this);
  painter.fillRect(rect(), QColor(48, 48, 48));

  if (m_image.isNull()) {
    return;
  }

  const QRect target = canvasRect();
  drawCheckerboard(painter, target, static_cast<int>(std::lround(std::clamp(state.zoom * 10.0, 8.0, 24.0))));
  painter.drawImage(target, m_image);
  painter.setPen(QPen(QColor(88, 96, 108), 1.0));
  painter.drawRect(target.adjusted(0, 0, -1, -1));

  if (m_controller != nullptr && m_showOverlay) {
    const app::bridge::CanvasOverlayViewModel overlay = m_controller->canvasOverlay();
    const core::ToolKind activeTool = m_controller->currentTool();
    painter.setRenderHint(QPainter::Antialiasing, true);

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

    if (overlay.selectionRect.has_value()) {
      const core::Rect rect = *overlay.selectionRect;
      const QRectF selectionRect(
          target.x() + static_cast<double>(rect.x) * state.zoom,
          target.y() + static_cast<double>(rect.y) * state.zoom,
          std::max(1.0, static_cast<double>(rect.width) * state.zoom),
          std::max(1.0, static_cast<double>(rect.height) * state.zoom));
      painter.setBrush(Qt::NoBrush);
      painter.setPen(QPen(QColor(255, 210, 80, 230), 1.4, Qt::SolidLine));
      painter.drawRect(selectionRect);
      painter.setBrush(QColor(255, 210, 80, 210));
      painter.setPen(Qt::NoPen);
      constexpr double handle = 4.0;
      painter.drawRect(QRectF(selectionRect.topLeft().x() - handle / 2.0, selectionRect.topLeft().y() - handle / 2.0, handle, handle));
      painter.drawRect(QRectF(selectionRect.topRight().x() - handle / 2.0, selectionRect.topRight().y() - handle / 2.0, handle, handle));
      painter.drawRect(QRectF(selectionRect.bottomLeft().x() - handle / 2.0, selectionRect.bottomLeft().y() - handle / 2.0, handle, handle));
      painter.drawRect(QRectF(selectionRect.bottomRight().x() - handle / 2.0, selectionRect.bottomRight().y() - handle / 2.0, handle, handle));
    }

    const QString activeToolText = toolNameJa(m_controller->currentTool());
    const QRect badgeRect(target.x() + 10, target.y() + 10, 180, 24);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 145));
    painter.drawRoundedRect(badgeRect, 4.0, 4.0);
    painter.setPen(QColor(255, 255, 255, 235));
    painter.drawText(badgeRect.adjusted(8, 0, -6, 0), Qt::AlignVCenter | Qt::AlignLeft, activeToolText);
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
    if (point.has_value() &&
        (activeTool == core::ToolKind::Brush || activeTool == core::ToolKind::Eraser)) {
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

  m_mouseDrawing = true;
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
      update();
    }
    return;
  }

  const auto point = canvasPoint;
  if (!point.has_value()) {
    return;
  }

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
  updateCursorForState(mapToCanvas(state.lastMousePos));
  update();
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

void CanvasWidget::keyPressEvent(QKeyEvent* event) {
  if (event->isAutoRepeat()) {
    QWidget::keyPressEvent(event);
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
  if (!targetRect.contains(widgetPos)) {
    return std::nullopt;
  }

  const auto& state = stateFor(this);
  if (state.zoom <= 0.0) {
    return std::nullopt;
  }

  const int cx = static_cast<int>(std::floor((widgetPos.x() - targetRect.x()) / state.zoom));
  const int cy = static_cast<int>(std::floor((widgetPos.y() - targetRect.y()) / state.zoom));
  const int clampedX = std::clamp(cx, 0, m_image.width() - 1);
  const int clampedY = std::clamp(cy, 0, m_image.height() - 1);
  return core::Point {clampedX, clampedY};
}

void CanvasWidget::updateCursorForState(const std::optional<core::Point>& canvasPoint) {
  const auto& state = stateFor(this);
  if (state.panning) {
    setCursor(state.panDragging ? Qt::ClosedHandCursor : Qt::OpenHandCursor);
    return;
  }
  if (m_spacePressed) {
    setCursor(Qt::OpenHandCursor);
    return;
  }
  if (!canvasPoint.has_value() || m_controller == nullptr) {
    unsetCursor();
    return;
  }

  const bool dragging = m_mouseDrawing || (state.panning && (QApplication::mouseButtons() & Qt::LeftButton));
  setCursor(cursorForTool(m_controller->currentTool(), dragging));
}

} // namespace app::canvasview

