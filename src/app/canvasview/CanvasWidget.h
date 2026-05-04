#pragma once

#include <optional>
#include <string>

#include <QImage>
#include <QWidget>
#include <QBasicTimer>

#include "core/common/Point.h"

class QWheelEvent;
class QKeyEvent;
class QEvent;
class QTimerEvent;

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
  void rotateViewLeft();
  void rotateViewRight();
  void resetViewRotation();
  int zoomPercent() const;
  void setGridVisible(bool visible);
  void setOverlayVisible(bool visible);
  bool isGridVisible() const noexcept { return m_showGrid; }
  bool isOverlayVisible() const noexcept { return m_showOverlay; }

protected:
  bool event(QEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void timerEvent(QTimerEvent* event) override;

private slots:
  void refreshFromController();

private:
  QRect canvasRect() const;
  std::optional<core::Point> mapToCanvas(const QPoint& widgetPos) const;
  void updateCursorForState(const std::optional<core::Point>& canvasPoint);
  void beginTextEditSession(const core::Point& canvasPoint);
  void commitTextEditSession();
  void cancelTextEditSession();
  bool switchToOperationObjectTool();
  bool switchToTextTool();

  app::bridge::AppController* m_controller {nullptr};
  QImage m_image;
  bool m_mouseDrawing {false};
  bool m_showGrid {false};
  bool m_showOverlay {true};
  bool m_spacePressed {false};
  bool m_textEditActive {false};
  std::string m_textEditObjectId;
  int m_operationCursorLockMode {0};
  bool m_textCaretVisible {true};
  QBasicTimer m_textCaretTimer;
};

} // namespace app::canvasview


