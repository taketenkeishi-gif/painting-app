#include "app/panels/LayerPanel.h"

#include <algorithm>

#include <QColor>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QAbstractItemModel>
#include <QList>
#include <QModelIndex>
#include <QSignalBlocker>
#include <QSlider>
#include <QSizePolicy>
#include <QStyle>
#include <QSize>
#include <QVBoxLayout>

#include "app/bridge/AppController.h"

namespace app::panels {

namespace {

constexpr int kNameRole = Qt::UserRole + 1;
constexpr int kVisibilityRole = Qt::UserRole + 2;
constexpr int kKindRole = Qt::UserRole + 3;
constexpr int kLayerIndexRole = Qt::UserRole + 4;
constexpr int kClippedRole = Qt::UserRole + 5;
constexpr int kHasMaskRole = Qt::UserRole + 6;
constexpr int kMaskEnabledRole = Qt::UserRole + 7;
constexpr int kLockedRole = Qt::UserRole + 8;
constexpr int kAlphaLockedRole = Qt::UserRole + 9;
constexpr int kPositionLockedRole = Qt::UserRole + 10;

QString kindPrefix(core::LayerKind kind) {
  switch (kind) {
    case core::LayerKind::Raster:
      return "[R] ";
    case core::LayerKind::Vector:
      return "[V] ";
    case core::LayerKind::Folder:
      return "[F] ";
    default:
      return "[?] ";
  }
}

QString decorateLayerName(const app::bridge::LayerViewModel& model) {
  QString text = kindPrefix(model.kind);
  if (model.clippedToBelow) {
    text += "[C] ";
  }
  if (model.hasMask) {
    text += model.maskEnabled ? "[M] " : "[m] ";
  }
  if (model.locked) {
    text += "[L] ";
  }
  if (model.alphaLocked) {
    text += "[A] ";
  }
  if (model.positionLocked) {
    text += "[P] ";
  }
  text += QString::fromStdString(model.name);
  return text;
}

QString stripLayerDecorators(QString text) {
  text = text.trimmed();
  while (text.startsWith('[')) {
    const int close = text.indexOf(']');
    if (close <= 0) {
      break;
    }
    text = text.mid(close + 1).trimmed();
  }
  return text;
}

} // namespace

LayerPanel::LayerPanel(QWidget* parent)
    : QWidget(parent),
      m_headerLabel(new QLabel("Layers", this)),
      m_filterEdit(new QLineEdit(this)),
      m_layerList(new QListWidget(this)),
      m_opacityLabel(new QLabel("Opacity: 100%", this)),
      m_opacitySlider(new QSlider(Qt::Horizontal, this)),
      m_addRasterButton(new QPushButton("New Raster", this)),
      m_addVectorButton(new QPushButton("New Vector", this)),
      m_addFolderButton(new QPushButton("New Folder", this)),
      m_duplicateButton(new QPushButton("Duplicate", this)),
      m_upButton(new QPushButton("Up", this)),
      m_downButton(new QPushButton("Down", this)),
      m_deleteButton(new QPushButton("Delete", this)),
      m_clipButton(new QPushButton("Clip", this)),
      m_maskButton(new QPushButton("Mask", this)),
      m_removeMaskButton(new QPushButton("Mask Off", this)),
      m_lockButton(new QPushButton("Lock", this)),
      m_lockAlphaButton(new QPushButton("Alpha", this)),
      m_lockPositionButton(new QPushButton("Pos", this)) {
  m_headerLabel->setStyleSheet("font-weight: 700;");
  m_filterEdit->setPlaceholderText("Search layers...");
  m_filterEdit->setClearButtonEnabled(true);
  m_layerList->setAlternatingRowColors(true);
  m_layerList->setSelectionMode(QAbstractItemView::SingleSelection);
  m_layerList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
  m_layerList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_layerList->setUniformItemSizes(true);
  m_layerList->setDragEnabled(true);
  m_layerList->viewport()->setAcceptDrops(true);
  m_layerList->setDropIndicatorShown(true);
  m_layerList->setDragDropMode(QAbstractItemView::InternalMove);
  m_layerList->setDefaultDropAction(Qt::MoveAction);
  m_layerList->setSpacing(2);
  m_layerList->setStyleSheet(
      "QListWidget::item { padding: 5px 8px; border-bottom: 1px solid #313844; }"
      "QListWidget::item:selected { background: #2e4f79; color: #ffffff; }"
      "QListWidget::item:drop { border-top: 2px solid #7fb3ff; }");

  m_opacitySlider->setRange(0, 100);
  m_opacitySlider->setValue(100);
  m_opacitySlider->setToolTip("Active layer opacity");

  m_addRasterButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
  m_addVectorButton->setIcon(style()->standardIcon(QStyle::SP_DriveNetIcon));
  m_addFolderButton->setIcon(style()->standardIcon(QStyle::SP_DirClosedIcon));
  m_duplicateButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
  m_upButton->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
  m_downButton->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
  m_deleteButton->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
  m_clipButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
  m_maskButton->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
  m_removeMaskButton->setIcon(style()->standardIcon(QStyle::SP_DialogNoButton));
  const QList<QPushButton*> buttons {
      m_addRasterButton,
      m_addVectorButton,
      m_addFolderButton,
      m_duplicateButton,
      m_upButton,
      m_downButton,
      m_deleteButton,
      m_clipButton,
      m_maskButton,
      m_removeMaskButton,
      m_lockButton,
      m_lockAlphaButton,
      m_lockPositionButton};
  for (QPushButton* button : buttons) {
    button->setMinimumHeight(30);
    button->setMinimumWidth(90);
    button->setIconSize(QSize(14, 14));
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  }

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(6, 6, 6, 6);
  layout->setSpacing(6);
  layout->addWidget(m_headerLabel);
  layout->addWidget(m_filterEdit);
  layout->addWidget(m_layerList);
  layout->addWidget(m_opacityLabel);
  layout->addWidget(m_opacitySlider);

  auto* commandGrid = new QGridLayout();
  commandGrid->setContentsMargins(0, 0, 0, 0);
  commandGrid->setHorizontalSpacing(4);
  commandGrid->setVerticalSpacing(4);
  commandGrid->addWidget(m_addRasterButton, 0, 0);
  commandGrid->addWidget(m_addVectorButton, 0, 1);
  commandGrid->addWidget(m_addFolderButton, 0, 2);
  commandGrid->addWidget(m_duplicateButton, 0, 3);
  commandGrid->addWidget(m_upButton, 1, 0);
  commandGrid->addWidget(m_downButton, 1, 1);
  commandGrid->addWidget(m_deleteButton, 1, 2);
  commandGrid->setColumnStretch(3, 1);
  layout->addLayout(commandGrid);

  auto* stateGrid = new QGridLayout();
  stateGrid->setContentsMargins(0, 0, 0, 0);
  stateGrid->setHorizontalSpacing(4);
  stateGrid->setVerticalSpacing(4);
  stateGrid->addWidget(m_clipButton, 0, 0);
  stateGrid->addWidget(m_maskButton, 0, 1);
  stateGrid->addWidget(m_removeMaskButton, 0, 2);
  stateGrid->addWidget(m_lockButton, 1, 0);
  stateGrid->addWidget(m_lockAlphaButton, 1, 1);
  stateGrid->addWidget(m_lockPositionButton, 1, 2);
  layout->addLayout(stateGrid);

  setLayout(layout);

  connect(m_addRasterButton, &QPushButton::clicked, this, &LayerPanel::onAddRasterLayerClicked);
  connect(m_addVectorButton, &QPushButton::clicked, this, &LayerPanel::onAddVectorLayerClicked);
  connect(m_addFolderButton, &QPushButton::clicked, this, &LayerPanel::onAddFolderLayerClicked);
  connect(m_duplicateButton, &QPushButton::clicked, this, &LayerPanel::onDuplicateLayerClicked);
  connect(m_upButton, &QPushButton::clicked, this, &LayerPanel::onMoveLayerUpClicked);
  connect(m_downButton, &QPushButton::clicked, this, &LayerPanel::onMoveLayerDownClicked);
  connect(m_deleteButton, &QPushButton::clicked, this, &LayerPanel::onDeleteLayerClicked);
  connect(m_clipButton, &QPushButton::clicked, this, &LayerPanel::onToggleClipClicked);
  connect(m_maskButton, &QPushButton::clicked, this, &LayerPanel::onToggleMaskClicked);
  connect(m_removeMaskButton, &QPushButton::clicked, this, &LayerPanel::onRemoveMaskClicked);
  connect(m_lockButton, &QPushButton::clicked, this, &LayerPanel::onToggleLockClicked);
  connect(m_lockAlphaButton, &QPushButton::clicked, this, &LayerPanel::onToggleAlphaLockClicked);
  connect(m_lockPositionButton, &QPushButton::clicked, this, &LayerPanel::onTogglePositionLockClicked);
  connect(m_layerList, &QListWidget::currentRowChanged, this, &LayerPanel::onCurrentLayerChanged);
  connect(m_layerList, &QListWidget::itemChanged, this, &LayerPanel::onLayerItemChanged);
  connect(m_filterEdit, &QLineEdit::textChanged, this, &LayerPanel::onFilterTextChanged);
  connect(
      m_layerList->model(),
      &QAbstractItemModel::rowsMoved,
      this,
      &LayerPanel::onLayerRowsMoved);
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
  const QString filterText = m_filterEdit->text().trimmed();
  const bool hasFilter = !filterText.isEmpty();
  m_layerList->setDragDropMode(hasFilter ? QAbstractItemView::NoDragDrop : QAbstractItemView::InternalMove);

  const auto models = m_controller->layerViewModels();
  for (std::size_t layerIndex = models.size(); layerIndex-- > 0;) {
    const auto& model = models[layerIndex];
    if (hasFilter) {
      const QString layerName = QString::fromStdString(model.name);
      if (!layerName.contains(filterText, Qt::CaseInsensitive)) {
        continue;
      }
    }
    auto* item = new QListWidgetItem(decorateLayerName(model), m_layerList);
    item->setFlags(
        Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEditable |
        Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    item->setCheckState(model.visible ? Qt::Checked : Qt::Unchecked);
    item->setData(kNameRole, QString::fromStdString(model.name));
    item->setData(kVisibilityRole, model.visible);
    item->setData(kKindRole, static_cast<int>(model.kind));
    item->setData(kLayerIndexRole, static_cast<int>(layerIndex));
    item->setData(kClippedRole, model.clippedToBelow);
    item->setData(kHasMaskRole, model.hasMask);
    item->setData(kMaskEnabledRole, model.maskEnabled);
    item->setData(kLockedRole, model.locked);
    item->setData(kAlphaLockedRole, model.alphaLocked);
    item->setData(kPositionLockedRole, model.positionLocked);
    item->setToolTip("Toggle visibility with the checkbox. Double-click name to rename.");
    item->setSizeHint(QSize(item->sizeHint().width(), 30));

    QFont font = item->font();
    font.setBold(model.active);
    item->setFont(font);
    item->setBackground(model.active ? QColor(48, 79, 130) : QColor(Qt::transparent));

    if (model.active) {
      m_layerList->setCurrentItem(item);
      const QSignalBlocker sliderBlocker(m_opacitySlider);
      m_opacitySlider->setValue(model.opacityPercent);
      m_opacityLabel->setText(QString("Opacity: %1%").arg(model.opacityPercent));
    }
  }

  m_isRefreshing = false;
  refreshButtonState();
}

void LayerPanel::onAddRasterLayerClicked() {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->addRasterLayer();
}

void LayerPanel::onAddVectorLayerClicked() {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->addVectorLayer();
}

void LayerPanel::onAddFolderLayerClicked() {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->addFolderLayer();
}

void LayerPanel::onDuplicateLayerClicked() {
  if (m_controller == nullptr) {
    return;
  }
  QListWidgetItem* currentItem = m_layerList->currentItem();
  if (currentItem == nullptr) {
    return;
  }
  const int layerIndex = currentItem->data(kLayerIndexRole).toInt();
  if (layerIndex < 0) {
    return;
  }
  m_controller->duplicateLayer(static_cast<std::size_t>(layerIndex));
}

void LayerPanel::onDeleteLayerClicked() {
  if (m_controller == nullptr) {
    return;
  }
  QListWidgetItem* currentItem = m_layerList->currentItem();
  if (currentItem == nullptr) {
    return;
  }
  const int layerIndex = currentItem->data(kLayerIndexRole).toInt();
  if (layerIndex < 0) {
    return;
  }
  m_controller->removeLayer(static_cast<std::size_t>(layerIndex));
}

void LayerPanel::onMoveLayerUpClicked() {
  if (m_controller == nullptr) {
    return;
  }
  QListWidgetItem* currentItem = m_layerList->currentItem();
  if (currentItem == nullptr) {
    return;
  }
  const int layerIndex = currentItem->data(kLayerIndexRole).toInt();
  if (layerIndex < 0) {
    return;
  }
  m_controller->moveLayerUp(static_cast<std::size_t>(layerIndex));
}

void LayerPanel::onMoveLayerDownClicked() {
  if (m_controller == nullptr) {
    return;
  }
  QListWidgetItem* currentItem = m_layerList->currentItem();
  if (currentItem == nullptr) {
    return;
  }
  const int layerIndex = currentItem->data(kLayerIndexRole).toInt();
  if (layerIndex < 0) {
    return;
  }
  m_controller->moveLayerDown(static_cast<std::size_t>(layerIndex));
}

void LayerPanel::onToggleClipClicked() {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->toggleActiveLayerClipToBelow();
}

void LayerPanel::onToggleMaskClicked() {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->toggleActiveLayerMask();
}

void LayerPanel::onRemoveMaskClicked() {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->removeActiveLayerMask();
}

void LayerPanel::onToggleLockClicked() {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->toggleActiveLayerLock();
}

void LayerPanel::onToggleAlphaLockClicked() {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->toggleActiveLayerAlphaLock();
}

void LayerPanel::onTogglePositionLockClicked() {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->toggleActiveLayerPositionLock();
}

void LayerPanel::onCurrentLayerChanged(int row) {
  if (m_controller == nullptr || m_isRefreshing || row < 0) {
    return;
  }
  QListWidgetItem* item = m_layerList->item(row);
  if (item == nullptr) {
    return;
  }
  const int layerIndex = item->data(kLayerIndexRole).toInt();
  if (layerIndex < 0) {
    return;
  }
  m_controller->setActiveLayer(static_cast<std::size_t>(layerIndex));
  refreshButtonState();
}

void LayerPanel::onLayerItemChanged(QListWidgetItem* item) {
  if (m_controller == nullptr || m_isRefreshing || item == nullptr) {
    return;
  }
  const int row = m_layerList->row(item);
  if (row < 0) {
    return;
  }
  const int layerIndexValue = item->data(kLayerIndexRole).toInt();
  const std::size_t layerIndex =
      layerIndexValue >= 0 ? static_cast<std::size_t>(layerIndexValue) : layerIndexFromRow(row);

  const QString oldName = item->data(kNameRole).toString();
  QString newName = stripLayerDecorators(item->text());
  if (newName.isEmpty()) {
    const QSignalBlocker signalBlocker(m_layerList);
    const core::LayerKind kind = static_cast<core::LayerKind>(item->data(kKindRole).toInt());
    app::bridge::LayerViewModel model;
    model.name = oldName.toStdString();
    model.kind = kind;
    model.clippedToBelow = item->data(kClippedRole).toBool();
    model.hasMask = item->data(kHasMaskRole).toBool();
    model.maskEnabled = item->data(kMaskEnabledRole).toBool();
    model.locked = item->data(kLockedRole).toBool();
    model.alphaLocked = item->data(kAlphaLockedRole).toBool();
    model.positionLocked = item->data(kPositionLockedRole).toBool();
    item->setText(decorateLayerName(model));
    return;
  }

  const bool visible = item->checkState() == Qt::Checked;
  const bool oldVisible = item->data(kVisibilityRole).toBool();

  const bool nameChanged = newName != oldName;
  const bool visibilityChanged = visible != oldVisible;

  if (nameChanged) {
    m_controller->renameLayer(layerIndex, newName.toStdString());
  }
  if (visibilityChanged) {
    m_controller->setLayerVisible(layerIndex, visible);
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

void LayerPanel::onFilterTextChanged(const QString& text) {
  Q_UNUSED(text);
  refreshLayers();
}

void LayerPanel::onLayerRowsMoved(
    const QModelIndex& parent,
    int start,
    int end,
    const QModelIndex& destination,
    int row) {
  Q_UNUSED(parent);
  Q_UNUSED(destination);
  if (m_controller == nullptr || m_isRefreshing || m_isDraggingLayer) {
    return;
  }
  if (start < 0 || end != start) {
    return;
  }
  int toRow = row;
  if (toRow > m_layerList->count()) {
    toRow = m_layerList->count();
  }
  if (toRow == m_layerList->count()) {
    toRow = m_layerList->count() - 1;
  }
  if (toRow > start) {
    --toRow;
  }
  if (toRow < 0 || toRow == start) {
    return;
  }

  const std::size_t fromLayerIndex = layerIndexFromRow(start);
  const std::size_t toLayerIndex = layerIndexFromRow(toRow);
  m_isDraggingLayer = true;
  const bool moved = m_controller->moveLayer(fromLayerIndex, toLayerIndex);
  m_isDraggingLayer = false;
  if (!moved) {
    refreshLayers();
  }
}

std::size_t LayerPanel::layerIndexFromRow(int row) const {
  if (m_controller == nullptr) {
    return 0;
  }
  const std::size_t count = m_controller->document().layerCount();
  if (count == 0) {
    return 0;
  }
  const int clamped = std::clamp(row, 0, static_cast<int>(count) - 1);
  return count - 1 - static_cast<std::size_t>(clamped);
}

int LayerPanel::rowFromLayerIndex(std::size_t layerIndex) const {
  if (m_controller == nullptr) {
    return 0;
  }
  const std::size_t count = m_controller->document().layerCount();
  if (count == 0) {
    return 0;
  }
  const std::size_t clamped = std::min(layerIndex, count - 1);
  return static_cast<int>(count - 1 - clamped);
}

void LayerPanel::refreshButtonState() {
  if (m_controller == nullptr) {
    m_deleteButton->setEnabled(false);
    m_duplicateButton->setEnabled(false);
    m_upButton->setEnabled(false);
    m_downButton->setEnabled(false);
    m_opacitySlider->setEnabled(false);
    m_clipButton->setEnabled(false);
    m_maskButton->setEnabled(false);
    m_removeMaskButton->setEnabled(false);
    m_lockButton->setEnabled(false);
    m_lockAlphaButton->setEnabled(false);
    m_lockPositionButton->setEnabled(false);
    return;
  }
  const bool hasSelection = m_layerList->currentRow() >= 0;
  const bool canDelete = m_controller->document().layerCount() > 1;
  const int current = m_layerList->currentRow();
  const int lastRow = static_cast<int>(m_controller->document().layerCount()) - 1;
  const bool canMoveUp = current > 0;
  const bool canMoveDown = current >= 0 && current < lastRow;
  m_deleteButton->setEnabled(hasSelection && canDelete);
  m_duplicateButton->setEnabled(hasSelection);
  m_upButton->setEnabled(canMoveUp);
  m_downButton->setEnabled(canMoveDown);
  m_opacitySlider->setEnabled(hasSelection);

  bool canClipOrMask = false;
  bool hasMask = false;
  bool maskEnabled = false;
  bool clipped = false;
  bool locked = false;
  bool alphaLocked = false;
  bool positionLocked = false;
  core::LayerKind kind = core::LayerKind::Raster;
  if (hasSelection) {
    QListWidgetItem* currentItem = m_layerList->item(current);
    if (currentItem != nullptr) {
      const int layerIndex = currentItem->data(kLayerIndexRole).toInt();
      if (layerIndex >= 0) {
        const core::Layer& layer = m_controller->document().layerAt(static_cast<std::size_t>(layerIndex));
        kind = layer.kind();
        canClipOrMask = layer.kind() != core::LayerKind::Folder;
        hasMask = layer.hasMask();
        maskEnabled = layer.maskEnabled();
        clipped = layer.clippedToBelow();
        locked = layer.locked();
        alphaLocked = layer.alphaLocked();
        positionLocked = layer.positionLocked();
      }
    }
  }
  m_clipButton->setEnabled(canClipOrMask);
  m_maskButton->setEnabled(canClipOrMask);
  m_removeMaskButton->setEnabled(canClipOrMask && hasMask);
  m_lockButton->setEnabled(hasSelection && kind != core::LayerKind::Folder);
  m_lockAlphaButton->setEnabled(hasSelection && kind == core::LayerKind::Raster);
  m_lockPositionButton->setEnabled(hasSelection && kind != core::LayerKind::Folder);
  m_clipButton->setText(clipped ? "Clip On" : "Clip");
  m_maskButton->setText(maskEnabled ? "Mask On" : "Mask");
  m_lockButton->setText(locked ? "Lock On" : "Lock");
  m_lockAlphaButton->setText(alphaLocked ? "Alpha On" : "Alpha");
  m_lockPositionButton->setText(positionLocked ? "Pos On" : "Pos");
}

} // namespace app::panels
