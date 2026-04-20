#include "app/panels/SubToolPanel.h"

#include <QAbstractItemView>
#include <QColor>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>

#include "app/bridge/AppController.h"

namespace app::panels {

namespace {
constexpr int kSubToolIdRole = Qt::UserRole;
constexpr int kSubToolEnabledRole = Qt::UserRole + 1;
} // namespace

SubToolPanel::SubToolPanel(QWidget* parent)
    : QWidget(parent),
      m_toolNameLabel(new QLabel("Tool: -", this)),
      m_summaryLabel(new QLabel("Sub Tool: -", this)),
      m_searchEdit(new QLineEdit(this)),
      m_duplicateButton(new QPushButton("Duplicate", this)),
      m_renameButton(new QToolButton(this)),
      m_deleteButton(new QToolButton(this)),
      m_resetButton(new QToolButton(this)),
      m_subToolList(new QListWidget(this)) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(6, 6, 6, 6);
  layout->setSpacing(6);

  m_searchEdit->setPlaceholderText("Search sub tool...");
  m_duplicateButton->setToolTip("Duplicate current preset.");
  m_renameButton->setText("Rename");
  m_deleteButton->setText("Delete");
  m_resetButton->setText("Reset");
  m_renameButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
  m_deleteButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
  m_resetButton->setToolButtonStyle(Qt::ToolButtonTextOnly);

  m_subToolList->setSelectionMode(QAbstractItemView::SingleSelection);
  m_subToolList->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_subToolList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_subToolList->setSpacing(2);
  m_subToolList->setStyleSheet(
      "QListWidget::item { padding: 4px 6px; border-bottom: 1px solid #30343d; }"
      "QListWidget::item:selected { background: #345985; color: #ffffff; }");

  auto* searchRow = new QHBoxLayout();
  searchRow->setContentsMargins(0, 0, 0, 0);
  searchRow->setSpacing(6);
  searchRow->addWidget(m_searchEdit, 1);
  searchRow->addWidget(m_duplicateButton);

  layout->addWidget(m_toolNameLabel);
  layout->addWidget(m_summaryLabel);
  layout->addLayout(searchRow);
  layout->addWidget(m_subToolList);

  auto* manageRow = new QHBoxLayout();
  manageRow->setContentsMargins(0, 0, 0, 0);
  manageRow->setSpacing(6);
  manageRow->addWidget(m_renameButton);
  manageRow->addWidget(m_deleteButton);
  manageRow->addWidget(m_resetButton);
  layout->addLayout(manageRow);

  connect(m_subToolList, &QListWidget::currentRowChanged, this, &SubToolPanel::onCurrentSubToolChanged);
  connect(m_searchEdit, &QLineEdit::textChanged, this, &SubToolPanel::onFilterTextChanged);
  connect(m_duplicateButton, &QPushButton::clicked, this, &SubToolPanel::onDuplicateClicked);
  connect(m_renameButton, &QToolButton::clicked, this, &SubToolPanel::onRenameClicked);
  connect(m_deleteButton, &QToolButton::clicked, this, &SubToolPanel::onDeleteClicked);
  connect(m_resetButton, &QToolButton::clicked, this, &SubToolPanel::onResetClicked);
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
  m_summaryLabel->setText(QString("Sub Tool: %1").arg(QString::fromStdString(m_controller->currentSubToolDisplayName())));
  m_subToolList->clear();
  const auto items = m_controller->subToolViewModels();
  const QString query = m_searchEdit->text().trimmed();
  bool hasEnabledRow = false;
  for (const auto& item : items) {
    const QString displayName = QString::fromStdString(item.name);
    if (!query.isEmpty() && !displayName.contains(query, Qt::CaseInsensitive)) {
      continue;
    }
    auto* row = new QListWidgetItem(QString::fromStdString(item.name), m_subToolList);
    row->setData(kSubToolIdRole, QString::fromStdString(item.id));
    row->setData(kSubToolEnabledRole, item.enabled);
    row->setFlags(item.enabled ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable) : Qt::NoItemFlags);
    row->setForeground(item.enabled ? palette().windowText().color() : QColor(130, 136, 148));
    row->setToolTip(item.hint.empty() ? displayName : QString::fromStdString(item.hint));
    row->setSizeHint(QSize(row->sizeHint().width(), 26));
    hasEnabledRow = hasEnabledRow || item.enabled;
    if (item.active) {
      m_subToolList->setCurrentItem(row);
    }
  }
  if (m_subToolList->count() > 0 && m_subToolList->currentRow() < 0) {
    for (int i = 0; i < m_subToolList->count(); ++i) {
      QListWidgetItem* row = m_subToolList->item(i);
      if (row != nullptr && row->data(kSubToolEnabledRole).toBool()) {
        m_subToolList->setCurrentRow(i);
        break;
      }
    }
  }
  const bool hasRows = m_subToolList->count() > 0;
  m_duplicateButton->setEnabled(hasRows);
  m_renameButton->setEnabled(hasRows && hasEnabledRow);
  m_deleteButton->setEnabled(hasRows && hasEnabledRow);
  m_resetButton->setEnabled(hasRows && hasEnabledRow);
  if (!hasEnabledRow && hasRows) {
    m_summaryLabel->setText("Sub Tool: (No compatible preset for active layer kind)");
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
  if (!item->data(kSubToolEnabledRole).toBool()) {
    return;
  }
  const QString id = item->data(kSubToolIdRole).toString();
  if (id.isEmpty()) {
    return;
  }
  m_controller->setCurrentSubTool(id.toStdString());
}

void SubToolPanel::onFilterTextChanged(const QString& text) {
  Q_UNUSED(text);
  refreshFromController();
}

void SubToolPanel::onDuplicateClicked() {
  if (m_controller == nullptr) {
    return;
  }
  if (!m_controller->duplicateCurrentSubTool()) {
    return;
  }
  refreshFromController();
}

void SubToolPanel::onRenameClicked() {
  if (m_controller == nullptr) {
    return;
  }
  bool ok = false;
  const QString current = QString::fromStdString(m_controller->currentSubToolDisplayName());
  const QString renamed = QInputDialog::getText(this, "Rename Sub Tool", "New name", QLineEdit::Normal, current, &ok);
  if (!ok) {
    return;
  }
  if (m_controller->renameCurrentSubTool(renamed.toStdString())) {
    refreshFromController();
  }
}

void SubToolPanel::onDeleteClicked() {
  if (m_controller == nullptr) {
    return;
  }
  const QString name = QString::fromStdString(m_controller->currentSubToolDisplayName());
  const auto answer = QMessageBox::question(this, "Delete Sub Tool", QString("Delete '%1'?").arg(name));
  if (answer != QMessageBox::Yes) {
    return;
  }
  if (m_controller->deleteCurrentSubTool()) {
    refreshFromController();
  }
}

void SubToolPanel::onResetClicked() {
  if (m_controller == nullptr) {
    return;
  }
  if (m_controller->resetCurrentSubTool()) {
    refreshFromController();
  }
}

} // namespace app::panels
