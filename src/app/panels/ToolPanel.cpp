#include "app/panels/ToolPanel.h"

#include <algorithm>
#include <vector>

#include <QSignalBlocker>
#include <QToolButton>
#include <QGridLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QStyle>

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

QString toolShortcutJa(core::ToolKind kind) {
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

ToolCategory categoryForTool(core::ToolKind kind) {
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

QString categoryNameJa(ToolCategory category) {
  switch (category) {
    case ToolCategory::Paint:
      return "描画ツール";
    case ToolCategory::Select:
      return "選択・編集";
    case ToolCategory::View:
      return "表示・移動";
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
      "  padding: 5px 5px;"
      "  min-height: 50px;"
      "}"
      "QToolButton:hover { background: #36404c; border-color: #8aa8cf; }"
      "QToolButton:checked { background: #2d527f; border-color: #8fc2ff; color: #ffffff; }"
      "QToolButton:disabled { background: #20252d; border-color: #303845; color: #7c8798; }");
  auto* layout = new QGridLayout(this);
  layout->setContentsMargins(6, 6, 6, 6);
  layout->setSpacing(5);
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
    const QString shortcut = toolShortcutJa(kind);
    button->setChecked(kind == current);
    button->setToolTip(
        enabled
            ? QString("%1 ツール（%2）").arg(toolNameJa(kind), shortcut)
            : QString("%1ツールは現在のレイヤー（%2）では使用できません").arg(toolNameJa(kind), layerKind));
    button->setEnabled(enabled);
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
    delete item;
  }

  auto* title = new QLabel("ツール", this);
  title->setStyleSheet("font-weight:700; color:#dfe6f2; padding:2px 2px;");
  layout->addWidget(title, 0, 0, 1, 2);

  const std::vector<core::ToolKind> orderedTools {
      core::ToolKind::Brush,
      core::ToolKind::Eraser,
      core::ToolKind::Eyedropper,
      core::ToolKind::Fill,
      core::ToolKind::Line,
      core::ToolKind::RectSelection,
      core::ToolKind::MoveLayer,
      core::ToolKind::Hand,
      core::ToolKind::Zoom};
  const std::vector<ToolCategory> orderedCategories {
      ToolCategory::Paint,
      ToolCategory::Select,
      ToolCategory::View};
  const auto availableTools = m_controller->availableTools();

  int row = 1;
  for (const ToolCategory category : orderedCategories) {
    std::vector<core::ToolKind> toolsInCategory;
    for (const core::ToolKind kind : orderedTools) {
      if (categoryForTool(kind) != category) {
        continue;
      }
      if (std::find(availableTools.begin(), availableTools.end(), kind) == availableTools.end()) {
        continue;
      }
      toolsInCategory.push_back(kind);
    }
    if (toolsInCategory.empty()) {
      continue;
    }

    auto* categoryLabel = new QLabel(categoryNameJa(category), this);
    categoryLabel->setStyleSheet("font-weight:600; color:#9fb4cf; padding:2px 1px;");
    layout->addWidget(categoryLabel, row++, 0, 1, 2);

    for (std::size_t i = 0; i < toolsInCategory.size(); ++i) {
      const core::ToolKind kind = toolsInCategory[i];
      auto* button = new QToolButton(this);
      const QString shortcut = toolShortcutJa(kind);
      button->setText(shortcut.isEmpty() ? toolNameJa(kind) : QString("%1\n[%2]").arg(toolNameJa(kind), shortcut));
      button->setCheckable(true);
      button->setAutoExclusive(true);
      button->setIconSize(QSize(16, 16));
      button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
      button->setMinimumSize(QSize(88, 50));
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
          button->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
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
    row += static_cast<int>((toolsInCategory.size() + 1) / 2);
  }
}

} // namespace app::panels
