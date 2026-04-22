#include "app/panels/ColorWheelWidget.h"

#include <algorithm>
#include <cmath>

#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>

namespace app::panels {

namespace {

constexpr double kPi = 3.14159265358979323846;

double hueFromPoint(const QPointF& center, const QPointF& point) {
  const double dx = point.x() - center.x();
  const double dy = point.y() - center.y();
  double angle = std::atan2(dx, -dy) * 180.0 / kPi;
  if (angle < 0.0) {
    angle += 360.0;
  }
  return angle;
}

QPointF pointFromHue(const QPointF& center, double radius, int hue) {
  const double radians = (static_cast<double>(hue) - 90.0) * kPi / 180.0;
  return QPointF(
      center.x() + std::cos(radians) * radius,
      center.y() + std::sin(radians) * radius);
}

} // namespace

ColorWheelWidget::ColorWheelWidget(QWidget* parent)
    : QWidget(parent) {
  setMinimumSize(160, 160);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QColor ColorWheelWidget::normalizedHsvColor(const QColor& color) const {
  QColor next = color;
  if (!next.isValid()) {
    return next;
  }
  next = next.toHsv();
  if (next.hsvHue() < 0) {
    const int fallbackHue = (m_color.isValid() && m_color.hsvHue() >= 0) ? m_color.hsvHue() : 0;
    next.setHsv(fallbackHue, 0, next.value(), next.alpha());
  }
  return next;
}

void ColorWheelWidget::setColor(const QColor& color) {
  const QColor next = normalizedHsvColor(color);
  if (!next.isValid()) {
    return;
  }
  if (!m_color.isValid() || m_color.toHsv() != next) {
    m_color = next;
    update();
  }
}

QPointF ColorWheelWidget::centerPoint() const {
  return QPointF(width() / 2.0, height() / 2.0);
}

double ColorWheelWidget::outerRadius() const {
  return std::max(10.0, std::min(width(), height()) * 0.5 - 3.0);
}

double ColorWheelWidget::innerRadius() const {
  return std::max(4.0, outerRadius() - ringThickness());
}

double ColorWheelWidget::ringRadius() const {
  return (outerRadius() + innerRadius()) * 0.5;
}

double ColorWheelWidget::ringThickness() const {
  return std::clamp(outerRadius() * 0.105, 6.0, 11.0);
}

QRectF ColorWheelWidget::squareRect() const {
  const double gapFromRing = 2.5;
  const double safeRadius = std::max(8.0, innerRadius() - gapFromRing);
  const double size = safeRadius * std::sqrt(2.0);
  const QPointF c = centerPoint();
  return QRectF(c.x() - size / 2.0, c.y() - size / 2.0, size, size);
}

void ColorWheelWidget::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.fillRect(rect(), palette().window());

  const QPointF c = centerPoint();
  const double outer = outerRadius();
  const double inner = innerRadius();
  const double ringMid = ringRadius();

  QImage ringImage(size(), QImage::Format_ARGB32_Premultiplied);
  ringImage.fill(Qt::transparent);
  const int minX = std::max(0, static_cast<int>(std::floor(c.x() - outer - 1.0)));
  const int maxX = std::min(width() - 1, static_cast<int>(std::ceil(c.x() + outer + 1.0)));
  const int minY = std::max(0, static_cast<int>(std::floor(c.y() - outer - 1.0)));
  const int maxY = std::min(height() - 1, static_cast<int>(std::ceil(c.y() + outer + 1.0)));
  const double edgeSoftness = 0.85;

  for (int y = minY; y <= maxY; ++y) {
    QRgb* scan = reinterpret_cast<QRgb*>(ringImage.scanLine(y));
    for (int x = minX; x <= maxX; ++x) {
      const double dx = static_cast<double>(x) + 0.5 - c.x();
      const double dy = static_cast<double>(y) + 0.5 - c.y();
      const double dist = std::sqrt(dx * dx + dy * dy);
      if (dist < inner - edgeSoftness || dist > outer + edgeSoftness) {
        continue;
      }
      const int hue =
          static_cast<int>(std::lround(hueFromPoint(c, QPointF(static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5)))) %
          360;
      QColor ringColor = QColor::fromHsv(hue, 255, 255);

      double alphaScale = 1.0;
      if (dist < inner) {
        alphaScale = std::clamp((dist - (inner - edgeSoftness)) / edgeSoftness, 0.0, 1.0);
      } else if (dist > outer) {
        alphaScale = std::clamp(((outer + edgeSoftness) - dist) / edgeSoftness, 0.0, 1.0);
      }
      const int alpha = static_cast<int>(std::lround(alphaScale * 255.0));
      scan[x] = qRgba(ringColor.red(), ringColor.green(), ringColor.blue(), alpha);
    }
  }
  painter.drawImage(0, 0, ringImage);

  painter.setPen(QPen(QColor(14, 17, 22, 235), 1.0));
  painter.setBrush(Qt::NoBrush);
  painter.drawEllipse(c, outer + 0.5, outer + 0.5);

  painter.setPen(QPen(QColor(24, 28, 34, 220), 1.0));
  painter.setBrush(Qt::NoBrush);
  painter.drawEllipse(c, inner + 0.5, inner + 0.5);

  const QRectF svRect = squareRect();
  const int hue = std::clamp(m_color.hsvHue() < 0 ? 0 : m_color.hsvHue(), 0, 359);
  QColor pureHue = QColor::fromHsv(hue, 255, 255);

  QLinearGradient satGradient(svRect.topLeft(), svRect.topRight());
  satGradient.setColorAt(0.0, QColor(255, 255, 255));
  satGradient.setColorAt(1.0, pureHue);
  painter.setBrush(satGradient);
  painter.setPen(QPen(QColor(62, 72, 88), 1.0));
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
  painter.setPen(QPen(QColor(10, 10, 10, 220), 1.3));
  painter.setBrush(Qt::NoBrush);
  painter.drawEllipse(svHandle, 2.9, 2.9);
  painter.setPen(QPen(QColor(245, 245, 245, 230), 1.0));
  painter.drawEllipse(svHandle, 2.1, 2.1);

  const QPointF hueHandle = pointFromHue(c, ringMid, hue);
  painter.setPen(QPen(QColor(10, 10, 10, 220), 1.2));
  painter.setBrush(QColor(240, 240, 240, 220));
  painter.drawEllipse(hueHandle, 2.6, 2.6);
}

bool ColorWheelWidget::updateHueFromPoint(const QPointF& point) {
  QColor next = m_color.toHsv();
  int hue = static_cast<int>(std::lround(hueFromPoint(centerPoint(), point)));
  hue = (hue + 360) % 360;
  if (next.hsvHue() == hue) {
    return false;
  }
  next.setHsv(hue, next.hsvSaturation(), next.value(), next.alpha());
  m_color = normalizedHsvColor(next);
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
  m_color = normalizedHsvColor(next);
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
