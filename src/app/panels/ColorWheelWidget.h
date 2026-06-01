#pragma once

#include <QColor>
#include <QWidget>

namespace app::panels {

class ColorWheelWidget : public QWidget {
  Q_OBJECT

public:
  explicit ColorWheelWidget(QWidget* parent = nullptr);

  QColor color() const noexcept { return m_color; }
  void setColor(const QColor& color);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

signals:
  void colorChanged(const QColor& color);

protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

private:
  QRectF squareRect() const;
  QPointF centerPoint() const;
  double outerRadius() const;
  double innerRadius() const;
  double ringRadius() const;
  double ringThickness() const;
  QColor normalizedHsvColor(const QColor& color) const;

  bool updateHueFromPoint(const QPointF& point);
  bool updateSvFromPoint(const QPointF& point);

  QColor m_color {Qt::black};
  bool m_dragHue {false};
  bool m_dragSv {false};
  // HiDPI キャッシュ
  mutable QImage m_ringCache;
  mutable QSize m_ringCacheSize;
  mutable qreal m_ringCacheDpr {0.0};
};

} // namespace app::panels
