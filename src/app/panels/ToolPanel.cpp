#include "app/panels/ToolPanel.h"

#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>
#include <QLayoutItem>

#include "app/bridge/AppController.h"

namespace app::panels {

ToolPanel::ToolPanel(QWidget* parent)
    : QWidget(parent) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(6, 6, 6, 6);
  layout->setSpacing(6);
  layout->addStretch(1);
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
  auto* layout = qobject_cast<QVBoxLayout*>(this->layout());
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

  for (const core::ToolKind kind : m_controller->availableTools()) {
    auto* button = new QToolButton(this);
    button->setText(QString::fromStdString(m_controller->toolDisplayName(kind)));
    button->setCheckable(true);
    button->setAutoExclusive(true);
    button->setMinimumHeight(34);
    button->setProperty("toolKind", static_cast<int>(kind));
    connect(button, &QToolButton::clicked, this, &ToolPanel::onToolButtonClicked);
    layout->addWidget(button);
    m_buttons[kind] = button;
  }
  layout->addStretch(1);
}

} // namespace app::panels
