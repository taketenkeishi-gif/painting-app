#include "app/panels/ColorWheelWidget.h"

#include <algorithm>
#include <cmath>

#include <QConicalGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPen>

namespace app::panels {

namespace {

double hueFromPoint(const QPointF& center, const QPointF& point) {
  const double angle = std::atan2(point.y() - center.y(), point.x() - center.x());
  const double degrees = angle * 180.0 / 3.14159265358979323846;
  const double normalized = std::fmod(360.0 - degrees + 90.0 + 360.0, 360.0);
  return normalized;
}

} // namespace

ColorWheelWidget::ColorWheelWidget(QWidget* parent)
    : QWidget(parent) {
  setMinimumSize(160, 160);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ColorWheelWidget::setColor(const QColor& color) {
  QColor next = color;
  if (!next.isValid()) {
    return;
  }
  next = next.toHsv();
  if (!m_color.isValid() || m_color.toHsv() != next) {
    m_color = next;
    update();
  }
}

QPointF ColorWheelWidget::centerPoint() const {
  return QPointF(width() / 2.0, height() / 2.0);
}

double ColorWheelWidget::outerRadius() const {
  return std::max(10.0, std::min(width(), height()) * 0.5 - 6.0);
}

double ColorWheelWidget::innerRadius() const {
  return std::max(4.0, outerRadius() - std::max(14.0, outerRadius() * 0.2));
}

QRectF ColorWheelWidget::squareRect() const {
  const double size = innerRadius() * std::sqrt(2.0);
  const QPointF c = centerPoint();
  return QRectF(c.x() - size / 2.0, c.y() - size / 2.0, size, size);
}

void ColorWheelWidget::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.fillRect(rect(), QColor(30, 34, 40));

  const QPointF c = centerPoint();
  const double outer = outerRadius();
  const double inner = innerRadius();

  QConicalGradient ringGradient(c, 0.0);
  ringGradient.setColorAt(0.0, QColor::fromHsv(0, 255, 255));
  ringGradient.setColorAt(1.0 / 6.0, QColor::fromHsv(60, 255, 255));
  ringGradient.setColorAt(2.0 / 6.0, QColor::fromHsv(120, 255, 255));
  ringGradient.setColorAt(3.0 / 6.0, QColor::fromHsv(180, 255, 255));
  ringGradient.setColorAt(4.0 / 6.0, QColor::fromHsv(240, 255, 255));
  ringGradient.setColorAt(5.0 / 6.0, QColor::fromHsv(300, 255, 255));
  ringGradient.setColorAt(1.0, QColor::fromHsv(360, 255, 255));

  painter.setPen(Qt::NoPen);
  painter.setBrush(ringGradient);
  painter.drawEllipse(c, outer, outer);

  painter.setCompositionMode(QPainter::CompositionMode_Source);
  painter.setBrush(QColor(30, 34, 40));
  painter.drawEllipse(c, inner, inner);
  painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

  const QRectF svRect = squareRect();
  const int hue = std::clamp(m_color.hsvHue() < 0 ? 0 : m_color.hsvHue(), 0, 359);
  QColor pureHue = QColor::fromHsv(hue, 255, 255);

  QLinearGradient satGradient(svRect.topLeft(), svRect.topRight());
  satGradient.setColorAt(0.0, QColor(255, 255, 255));
  satGradient.setColorAt(1.0, pureHue);
  painter.setBrush(satGradient);
  painter.setPen(QPen(QColor(70, 80, 96), 1.0));
  painter.drawRect(svRect);

  QLinearGradient valGradient(svRect.topLeft(), svRect.bottomLeft());
  valGradient.setColorAt(0.0, QColor(0, 0, 0, 0));
  valGradient.setColorAt(1.0, QColor(0, 0, 0, 255));
  painter.setBrush(valGradient);
  painter.setPen(Qt::NoPen);
  painter.drawRect(svRect);

  const int saturation = std::clamp(m_color.hsvSaturation(), 0, 255);
  const int value = std::clamp(m_color.value(), 0, 255);
  const QPointF svHandle(
      svRect.left() + (static_cast<double>(saturation) / 255.0) * svRect.width(),
      svRect.top() + (1.0 - static_cast<double>(value) / 255.0) * svRect.height());
  painter.setPen(QPen(QColor(0, 0, 0, 230), 2.0));
  painter.setBrush(Qt::NoBrush);
  painter.drawEllipse(svHandle, 4.5, 4.5);
  painter.setPen(QPen(QColor(255, 255, 255, 240), 1.0));
  painter.drawEllipse(svHandle, 3.5, 3.5);

  const double hueRadians = (90.0 - static_cast<double>(hue)) * 3.14159265358979323846 / 180.0;
  const double ringRadius = (outer + inner) * 0.5;
  const QPointF hueHandle(
      c.x() + std::cos(hueRadians) * ringRadius,
      c.y() - std::sin(hueRadians) * ringRadius);
  painter.setPen(QPen(QColor(15, 15, 15, 230), 2.0));
  painter.drawEllipse(hueHandle, 4.5, 4.5);
  painter.setPen(QPen(QColor(245, 245, 245, 240), 1.0));
  painter.drawEllipse(hueHandle, 3.5, 3.5);
}

bool ColorWheelWidget::updateHueFromPoint(const QPointF& point) {
  QColor next = m_color.toHsv();
  int hue = static_cast<int>(std::lround(hueFromPoint(centerPoint(), point)));
  hue = (hue + 360) % 360;
  if (next.hsvHue() == hue) {
    return false;
  }
  next.setHsv(hue, next.hsvSaturation(), next.value(), next.alpha());
  m_color = next;
  emit colorChanged(m_color);
  update();
  return true;
}

bool ColorWheelWidget::updateSvFromPoint(const QPointF& point) {
  const QRectF rect = squareRect();
  const double clampedX = std::clamp(point.x(), rect.left(), rect.right());
  const double clampedY = std::clamp(point.y(), rect.top(), rect.bottom());
  const int saturation = static_cast<int>(std::lround(((clampedX - rect.left()) / rect.width()) * 255.0));
  const int value = static_cast<int>(std::lround((1.0 - (clampedY - rect.top()) / rect.height()) * 255.0));

  QColor next = m_color.toHsv();
  if (next.hsvSaturation() == saturation && next.value() == value) {
    return false;
  }
  next.setHsv(next.hsvHue(), std::clamp(saturation, 0, 255), std::clamp(value, 0, 255), next.alpha());
  m_color = next;
  emit colorChanged(m_color);
  update();
  return true;
}

void ColorWheelWidget::mousePressEvent(QMouseEvent* event) {
  const QPointF p = event->position();
  const QPointF c = centerPoint();
  const double dx = p.x() - c.x();
  const double dy = p.y() - c.y();
  const double dist = std::sqrt(dx * dx + dy * dy);

  if (dist >= innerRadius() && dist <= outerRadius()) {
    m_dragHue = true;
    updateHueFromPoint(p);
    return;
  }

  if (squareRect().contains(p)) {
    m_dragSv = true;
    updateSvFromPoint(p);
    return;
  }
}

void ColorWheelWidget::mouseMoveEvent(QMouseEvent* event) {
  if (m_dragHue) {
    updateHueFromPoint(event->position());
  } else if (m_dragSv) {
    updateSvFromPoint(event->position());
  }
}

void ColorWheelWidget::mouseReleaseEvent(QMouseEvent* event) {
  Q_UNUSED(event);
  m_dragHue = false;
  m_dragSv = false;
}

} // namespace app::panels
