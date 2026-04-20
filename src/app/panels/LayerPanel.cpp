#include "app/panels/LayerPanel.h"

#include <QColor>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSlider>
#include <QStyle>
#include <QSize>
#include <QVBoxLayout>

#include "app/bridge/AppController.h"

namespace app::panels {

namespace {

constexpr int kNameRole = Qt::UserRole + 1;
constexpr int kVisibilityRole = Qt::UserRole + 2;

} // namespace

LayerPanel::LayerPanel(QWidget* parent)
    : QWidget(parent),
      m_headerLabel(new QLabel("Layers", this)),
      m_layerList(new QListWidget(this)),
      m_opacityLabel(new QLabel("Opacity: 100%", this)),
      m_opacitySlider(new QSlider(Qt::Horizontal, this)),
      m_addButton(new QPushButton("Add", this)),
      m_upButton(new QPushButton("Up", this)),
      m_downButton(new QPushButton("Down", this)),
      m_deleteButton(new QPushButton("Delete", this)) {
  m_headerLabel->setStyleSheet("font-weight: 700;");
  m_layerList->setAlternatingRowColors(true);
  m_layerList->setSelectionMode(QAbstractItemView::SingleSelection);
  m_layerList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
  m_layerList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_layerList->setSpacing(2);

  m_opacitySlider->setRange(0, 100);
  m_opacitySlider->setValue(100);
  m_opacitySlider->setToolTip("Active layer opacity");

  m_addButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
  m_upButton->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
  m_downButton->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
  m_deleteButton->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(6, 6, 6, 6);
  layout->setSpacing(6);
  layout->addWidget(m_headerLabel);
  layout->addWidget(m_layerList);
  layout->addWidget(m_opacityLabel);
  layout->addWidget(m_opacitySlider);

  auto* buttonRow = new QHBoxLayout();
  buttonRow->addWidget(m_addButton);
  buttonRow->addWidget(m_upButton);
  buttonRow->addWidget(m_downButton);
  buttonRow->addWidget(m_deleteButton);
  layout->addLayout(buttonRow);

  setLayout(layout);

  connect(m_addButton, &QPushButton::clicked, this, &LayerPanel::onAddLayerClicked);
  connect(m_upButton, &QPushButton::clicked, this, &LayerPanel::onMoveLayerUpClicked);
  connect(m_downButton, &QPushButton::clicked, this, &LayerPanel::onMoveLayerDownClicked);
  connect(m_deleteButton, &QPushButton::clicked, this, &LayerPanel::onDeleteLayerClicked);
  connect(m_layerList, &QListWidget::currentRowChanged, this, &LayerPanel::onCurrentLayerChanged);
  connect(m_layerList, &QListWidget::itemChanged, this, &LayerPanel::onLayerItemChanged);
  connect(m_opacitySlider, &QSlider::valueChanged, this, &LayerPanel::onOpacityChanged);
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
    item->setSizeHint(QSize(item->sizeHint().width(), 28));

    QFont font = item->font();
    font.setBold(model.active);
    item->setFont(font);
    item->setBackground(model.active ? QColor(48, 79, 130) : QColor(Qt::transparent));

    if (model.active) {
      m_layerList->setCurrentRow(i);
      const QSignalBlocker sliderBlocker(m_opacitySlider);
      m_opacitySlider->setValue(model.opacityPercent);
      m_opacityLabel->setText(QString("Opacity: %1%").arg(model.opacityPercent));
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

void LayerPanel::onMoveLayerUpClicked() {
  if (m_controller == nullptr) {
    return;
  }
  const int row = m_layerList->currentRow();
  if (row < 0) {
    return;
  }
  m_controller->moveLayerUp(static_cast<std::size_t>(row));
}

void LayerPanel::onMoveLayerDownClicked() {
  if (m_controller == nullptr) {
    return;
  }
  const int row = m_layerList->currentRow();
  if (row < 0) {
    return;
  }
  m_controller->moveLayerDown(static_cast<std::size_t>(row));
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

void LayerPanel::onOpacityChanged(int value) {
  if (m_controller == nullptr || m_isRefreshing) {
    return;
  }
  m_opacityLabel->setText(QString("Opacity: %1%").arg(value));
  m_controller->setActiveLayerOpacity(value);
}

void LayerPanel::refreshButtonState() {
  if (m_controller == nullptr) {
    m_deleteButton->setEnabled(false);
    m_upButton->setEnabled(false);
    m_downButton->setEnabled(false);
    m_opacitySlider->setEnabled(false);
    return;
  }
  const bool hasSelection = m_layerList->currentRow() >= 0;
  const bool canDelete = m_controller->document().layerCount() > 1;
  const int current = m_layerList->currentRow();
  const bool canMoveUp = current >= 0 && static_cast<std::size_t>(current + 1) < m_controller->document().layerCount();
  const bool canMoveDown = current > 0;
  m_deleteButton->setEnabled(hasSelection && canDelete);
  m_upButton->setEnabled(canMoveUp);
  m_downButton->setEnabled(canMoveDown);
  m_opacitySlider->setEnabled(hasSelection);
}

} // namespace app::panels
