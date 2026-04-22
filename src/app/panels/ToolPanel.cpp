#include "app/panels/ToolPanel.h"

#include <algorithm>
#include <vector>

#include <QGridLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QSignalBlocker>
#include <QSlider>
#include <QStyle>
#include <QToolButton>

#include "app/bridge/AppController.h"

namespace app::panels {

namespace {

QString toolNameJa(core::ToolKind kind) {
  switch (kind) {
    case core::ToolKind::Brush:
      return "ブラシ";
    case core::ToolKind::Eraser:
      return "消しゴム";
    case core::ToolKind::Eyedropper:
      return "スポイト";
    case core::ToolKind::Fill:
      return "塗りつぶし";
    case core::ToolKind::Line:
      return "直線";
    case core::ToolKind::RectSelection:
      return "選択";
    case core::ToolKind::MoveLayer:
      return "移動";
    case core::ToolKind::Hand:
      return "手のひら";
    case core::ToolKind::Zoom:
      return "ズーム";
    default:
      return "ツール";
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

enum class ToolCategory {
  Paint,
  Select,
  View,
};

ToolCategory categoryFor(core::ToolKind kind) {
  switch (kind) {
    case core::ToolKind::Brush:
    case core::ToolKind::Eraser:
    case core::ToolKind::Eyedropper:
    case core::ToolKind::Fill:
    case core::ToolKind::Line:
      return ToolCategory::Paint;
    case core::ToolKind::RectSelection:
    case core::ToolKind::MoveLayer:
      return ToolCategory::Select;
    case core::ToolKind::Hand:
    case core::ToolKind::Zoom:
      return ToolCategory::View;
    default:
      return ToolCategory::Paint;
  }
}

QString categoryName(ToolCategory category) {
  switch (category) {
    case ToolCategory::Paint:
      return "描画";
    case ToolCategory::Select:
      return "選択/移動";
    case ToolCategory::View:
      return "表示";
    default:
      return "ツール";
  }
}

} // namespace

ToolPanel::ToolPanel(QWidget* parent)
    : QWidget(parent) {
  setStyleSheet(
      "QToolButton {"
      "  border: 1px solid #445163;"
      "  background: #29303a;"
      "  color: #d8d8d8;"
      "  border-radius: 4px;"
      "  padding: 3px 3px;"
      "  min-height: 40px;"
      "}"
      "QToolButton:hover { background: #36404c; border-color: #8aa8cf; }"
      "QToolButton:checked { background: #2d527f; border-color: #8fc2ff; color: #ffffff; }"
      "QToolButton:disabled { background: #20252d; border-color: #303845; color: #7c8798; }"
      "QSlider::groove:horizontal { background: #1f2530; border: 1px solid #39485d; height: 5px; border-radius: 3px; }"
      "QSlider::handle:horizontal { background: #87aee0; width: 12px; margin: -4px 0; border-radius: 6px; }");
  auto* layout = new QGridLayout(this);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setHorizontalSpacing(4);
  layout->setVerticalSpacing(3);
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
  const QString layerKind = QString::fromStdString(m_controller->activeLayerKindDisplayName());
  for (const auto& [kind, button] : m_buttons) {
    const QSignalBlocker blocker(button);
    const bool enabled = m_controller->canUseToolOnActiveLayer(kind);
    const QString shortcut = toolShortcut(kind);
    button->setChecked(kind == current);
    button->setToolTip(
        enabled
            ? QString("%1 [%2]").arg(toolNameJa(kind), shortcut)
            : QString("%1は現在のレイヤー(%2)では使用できません").arg(toolNameJa(kind), layerKind));
    button->setEnabled(enabled);
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
    m_opacityValueLabel->setText(QString("%1%").arg(state.opacity));
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

  auto* title = new QLabel("ツール", this);
  title->setStyleSheet("font-weight:700; color:#dfe6f2; padding:1px 1px;");
  layout->addWidget(title, 0, 0, 1, 2);

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
  const std::vector<ToolCategory> categories {
      ToolCategory::Paint,
      ToolCategory::Select,
      ToolCategory::View};
  const auto available = m_controller->availableTools();

  int row = 1;
  for (ToolCategory category : categories) {
    std::vector<core::ToolKind> tools;
    for (core::ToolKind kind : ordered) {
      if (categoryFor(kind) != category) {
        continue;
      }
      if (std::find(available.begin(), available.end(), kind) == available.end()) {
        continue;
      }
      tools.push_back(kind);
    }
    if (tools.empty()) {
      continue;
    }

    auto* label = new QLabel(categoryName(category), this);
    label->setStyleSheet("font-weight:600; color:#9fb4cf; padding:2px 1px;");
    layout->addWidget(label, row++, 0, 1, 2);

    for (std::size_t i = 0; i < tools.size(); ++i) {
      const core::ToolKind kind = tools[i];
      auto* button = new QToolButton(this);
      const QString shortKey = toolShortcut(kind);
      button->setText(QString("%1\n%2").arg(toolNameJa(kind), shortKey));
      button->setCheckable(true);
      button->setAutoExclusive(true);
      button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
      button->setMinimumSize(QSize(64, 44));
      button->setIconSize(QSize(16, 16));
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
          button->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
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
      layout->addWidget(button, row + static_cast<int>(i / 2), static_cast<int>(i % 2));
      m_buttons[kind] = button;
    }
    row += static_cast<int>((tools.size() + 1) / 2);
  }

  auto* sliderTitle = new QLabel("クイック調整", this);
  sliderTitle->setStyleSheet("font-weight:600; color:#9fb4cf; padding:2px 1px;");
  layout->addWidget(sliderTitle, row++, 0, 1, 2);

  auto* sizeLabel = new QLabel("サイズ", this);
  m_sizeValueLabel = new QLabel("8", this);
  m_sizeValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  layout->addWidget(sizeLabel, row, 0);
  layout->addWidget(m_sizeValueLabel, row, 1);
  ++row;

  m_sizeSlider = new QSlider(Qt::Horizontal, this);
  m_sizeSlider->setRange(1, 128);
  layout->addWidget(m_sizeSlider, row++, 0, 1, 2);

  auto* opacityLabel = new QLabel("不透明度", this);
  m_opacityValueLabel = new QLabel("100%", this);
  m_opacityValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  layout->addWidget(opacityLabel, row, 0);
  layout->addWidget(m_opacityValueLabel, row, 1);
  ++row;

  m_opacitySlider = new QSlider(Qt::Horizontal, this);
  m_opacitySlider->setRange(0, 100);
  layout->addWidget(m_opacitySlider, row++, 0, 1, 2);
  layout->setRowStretch(row, 1);

  connect(m_sizeSlider, &QSlider::valueChanged, this, &ToolPanel::onSizeSliderChanged);
  connect(m_opacitySlider, &QSlider::valueChanged, this, &ToolPanel::onOpacitySliderChanged);
}

} // namespace app::panels
