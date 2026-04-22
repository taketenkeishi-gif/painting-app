#include "app/panels/ToolPanel.h"

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

} // namespace

ToolPanel::ToolPanel(QWidget* parent)
    : QWidget(parent) {
  setStyleSheet(
      "QToolButton {"
      "  border: 1px solid #445163;"
      "  background: #29303a;"
      "  color: #d8d8d8;"
      "  border-radius: 4px;"
      "  padding: 6px 6px;"
      "  min-height: 54px;"
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
    button->setChecked(kind == current);
    button->setToolTip(
        enabled
            ? QString("%1 ツール").arg(toolNameJa(kind))
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

  int index = 0;
  for (const core::ToolKind kind : m_controller->availableTools()) {
    auto* button = new QToolButton(this);
    button->setText(toolNameJa(kind));
    button->setCheckable(true);
    button->setAutoExclusive(true);
    button->setIconSize(QSize(16, 16));
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setMinimumSize(QSize(88, 54));
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
    layout->addWidget(button, (index / 2) + 1, index % 2);
    m_buttons[kind] = button;
    ++index;
  }
}

} // namespace app::panels
