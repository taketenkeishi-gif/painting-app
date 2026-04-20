#pragma once

#include <optional>

#include <QImage>
#include <QWidget>

#include "core/common/Point.h"

class QWheelEvent;

namespace app::bridge {
class AppController;
}

namespace app::canvasview {

class CanvasWidget : public QWidget {
  Q_OBJECT

public:
  explicit CanvasWidget(QWidget* parent = nullptr);
  void setController(app::bridge::AppController* controller);
  void zoomIn();
  void zoomOut();
  void resetZoom();
  void fitToScreen();
  int zoomPercent() const;

protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

private slots:
  void refreshFromController();

private:
  QRect canvasRect() const;
  std::optional<core::Point> mapToCanvas(const QPoint& widgetPos) const;
  void updateCursorForState(const std::optional<core::Point>& canvasPoint);

  app::bridge::AppController* m_controller {nullptr};
  QImage m_image;
  bool m_mouseDrawing {false};
};

} // namespace app::canvasview
