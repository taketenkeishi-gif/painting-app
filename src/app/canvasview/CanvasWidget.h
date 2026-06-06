#pragma once

#include <optional>

#include <QImage>
#include <QTimer>
#include <QWidget>

#include "core/common/FPoint.h"
#include "core/common/Point.h"

class QWheelEvent;
class QKeyEvent;
class QTabletEvent;

namespace app::bridge {
class AppController;
}

namespace app::canvasview {

class CanvasWidget : public QWidget {
  Q_OBJECT

signals:
  void viewTransformChanged();
  void canvasPositionChanged(int x, int y);

public:
  explicit CanvasWidget(QWidget* parent = nullptr);
  void setController(app::bridge::AppController* controller);
  void zoomIn();
  void zoomOut();
  void resetZoom();
  void fitToScreen();
  int zoomPercent() const;
  QRectF visibleCanvasFractionF() const; // normalized 0..1 rect of visible canvas area
  void setGridVisible(bool visible);
  void setOverlayVisible(bool visible);
  void resetRotation();
  double canvasRotationDeg() const noexcept { return m_canvasRotationDeg; }
  void setMirrorView(bool mirror);
  bool isGridVisible() const noexcept { return m_showGrid; }
  bool isOverlayVisible() const noexcept { return m_showOverlay; }

protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void tabletEvent(QTabletEvent* event) override;
  void leaveEvent(QEvent* event) override;

private slots:
  void refreshFromController();

private:
  QRect canvasRect() const;
  std::optional<core::Point> mapToCanvas(const QPoint& widgetPos) const;
  std::optional<core::FPoint> mapToCanvasF(const QPointF& widgetPos) const;
  void updateCursorForState(const std::optional<core::Point>& canvasPoint);

  app::bridge::AppController* m_controller {nullptr};
  QImage m_image;
  QTimer* m_marchingTimer {nullptr};
  int m_marchingOffset {0};
  bool m_mouseDrawing {false};
  bool m_showGrid {false};
  bool m_showOverlay {true};
  bool m_spacePressed {false};
  bool m_tabletActive {false};
  double m_canvasRotationDeg {0.0};
  bool m_rotateKeyHeld {false};
  bool m_ctrlSpaceZoom {false};
  bool m_mirrorView {false};
  QPoint m_ctrlSpaceStartPos;
  double m_ctrlSpaceStartZoom {1.0};
};

} // namespace app::canvasview
