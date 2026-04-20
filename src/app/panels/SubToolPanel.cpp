#include "app/panels/SubToolPanel.h"

#include <QAbstractItemView>
#include <QLabel>
#include <QListWidget>
#include <QSignalBlocker>
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

  connect(m_subToolList, &QListWidget::currentRowChanged, this, &SubToolPanel::onCurrentSubToolChanged);
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

  const QSignalBlocker blocker(m_subToolList);
  m_refreshing = true;
  m_toolNameLabel->setText(QString("Tool: %1").arg(QString::fromStdString(m_controller->currentToolDisplayName())));
  m_subToolList->clear();
  const auto items = m_controller->subToolViewModels();
  for (const auto& item : items) {
    auto* row = new QListWidgetItem(QString::fromStdString(item.name), m_subToolList);
    row->setData(Qt::UserRole, QString::fromStdString(item.id));
    if (item.active) {
      m_subToolList->setCurrentItem(row);
    }
  }
  if (m_subToolList->count() > 0 && m_subToolList->currentRow() < 0) {
    m_subToolList->setCurrentRow(0);
  }
  m_refreshing = false;
}

void SubToolPanel::onCurrentSubToolChanged(int row) {
  if (m_controller == nullptr || m_refreshing || row < 0) {
    return;
  }
  auto* item = m_subToolList->item(row);
  if (item == nullptr) {
    return;
  }
  const QString id = item->data(Qt::UserRole).toString();
  if (id.isEmpty()) {
    return;
  }
  m_controller->setCurrentSubTool(id.toStdString());
}

} // namespace app::panels
