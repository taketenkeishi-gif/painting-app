#include "app/panels/ToolPanel.h"

#include <utility>
#include <vector>

#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>

#include "app/bridge/AppController.h"

namespace app::panels {

ToolPanel::ToolPanel(QWidget* parent)
    : QWidget(parent) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(6, 6, 6, 6);
  layout->setSpacing(6);

  const std::vector<std::pair<core::ToolKind, QString>> toolItems = {
      {core::ToolKind::Brush, "Brush"},
      {core::ToolKind::Eraser, "Eraser"},
      {core::ToolKind::Eyedropper, "Eye"},
      {core::ToolKind::Fill, "Fill"},
      {core::ToolKind::Line, "Line"},
      {core::ToolKind::RectSelection, "Select"},
      {core::ToolKind::MoveLayer, "Move"},
      {core::ToolKind::Hand, "Hand"},
      {core::ToolKind::Zoom, "Zoom"}};

  for (const auto& [kind, label] : toolItems) {
    auto* button = new QToolButton(this);
    button->setText(label);
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

void ToolPanel::setController(app::bridge::AppController* controller) {
  if (m_controller != nullptr) {
    disconnect(m_controller, nullptr, this, nullptr);
  }

  m_controller = controller;
  if (m_controller == nullptr) {
    return;
  }

  connect(m_controller, &app::bridge::AppController::toolStateChanged, this, &ToolPanel::refreshFromController);
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
    button->setToolTip(QString::fromLatin1(core::toolKindDisplayName(kind)));
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

} // namespace app::panels
