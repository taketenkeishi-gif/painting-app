#include "app/panels/LayerPanel.h"

#include <QColor>
#include <QFont>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "app/bridge/AppController.h"

namespace app::panels {

namespace {

constexpr int kNameRole = Qt::UserRole + 1;
constexpr int kVisibilityRole = Qt::UserRole + 2;

} // namespace

LayerPanel::LayerPanel(QWidget* parent)
    : QWidget(parent),
      m_layerList(new QListWidget(this)),
      m_addButton(new QPushButton("Add", this)),
      m_deleteButton(new QPushButton("Delete", this)) {
  m_layerList->setAlternatingRowColors(true);
  m_layerList->setSelectionMode(QAbstractItemView::SingleSelection);
  m_layerList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(m_layerList);

  auto* buttonRow = new QHBoxLayout();
  buttonRow->addWidget(m_addButton);
  buttonRow->addWidget(m_deleteButton);
  layout->addLayout(buttonRow);

  setLayout(layout);

  connect(m_addButton, &QPushButton::clicked, this, &LayerPanel::onAddLayerClicked);
  connect(m_deleteButton, &QPushButton::clicked, this, &LayerPanel::onDeleteLayerClicked);
  connect(m_layerList, &QListWidget::currentRowChanged, this, &LayerPanel::onCurrentLayerChanged);
  connect(m_layerList, &QListWidget::itemChanged, this, &LayerPanel::onLayerItemChanged);
}

void LayerPanel::setController(app::bridge::AppController* controller) {
  if (m_controller != nullptr) {
    disconnect(m_controller, nullptr, this, nullptr);
  }

  m_controller = controller;
  if (m_controller == nullptr) {
    return;
  }

  connect(m_controller, &app::bridge::AppController::layersChanged, this, &LayerPanel::refreshLayers);
  refreshLayers();
}

void LayerPanel::refreshLayers() {
  if (m_controller == nullptr) {
    return;
  }

  const QSignalBlocker signalBlocker(m_layerList);
  m_isRefreshing = true;
  m_layerList->clear();

  const auto models = m_controller->layerViewModels();
  for (int i = 0; i < static_cast<int>(models.size()); ++i) {
    const auto& model = models[static_cast<std::size_t>(i)];
    auto* item = new QListWidgetItem(QString::fromStdString(model.name), m_layerList);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
    item->setCheckState(model.visible ? Qt::Checked : Qt::Unchecked);
    item->setData(kNameRole, QString::fromStdString(model.name));
    item->setData(kVisibilityRole, model.visible);
    item->setToolTip("Toggle visibility with the checkbox. Double-click name to rename.");

    QFont font = item->font();
    font.setBold(model.active);
    item->setFont(font);
    item->setBackground(model.active ? QColor(55, 80, 120) : QColor(Qt::transparent));

    if (model.active) {
      m_layerList->setCurrentRow(i);
    }
  }

  m_isRefreshing = false;
  refreshButtonState();
}

void LayerPanel::onAddLayerClicked() {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->addLayer();
}

void LayerPanel::onDeleteLayerClicked() {
  if (m_controller == nullptr) {
    return;
  }
  const int row = m_layerList->currentRow();
  if (row < 0) {
    return;
  }
  m_controller->removeLayer(static_cast<std::size_t>(row));
}

void LayerPanel::onCurrentLayerChanged(int row) {
  if (m_controller == nullptr || m_isRefreshing || row < 0) {
    return;
  }
  m_controller->setActiveLayer(static_cast<std::size_t>(row));
}

void LayerPanel::onLayerItemChanged(QListWidgetItem* item) {
  if (m_controller == nullptr || m_isRefreshing || item == nullptr) {
    return;
  }
  const int row = m_layerList->row(item);
  if (row < 0) {
    return;
  }

  const QString oldName = item->data(kNameRole).toString();
  const QString newName = item->text().trimmed();
  if (newName.isEmpty()) {
    const QSignalBlocker signalBlocker(m_layerList);
    item->setText(oldName);
    return;
  }

  const bool visible = item->checkState() == Qt::Checked;
  const bool oldVisible = item->data(kVisibilityRole).toBool();

  const bool nameChanged = newName != oldName;
  const bool visibilityChanged = visible != oldVisible;

  if (nameChanged) {
    m_controller->renameLayer(static_cast<std::size_t>(row), newName.toStdString());
  }
  if (visibilityChanged) {
    m_controller->setLayerVisible(static_cast<std::size_t>(row), visible);
  }
  if (!nameChanged && !visibilityChanged) {
    refreshButtonState();
  }
}

void LayerPanel::refreshButtonState() {
  if (m_controller == nullptr) {
    m_deleteButton->setEnabled(false);
    return;
  }
  const bool hasSelection = m_layerList->currentRow() >= 0;
  const bool canDelete = m_controller->document().layerCount() > 1;
  m_deleteButton->setEnabled(hasSelection && canDelete);
}

} // namespace app::panels
