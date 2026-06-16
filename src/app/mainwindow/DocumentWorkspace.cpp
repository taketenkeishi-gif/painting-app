#include "DocumentWorkspace.h"

#include <QVBoxLayout>

#include <DockManager.h>
#include <DockWidget.h>
#include <DockAreaWidget.h>
#include <DockContainerWidget.h>

#include "app/canvasview/CanvasWidget.h"

using namespace app::mainwindow;
using CanvasWidget = app::canvasview::CanvasWidget;

// ─────────────────────────────────────────────────────────────────────────────

DocumentWorkspace::DocumentWorkspace(QWidget* parent) : QWidget(parent) {
  // ADS global flags — set once before the first CDockManager is constructed.
  // (These are process-wide statics; repeated calls are idempotent.)
  ads::CDockManager::setConfigFlag(ads::CDockManager::AllTabsHaveCloseButton,          true);
  ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasCloseButton,          false);
  ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasUndockButton,         false);
  ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasTabsMenuButton,       false);
  ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting,               true);
  ads::CDockManager::setConfigFlag(ads::CDockManager::FloatingContainerHasWidgetTitle, true);
  ads::CDockManager::setConfigFlag(ads::CDockManager::MiddleMouseButtonClosesTab,      true);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  m_dockManager = new ads::CDockManager(this);
  layout->addWidget(m_dockManager);

  // Propagate ADS focus changes → our activeDocumentChanged signal
  connect(m_dockManager, &ads::CDockManager::focusedDockWidgetChanged,
          this, [this](ads::CDockWidget*, ads::CDockWidget* now) {
    if (!now) return;
    const int i = m_dockWidgets.indexOf(now);
    if (i >= 0)
      emit activeDocumentChanged(m_canvases[i]);
  });
}

DocumentWorkspace::~DocumentWorkspace() = default;

// ── ドキュメント追加 ──────────────────────────────────────────────────────────

void DocumentWorkspace::addDocument(CanvasWidget* canvas,
                                    const QString& title,
                                    int /*insertAt*/) {
  if (m_canvases.contains(canvas)) return;

  auto* dw = new ads::CDockWidget(title);
  // ForceNoScrollArea: キャンバスは自前のスクロールを持つためスクロールエリア不要
  dw->setWidget(canvas, ads::CDockWidget::ForceNoScrollArea);
  // CustomCloseHandling: ADS の自動クローズを抑止し、closeRequested シグナルのみ発火
  dw->setFeature(ads::CDockWidget::CustomCloseHandling,   true);
  dw->setFeature(ads::CDockWidget::DockWidgetDeleteOnClose, false);

  // X ボタン → MainWindow が unsaved-changes チェックを行う
  connect(dw, &ads::CDockWidget::closeRequested, this, [this, canvas]() {
    emit closeRequested(canvas);
  });

  if (!m_mainArea) {
    m_mainArea = m_dockManager->addDockWidget(ads::CenterDockWidgetArea, dw);
  } else {
    m_dockManager->addDockWidget(ads::CenterDockWidgetArea, dw, m_mainArea);
  }

  m_canvases.append(canvas);
  m_dockWidgets.append(dw);

  dw->raise(); // 新規ドキュメントをアクティブに
}

// ── ドキュメント削除 ──────────────────────────────────────────────────────────

void DocumentWorkspace::removeDocument(CanvasWidget* canvas) {
  const int idx = m_canvases.indexOf(canvas);
  if (idx < 0) return;

  auto* dw = m_dockWidgets[idx];
  m_canvases.removeAt(idx);
  m_dockWidgets.removeAt(idx);

  // canvas を再ペアレントしてから DockWidget を破棄（canvas が巻き込まれないよう）
  canvas->setParent(this);
  canvas->hide();

  m_dockManager->removeDockWidget(dw);
  dw->deleteLater();

  if (m_canvases.isEmpty()) {
    m_mainArea = nullptr;
    emit becameEmpty();
  }
}

// ── その他ドキュメント操作 ────────────────────────────────────────────────────

void DocumentWorkspace::setDocumentTitle(CanvasWidget* canvas, const QString& title) {
  if (auto* dw = dockWidgetFor(canvas))
    dw->setWindowTitle(title);
}

CanvasWidget* DocumentWorkspace::activeCanvas() const {
  auto* focused = m_dockManager->focusedDockWidget();
  if (focused) {
    const int i = m_dockWidgets.indexOf(focused);
    if (i >= 0) return m_canvases[i];
  }
  // フォールバック: リストの先頭
  return m_canvases.isEmpty() ? nullptr : m_canvases.first();
}

CanvasWidget* DocumentWorkspace::documentAt(int i) const {
  return (i >= 0 && i < m_canvases.size()) ? m_canvases[i] : nullptr;
}

void DocumentWorkspace::setActiveDocument(CanvasWidget* canvas) {
  if (auto* dw = dockWidgetFor(canvas))
    dw->raise();
}

// ── private helper ────────────────────────────────────────────────────────────

ads::CDockWidget* DocumentWorkspace::dockWidgetFor(CanvasWidget* canvas) const {
  const int i = m_canvases.indexOf(canvas);
  return (i >= 0) ? m_dockWidgets[i] : nullptr;
}
