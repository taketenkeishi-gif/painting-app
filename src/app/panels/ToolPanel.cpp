#include "app/panels/ToolPanel.h"

#include <algorithm>
#include <vector>

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QSignalBlocker>
#include <QSlider>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include "app/bridge/AppController.h"

namespace app::panels {

namespace {

QString toolNameJa(core::ToolKind kind) {
  switch (kind) {
    case core::ToolKind::Brush:
      return QString::fromUtf8(u8"ブラシ");
    case core::ToolKind::Eraser:
      return QString::fromUtf8(u8"消しゴム");
    case core::ToolKind::Eyedropper:
      return QString::fromUtf8(u8"スポイト");
    case core::ToolKind::Fill:
      return QString::fromUtf8(u8"塗りつぶし");
    case core::ToolKind::Line:
      return QString::fromUtf8(u8"直線");
    case core::ToolKind::RectSelection:
      return QString::fromUtf8(u8"選択");
    case core::ToolKind::MoveLayer:
      return QString::fromUtf8(u8"移動");
    case core::ToolKind::Hand:
      return QString::fromUtf8(u8"手のひら");
    case core::ToolKind::Zoom:
      return QString::fromUtf8(u8"ズーム");
    default:
      return QString::fromUtf8(u8"ツール");
  }
}

QString toolShortcut(core::ToolKind kind) {
  switch (kind) {
    case core::ToolKind::Brush:
      return "B";
    case core::ToolKind::Eraser:
      return "E";
    case core::ToolKind::Eyedropper:
      return "I";
    case core::ToolKind::Fill:
      return "G";
    case core::ToolKind::Line:
      return "U";
    case core::ToolKind::RectSelection:
      return "R";
    case core::ToolKind::MoveLayer:
      return "M";
    case core::ToolKind::Hand:
      return "H";
    case core::ToolKind::Zoom:
      return "Z";
    default:
      return {};
  }
}

QIcon toolIcon(QWidget* owner, core::ToolKind kind) {
  switch (kind) {
    case core::ToolKind::Brush:
      return owner->style()->standardIcon(QStyle::SP_DriveFDIcon);
    case core::ToolKind::Eraser:
      return owner->style()->standardIcon(QStyle::SP_DialogResetButton);
    case core::ToolKind::Eyedropper:
      return owner->style()->standardIcon(QStyle::SP_BrowserReload);
    case core::ToolKind::Fill:
      return owner->style()->standardIcon(QStyle::SP_DialogApplyButton);
    case core::ToolKind::Line:
      return owner->style()->standardIcon(QStyle::SP_ArrowForward);
    case core::ToolKind::RectSelection:
      return owner->style()->standardIcon(QStyle::SP_DialogOpenButton);
    case core::ToolKind::MoveLayer:
      return owner->style()->standardIcon(QStyle::SP_ArrowUp);
    case core::ToolKind::Hand:
      return owner->style()->standardIcon(QStyle::SP_TitleBarNormalButton);
    case core::ToolKind::Zoom:
      return owner->style()->standardIcon(QStyle::SP_FileDialogDetailedView);
    default:
      return QIcon {};
  }
}

QWidget* makeQuickSliderBlock(
    QWidget* parent,
    const QString& labelText,
    QSlider*& sliderOut,
    QLabel*& valueLabelOut,
    int min,
    int max) {
  auto* block = new QWidget(parent);
  auto* blockLayout = new QVBoxLayout(block);
  blockLayout->setContentsMargins(0, 0, 0, 0);
  blockLayout->setSpacing(2);

  auto* top = new QHBoxLayout();
  top->setContentsMargins(0, 0, 0, 0);
  top->setSpacing(2);
  auto* label = new QLabel(labelText, block);
  label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  valueLabelOut = new QLabel("0", block);
  valueLabelOut->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  valueLabelOut->setMinimumWidth(20);
  top->addWidget(label);
  top->addWidget(valueLabelOut, 1);

  sliderOut = new QSlider(Qt::Vertical, block);
  sliderOut->setRange(min, max);
  sliderOut->setInvertedAppearance(true);
  sliderOut->setFixedSize(12, 98);
  sliderOut->setFocusPolicy(Qt::StrongFocus);

  auto* sliderHolder = new QHBoxLayout();
  sliderHolder->setContentsMargins(0, 0, 0, 0);
  sliderHolder->setSpacing(0);
  sliderHolder->addStretch(1);
  sliderHolder->addWidget(sliderOut);
  sliderHolder->addStretch(1);

  blockLayout->addLayout(top);
  blockLayout->addLayout(sliderHolder);
  return block;
}

} // namespace

ToolPanel::ToolPanel(QWidget* parent)
    : QWidget(parent) {
  setStyleSheet(
      "QToolButton {"
      "  border: 1px solid #3f4a59;"
      "  background: #262c35;"
      "  color: #d8deea;"
      "  border-radius: 3px;"
      "  padding: 0;"
      "  min-width: 24px;"
      "  min-height: 24px;"
      "}"
      "QToolButton:hover { background: #323a46; border-color: #7995ba; }"
      "QToolButton:checked { background: #2b4d73; border-color: #8ec0ff; color: #ffffff; }"
      "QToolButton:disabled { background: #1f232b; border-color: #2d3441; color: #6d7888; }"
      "QLabel { color: #c9d2df; }"
      "QSlider::groove:vertical { background: #161b23; border: 1px solid #343d4d; width: 4px; border-radius: 2px; }"
      "QSlider::handle:vertical { background: #82abd9; height: 10px; margin: 0 -4px; border-radius: 5px; }");

  auto* layout = new QGridLayout(this);
  layout->setContentsMargins(2, 2, 2, 2);
  layout->setHorizontalSpacing(2);
  layout->setVerticalSpacing(2);
  layout->setColumnStretch(0, 1);
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
  const QString layerKind = QString::fromStdString(m_controller->activeLayerKindDisplayName());
  for (const auto& [kind, button] : m_buttons) {
    const QSignalBlocker blocker(button);
    const bool enabled = m_controller->canUseToolOnActiveLayer(kind);
    QString tip = QString("%1  [%2]").arg(toolNameJa(kind), toolShortcut(kind));
    if (!enabled) {
      tip = QString("%1（%2では使用不可）").arg(toolNameJa(kind), layerKind);
    }
    button->setChecked(kind == current);
    button->setEnabled(enabled);
    button->setToolTip(tip);
  }

  if (m_sizeSlider != nullptr && m_opacitySlider != nullptr &&
      m_sizeValueLabel != nullptr && m_opacityValueLabel != nullptr) {
    const QSignalBlocker b1(m_sizeSlider);
    const QSignalBlocker b2(m_opacitySlider);
    m_refreshingSliders = true;
    const app::bridge::ToolStateViewModel state = m_controller->toolState();
    m_sizeSlider->setValue(state.size);
    m_opacitySlider->setValue(state.opacity);
    m_sizeValueLabel->setText(QString::number(state.size));
    m_opacityValueLabel->setText(QString::number(state.opacity));
    const bool sizeEnabled = m_controller->currentToolSupportsSize();
    const bool opacityEnabled = m_controller->currentToolSupportsOpacity();
    m_sizeSlider->setEnabled(sizeEnabled);
    m_opacitySlider->setEnabled(opacityEnabled);
    m_sizeValueLabel->setEnabled(sizeEnabled);
    m_opacityValueLabel->setEnabled(opacityEnabled);
    m_refreshingSliders = false;
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
  const core::ToolKind kind = static_cast<core::ToolKind>(button->property("toolKind").toInt());
  m_controller->setCurrentTool(kind);
}

void ToolPanel::onSizeSliderChanged(int value) {
  if (m_controller == nullptr || m_refreshingSliders) {
    return;
  }
  m_controller->setBrushSize(value);
}

void ToolPanel::onOpacitySliderChanged(int value) {
  if (m_controller == nullptr || m_refreshingSliders) {
    return;
  }
  m_controller->setBrushOpacity(value);
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
    if (item->widget() != nullptr) {
      item->widget()->deleteLater();
    }
    delete item;
  }

  const std::vector<core::ToolKind> ordered {
      core::ToolKind::Brush,
      core::ToolKind::Eraser,
      core::ToolKind::Eyedropper,
      core::ToolKind::Fill,
      core::ToolKind::Line,
      core::ToolKind::RectSelection,
      core::ToolKind::MoveLayer,
      core::ToolKind::Hand,
      core::ToolKind::Zoom};
  const auto available = m_controller->availableTools();

  int row = 0;
  for (core::ToolKind kind : ordered) {
    if (std::find(available.begin(), available.end(), kind) == available.end()) {
      continue;
    }
    auto* button = new QToolButton(this);
    button->setCheckable(true);
    button->setAutoExclusive(true);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setIcon(toolIcon(this, kind));
    button->setIconSize(QSize(14, 14));
    button->setFixedSize(24, 24);
    button->setProperty("toolKind", static_cast<int>(kind));
    connect(button, &QToolButton::clicked, this, &ToolPanel::onToolButtonClicked);
    layout->addWidget(button, row++, 0, Qt::AlignHCenter);
    m_buttons[kind] = button;
  }

  auto* divider = new QLabel(this);
  divider->setFixedSize(20, 1);
  divider->setStyleSheet("background:#3a4352;");
  layout->addWidget(divider, row++, 0, Qt::AlignHCenter);

  auto* quickWrap = new QHBoxLayout();
  quickWrap->setContentsMargins(0, 0, 0, 0);
  quickWrap->setSpacing(4);

  quickWrap->addWidget(makeQuickSliderBlock(this, "S", m_sizeSlider, m_sizeValueLabel, 1, 128));
  quickWrap->addWidget(makeQuickSliderBlock(this, "O", m_opacitySlider, m_opacityValueLabel, 0, 100));

  auto* quickContainer = new QWidget(this);
  quickContainer->setLayout(quickWrap);
  layout->addWidget(quickContainer, row++, 0, Qt::AlignHCenter);

  layout->setRowStretch(row, 1);

  connect(m_sizeSlider, &QSlider::valueChanged, this, &ToolPanel::onSizeSliderChanged);
  connect(m_opacitySlider, &QSlider::valueChanged, this, &ToolPanel::onOpacitySliderChanged);
}

} // namespace app::panels
