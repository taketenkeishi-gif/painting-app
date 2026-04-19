#include "app/panels/SubToolPanel.h"

#include <QAbstractItemView>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

#include "app/bridge/AppController.h"

namespace app::panels {

SubToolPanel::SubToolPanel(QWidget* parent)
    : QWidget(parent),
      m_toolNameLabel(new QLabel("Tool: -", this)),
      m_subToolList(new QListWidget(this)) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(6, 6, 6, 6);
  layout->setSpacing(6);

  m_subToolList->setSelectionMode(QAbstractItemView::SingleSelection);
  m_subToolList->setEditTriggers(QAbstractItemView::NoEditTriggers);

  layout->addWidget(m_toolNameLabel);
  layout->addWidget(m_subToolList);
}

void SubToolPanel::setController(app::bridge::AppController* controller) {
  if (m_controller != nullptr) {
    disconnect(m_controller, nullptr, this, nullptr);
  }

  m_controller = controller;
  if (m_controller == nullptr) {
    return;
  }

  connect(m_controller, &app::bridge::AppController::toolStateChanged, this, &SubToolPanel::refreshFromController);
  refreshFromController();
}

void SubToolPanel::refreshFromController() {
  if (m_controller == nullptr) {
    return;
  }

  m_toolNameLabel->setText(QString("Tool: %1").arg(QString::fromStdString(m_controller->currentToolDisplayName())));
  m_subToolList->clear();
  m_subToolList->addItem(QString::fromStdString(m_controller->currentSubToolDisplayName()));
  m_subToolList->setCurrentRow(0);
}

} // namespace app::panels
