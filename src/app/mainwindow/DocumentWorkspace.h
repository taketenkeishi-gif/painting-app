#pragma once
#include <QWidget>
#include <QVector>
#include <QString>

// Qt Advanced Docking System forward declarations
namespace ads {
class CDockManager;
class CDockWidget;
class CDockAreaWidget;
}

namespace app::canvasview { class CanvasWidget; }

namespace app::mainwindow {

// CSP-style document workspace powered by Qt Advanced Docking System.
//
// Each canvas becomes a CDockWidget: tabs can be reordered by horizontal drag,
// detached into a floating window by downward drag, and re-docked by dropping
// back onto the tab bar — all handled natively by ADS.
//
// Tool/Layer/Property panels are NOT managed here; those remain as Qt standard
// QDockWidget instances in MainWindow.
class DocumentWorkspace : public QWidget {
  Q_OBJECT
public:
  explicit DocumentWorkspace(QWidget* parent = nullptr);
  ~DocumentWorkspace() override;

  void addDocument(app::canvasview::CanvasWidget* canvas,
                   const QString& title,
                   int insertAt = -1);
  void removeDocument(app::canvasview::CanvasWidget* canvas);
  void setDocumentTitle(app::canvasview::CanvasWidget* canvas, const QString& title);

  app::canvasview::CanvasWidget* activeCanvas()        const;
  int                            documentCount()        const { return m_canvases.size(); }
  app::canvasview::CanvasWidget* documentAt(int index)  const;
  void                           setActiveDocument(app::canvasview::CanvasWidget* canvas);

signals:
  void activeDocumentChanged(app::canvasview::CanvasWidget* canvas);
  void closeRequested(app::canvasview::CanvasWidget* canvas);
  void becameEmpty();

private:
  ads::CDockWidget* dockWidgetFor(app::canvasview::CanvasWidget* canvas) const;

  ads::CDockManager*                      m_dockManager {nullptr};
  ads::CDockAreaWidget*                   m_mainArea    {nullptr};
  QVector<app::canvasview::CanvasWidget*> m_canvases;
  QVector<ads::CDockWidget*>              m_dockWidgets;
};

} // namespace app::mainwindow
