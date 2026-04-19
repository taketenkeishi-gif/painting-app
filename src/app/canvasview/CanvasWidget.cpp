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
#include "platform/qt/QtImageConverter.h"

namespace app::canvasview {

namespace {

struct CanvasInteractionState {
  double zoom {1.0};
  QPointF panOffset {0.0, 0.0};
  bool panning {false};
  QPoint lastPanPos {0, 0};
  QPoint lastMousePos {0, 0};
  bool hasMousePos {false};
  bool straightMode {false};
  core::Point straightStart {0, 0};
  std::optional<core::Point> straightEnd;
};

std::unordered_map<const CanvasWidget*, CanvasInteractionState> g_canvasStates;
bool g_spacePressed {false};

class SpaceKeyTracker : public QObject {
public:
  using QObject::QObject;

protected:
  bool eventFilter(QObject* watched, QEvent* event) override {
    Q_UNUSED(watched);
    if (event->type() == QEvent::KeyPress) {
      const auto* keyEvent = static_cast<QKeyEvent*>(event);
      if (!keyEvent->isAutoRepeat() && keyEvent->key() == Qt::Key_Space) {
        g_spacePressed = true;
      }
    } else if (event->type() == QEvent::KeyRelease) {
      const auto* keyEvent = static_cast<QKeyEvent*>(event);
      if (!keyEvent->isAutoRepeat() && keyEvent->key() == Qt::Key_Space) {
        g_spacePressed = false;
      }
    }
    return QObject::eventFilter(watched, event);
  }
};

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
  zoomLabel->setText(QString("Zoom: %1%").arg(percent));
}

void updateCursorForState(CanvasWidget* widget, const std::optional<core::Point>& canvasPoint) {
  if (widget == nullptr) {
    return;
  }
  const auto& state = stateFor(widget);
  if (state.panning) {
    widget->setCursor(Qt::ClosedHandCursor);
    return;
  }
  if (g_spacePressed) {
    widget->setCursor(Qt::OpenHandCursor);
    return;
  }
  if (canvasPoint.has_value()) {
    widget->setCursor(Qt::BlankCursor);
    return;
  }
  widget->unsetCursor();
}

void ensureSpaceTrackerInstalled() {
  static bool installed = false;
  if (installed || qApp == nullptr) {
    return;
  }
  auto* tracker = new SpaceKeyTracker(qApp);
  qApp->installEventFilter(tracker);
  installed = true;
}

} // namespace

CanvasWidget::CanvasWidget(QWidget* parent)
    : QWidget(parent) {
  ensureSpaceTrackerInstalled();
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
  refreshFromController();
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
  painter.fillRect(target, QColor(32, 32, 32));
  painter.drawImage(target, m_image);

  if (state.straightMode && state.straightEnd.has_value()) {
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QPointF p1(target.x() + static_cast<double>(state.straightStart.x) * state.zoom,
                     target.y() + static_cast<double>(state.straightStart.y) * state.zoom);
    const QPointF p2(target.x() + static_cast<double>(state.straightEnd->x) * state.zoom,
                     target.y() + static_cast<double>(state.straightEnd->y) * state.zoom);
    painter.setPen(QPen(QColor(0, 0, 0, 180), 3.0, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(p1, p2);
    painter.setPen(QPen(QColor(255, 255, 255, 230), 1.8, Qt::DashLine, Qt::RoundCap));
    painter.drawLine(p1, p2);
    painter.setBrush(QBrush(QColor(255, 255, 255, 230)));
    painter.setPen(QPen(QColor(0, 0, 0, 180), 1.0));
    painter.drawEllipse(p2, 4.0, 4.0);
  }

  if (!state.panning && !g_spacePressed && state.hasMousePos && m_controller != nullptr) {
    const auto point = mapToCanvas(state.lastMousePos);
    if (point.has_value()) {
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

  auto& state = stateFor(this);
  state.lastMousePos = event->position().toPoint();
  state.hasMousePos = true;

  if (event->button() == Qt::RightButton) {
    const auto point = mapToCanvas(event->position().toPoint());
    if (!point.has_value()) {
      return;
    }
    m_controller->pickColorAt(point->x, point->y);
    updateCursorForState(this, point);
    update();
    return;
  }

  if (event->button() != Qt::LeftButton) {
    return;
  }

  if (g_spacePressed) {
    state.panning = true;
    state.lastPanPos = event->position().toPoint();
    updateCursorForState(this, std::nullopt);
    return;
  }

  const auto point = mapToCanvas(event->position().toPoint());
  if (!point.has_value()) {
    return;
  }

  if (event->modifiers() & Qt::ShiftModifier) {
    state.straightMode = true;
    state.straightStart = *point;
    state.straightEnd = *point;
    m_mouseDrawing = false;
    updateCursorForState(this, point);
    update();
    return;
  }

  state.straightMode = false;
  state.straightEnd.reset();
  m_mouseDrawing = true;
  m_controller->beginStroke(point->x, point->y);
  updateCursorForState(this, point);
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event) {
  if (m_controller == nullptr) {
    return;
  }

  auto& state = stateFor(this);
  state.lastMousePos = event->position().toPoint();
  state.hasMousePos = true;
  const auto canvasPoint = mapToCanvas(state.lastMousePos);
  updateCursorForState(this, canvasPoint);

  if (state.panning && (event->buttons() & Qt::LeftButton)) {
    const QPoint current = event->position().toPoint();
    const QPoint delta = current - state.lastPanPos;
    state.panOffset += QPointF(delta.x(), delta.y());
    state.lastPanPos = current;
    updateCursorForState(this, std::nullopt);
    update();
    return;
  }

  if (state.straightMode && (event->buttons() & Qt::LeftButton)) {
    const auto point = canvasPoint;
    if (point.has_value()) {
      state.straightEnd = point;
      update();
    }
    return;
  }

  if (!m_mouseDrawing || !(event->buttons() & Qt::LeftButton)) {
    if (!g_spacePressed) {
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
  if (event->button() != Qt::LeftButton || m_controller == nullptr) {
    return;
  }

  auto& state = stateFor(this);
  state.lastMousePos = event->position().toPoint();
  state.hasMousePos = true;
  if (state.panning) {
    state.panning = false;
    updateCursorForState(this, mapToCanvas(state.lastMousePos));
    update();
    return;
  }

  if (state.straightMode) {
    if (state.straightEnd.has_value()) {
      m_controller->beginStroke(state.straightStart.x, state.straightStart.y);
      m_controller->continueStroke(state.straightEnd->x, state.straightEnd->y);
      m_controller->endStroke();
    }
    state.straightMode = false;
    state.straightEnd.reset();
    updateCursorForState(this, mapToCanvas(state.lastMousePos));
    update();
    return;
  }

  if (m_mouseDrawing) {
    m_controller->endStroke();
  }
  m_mouseDrawing = false;
  updateCursorForState(this, mapToCanvas(state.lastMousePos));
  update();
}

void CanvasWidget::wheelEvent(QWheelEvent* event) {
  if (m_controller == nullptr) {
    event->ignore();
    return;
  }

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
    updateCursorForState(this, mapToCanvas(event->position().toPoint()));
    update();
    event->accept();
    return;
  }

  const int delta = steps > 0.0 ? static_cast<int>(std::ceil(steps)) : static_cast<int>(std::floor(steps));
  const int currentSize = m_controller->toolState().size;
  const int nextSize = std::clamp(currentSize + delta, 1, 128);
  if (nextSize != currentSize) {
    m_controller->setBrushSize(nextSize);
    update();
  }
  event->accept();
}

void CanvasWidget::refreshFromController() {
  if (m_controller == nullptr) {
    return;
  }
  m_image = platform::qt::QtImageConverter::toQImage(m_controller->compositedBuffer());
  updateZoomStatusLabel(this);
  update();
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

} // namespace app::canvasview
