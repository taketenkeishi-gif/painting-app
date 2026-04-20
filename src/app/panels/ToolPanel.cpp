#include "app/panels/ToolPanel.h"

#include <QSignalBlocker>
#include <QToolButton>
#include <QGridLayout>
#include <QLayoutItem>
#include <QStyle>

#include "app/bridge/AppController.h"

namespace app::panels {

ToolPanel::ToolPanel(QWidget* parent)
    : QWidget(parent) {
  setStyleSheet(
      "QToolButton {"
      "  border: 1px solid #3a3a3a;"
      "  background: #2b2d31;"
      "  color: #d8d8d8;"
      "  border-radius: 4px;"
      "  padding: 2px;"
      "}"
      "QToolButton:hover { background: #353942; border-color: #5e6f90; }"
      "QToolButton:checked { background: #2f4f7f; border-color: #7fb3ff; color: #ffffff; }");
  auto* layout = new QGridLayout(this);
  layout->setContentsMargins(6, 6, 6, 6);
  layout->setSpacing(6);
  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(1, 1);
}

void ToolPanel::setController(app::bridge::AppController* controller) {
  if (m_controller != nullptr) {
    disconnect(m_controller, nullptr, this, nullptr);
  }

  m_controller = controller;
  if (m_controller == nullptr) {
    return;
  }

  connect(m_controller, &app::bridge::AppController::toolStateChanged, this, &ToolPanel::refreshFromController);
  rebuildButtons();
  refreshFromController();
}

void ToolPanel::refreshFromController() {
  if (m_controller == nullptr) {
    return;
  }

  const core::ToolKind current = m_controller->currentTool();
  for (const auto& [kind, button] : m_buttons) {
    const QSignalBlocker blocker(button);
    button->setChecked(kind == current);
    button->setToolTip(QString::fromStdString(m_controller->toolDisplayName(kind)));
    button->setEnabled(m_controller->canUseToolOnActiveLayer(kind));
  }
}

void ToolPanel::onToolButtonClicked() {
  if (m_controller == nullptr) {
    return;
  }

  auto* button = qobject_cast<QToolButton*>(sender());
  if (button == nullptr) {
    return;
  }

  const int kindValue = button->property("toolKind").toInt();
  const core::ToolKind kind = static_cast<core::ToolKind>(kindValue);
  m_controller->setCurrentTool(kind);
}

void ToolPanel::rebuildButtons() {
  auto* layout = qobject_cast<QGridLayout*>(this->layout());
  if (layout == nullptr || m_controller == nullptr) {
    return;
  }

  for (const auto& [kind, button] : m_buttons) {
    Q_UNUSED(kind);
    layout->removeWidget(button);
    button->deleteLater();
  }
  m_buttons.clear();

  while (layout->count() > 0) {
    QLayoutItem* item = layout->takeAt(0);
    delete item;
  }

  int index = 0;
  for (const core::ToolKind kind : m_controller->availableTools()) {
    auto* button = new QToolButton(this);
    button->setText(QString::fromStdString(m_controller->toolDisplayName(kind)));
    button->setCheckable(true);
    button->setAutoExclusive(true);
    button->setIconSize(QSize(18, 18));
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setMinimumHeight(56);
    button->setProperty("toolKind", static_cast<int>(kind));

    switch (kind) {
      case core::ToolKind::Brush:
        button->setIcon(style()->standardIcon(QStyle::SP_DriveFDIcon));
        break;
      case core::ToolKind::Eraser:
        button->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
        break;
      case core::ToolKind::Eyedropper:
        button->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
        break;
      case core::ToolKind::Fill:
        button->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
        break;
      case core::ToolKind::Line:
        button->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
        break;
      case core::ToolKind::RectSelection:
        button->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
        break;
      case core::ToolKind::MoveLayer:
        button->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
        break;
      case core::ToolKind::Hand:
        button->setIcon(style()->standardIcon(QStyle::SP_DialogHelpButton));
        break;
      case core::ToolKind::Zoom:
        button->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
        break;
      default:
        break;
    }

    connect(button, &QToolButton::clicked, this, &ToolPanel::onToolButtonClicked);
    layout->addWidget(button, index / 2, index % 2);
    m_buttons[kind] = button;
    ++index;
  }
}

} // namespace app::panels
