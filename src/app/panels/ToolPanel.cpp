#include "app/panels/ToolPanel.h"

#include <algorithm>
#include <vector>

#include <QGridLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>

#include "app/bridge/AppController.h"
#include "app/ui/IconLoader.h"

namespace app::panels {

namespace {

QString toolNameJa(core::ToolKind kind) {
  switch (kind) {
    case core::ToolKind::Brush:
      return QStringLiteral("ブラシ");
    case core::ToolKind::Eraser:
      return QStringLiteral("消しゴム");
    case core::ToolKind::Eyedropper:
      return QStringLiteral("スポイト");
    case core::ToolKind::Fill:
      return QStringLiteral("塗りつぶし");
    case core::ToolKind::Line:
      return QStringLiteral("直線");
    case core::ToolKind::RectSelection:
      return QStringLiteral("選択");
    case core::ToolKind::MoveLayer:
      return QStringLiteral("移動");
    case core::ToolKind::Hand:
      return QStringLiteral("手のひら");
    case core::ToolKind::Zoom:
      return QStringLiteral("ズーム");
    default:
      return QStringLiteral("ツール");
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

QString iconName(core::ToolKind kind) {
  switch (kind) {
    case core::ToolKind::Brush:
      return "brush";
    case core::ToolKind::Eraser:
      return "eraser";
    case core::ToolKind::Eyedropper:
      return "eyedropper";
    case core::ToolKind::Fill:
      return "fill";
    case core::ToolKind::Line:
      return "line";
    case core::ToolKind::RectSelection:
      return "select";
    case core::ToolKind::MoveLayer:
      return "move";
    case core::ToolKind::Hand:
      return "hand";
    case core::ToolKind::Zoom:
      return "zoom";
    default:
      return "brush";
  }
}

QWidget* makeQuickSliderBlock(
    QWidget* parent,
    const QString& labelText,
    const QString& swatchStyle,
    const QString& grooveColor,
    const QString& fillColor,
    const QString& emptyColor,
    QSlider*& sliderOut,
    QLabel*& valueLabelOut,
    int min,
    int max) {
  auto* block = new QWidget(parent);
  block->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  auto* blockLayout = new QVBoxLayout(block);
  blockLayout->setContentsMargins(0, 0, 0, 0);
  blockLayout->setSpacing(2);

  auto* top = new QHBoxLayout();
  top->setContentsMargins(0, 0, 0, 0);
  top->setSpacing(2);
  auto* swatch = new QFrame(block);
  swatch->setFixedSize(10, 10);
  swatch->setStyleSheet(swatchStyle);
  auto* label = new QLabel(labelText, block);
  label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  label->setStyleSheet("font-size: 10px; color: #b7c2d3;");
  valueLabelOut = new QLabel("0", block);
  valueLabelOut->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  valueLabelOut->setMinimumWidth(20);
  valueLabelOut->setStyleSheet("font-size: 10px; color: #dbe4f3;");
  top->addWidget(swatch, 0, Qt::AlignVCenter);
  top->addWidget(label);
  top->addWidget(valueLabelOut, 1);

  sliderOut = new QSlider(Qt::Vertical, block);
  sliderOut->setRange(min, max);
  sliderOut->setInvertedAppearance(true);
  sliderOut->setInvertedControls(false);
  sliderOut->setFixedWidth(8);
  sliderOut->setMinimumHeight(120);
  sliderOut->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  sliderOut->setFocusPolicy(Qt::StrongFocus);
  sliderOut->setStyleSheet(
      QString(
          "QSlider::groove:vertical { background: %1; border: 1px solid #343d4d; width: 4px; border-radius: 2px; }"
          "QSlider::sub-page:vertical { background: %2; border-radius: 2px; }"
          "QSlider::add-page:vertical { background: %3; border-radius: 2px; }"
          "QSlider::handle:vertical { background: #edf2fb; height: 8px; margin: 0 -4px; border-radius: 4px; border: 1px solid rgba(0,0,0,0.25); }")
          .arg(grooveColor, emptyColor, fillColor));

  auto* sliderHolder = new QHBoxLayout();
  sliderHolder->setContentsMargins(0, 2, 0, 2);
  sliderHolder->setSpacing(0);
  sliderHolder->addStretch(1);
  sliderHolder->addWidget(sliderOut);
  sliderHolder->addStretch(1);

  blockLayout->addLayout(top);
  blockLayout->addLayout(sliderHolder, 1);
  return block;
}

} // namespace

ToolPanel::ToolPanel(QWidget* parent)
    : QWidget(parent),
      m_buttonGridHost(new QWidget(this)),
      m_buttonGrid(new QGridLayout()),
      m_quickHost(new QWidget(this)),
      m_rootLayout(new QVBoxLayout(this)) {
  setStyleSheet(
      "QToolButton {"
      "  border: 1px solid #394251;"
      "  background: #242a32;"
      "  color: #d8deea;"
      "  border-radius: 3px;"
      "  padding: 0;"
      "  min-width: 30px;"
      "  min-height: 30px;"
      "}"
      "QToolButton:hover { background: #303846; border-color: #6f8fb7; }"
      "QToolButton:checked { background: #2d4f74; border-color: #93c1f3; color: #ffffff; }"
      "QToolButton:disabled { background: #1f232b; border-color: #2d3441; color: #6d7888; }"
      "QLabel { color: #c9d2df; }");

  m_rootLayout->setContentsMargins(1, 1, 1, 1);
  m_rootLayout->setSpacing(2);

  m_buttonGrid->setContentsMargins(0, 0, 0, 0);
  m_buttonGrid->setHorizontalSpacing(1);
  m_buttonGrid->setVerticalSpacing(1);
  m_buttonGridHost->setLayout(m_buttonGrid);
  m_rootLayout->addWidget(m_buttonGridHost);

  auto* quickWrap = new QHBoxLayout(m_quickHost);
  quickWrap->setContentsMargins(0, 2, 0, 2);
  quickWrap->setSpacing(2);
  quickWrap->addWidget(
      makeQuickSliderBlock(
          this,
          "px",
          "QFrame { background: #79a8f5; border: 1px solid #9ec2ff; border-radius: 2px; }",
          "#161b23",
          "#5f8fda",
          "#2e3540",
          m_sizeSlider,
          m_sizeValueLabel,
          1,
          128));
  quickWrap->addWidget(
      makeQuickSliderBlock(
          this,
          "%",
          "QFrame { background: #9098a6; border: 1px solid #b6becd; border-radius: 2px; }",
          "#161b23",
          "#8d96a3",
          "#2e3540",
          m_opacitySlider,
          m_opacityValueLabel,
          0,
          100));
  m_quickHost->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  m_rootLayout->addWidget(m_quickHost, 1, Qt::AlignHCenter);

  connect(m_sizeSlider, &QSlider::valueChanged, this, &ToolPanel::onSizeSliderChanged);
  connect(m_opacitySlider, &QSlider::valueChanged, this, &ToolPanel::onOpacitySliderChanged);
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

void ToolPanel::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  if ((m_sections & ButtonsOnly) == 0U) {
    return;
  }
  relayoutButtons();
}

void ToolPanel::setSections(Sections sections) noexcept {
  m_sections = sections;
  const bool showButtons = (m_sections & ButtonsOnly) != 0U;
  const bool showQuick = (m_sections & QuickSlidersOnly) != 0U;
  m_buttonGridHost->setVisible(showButtons);
  m_quickHost->setVisible(showQuick);
  if (showButtons) {
    relayoutButtons();
  }
}

int ToolPanel::columnCountForWidth(int width) const noexcept {
  if (width < 60) {
    return 1;
  }
  if (width < 100) {
    return 2;
  }
  if (width < 160) {
    return 3;
  }
  return 4;
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
    QString tip = QString("%1 [%2]").arg(toolNameJa(kind), toolShortcut(kind));
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
  if (m_buttonGrid == nullptr || m_controller == nullptr) {
    return;
  }

  for (const auto& [kind, button] : m_buttons) {
    Q_UNUSED(kind);
    m_buttonGrid->removeWidget(button);
    button->deleteLater();
  }
  m_buttons.clear();
  m_buttonOrder.clear();

  while (m_buttonGrid->count() > 0) {
    QLayoutItem* item = m_buttonGrid->takeAt(0);
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

  for (core::ToolKind kind : ordered) {
    if (std::find(available.begin(), available.end(), kind) == available.end()) {
      continue;
    }
    auto* button = new QToolButton(this);
    button->setCheckable(true);
    button->setAutoExclusive(true);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setIcon(app::ui::icon(iconName(kind)));
    button->setIconSize(QSize(18, 18));
    button->setFixedSize(32, 32);
    button->setProperty("toolKind", static_cast<int>(kind));
    connect(button, &QToolButton::clicked, this, &ToolPanel::onToolButtonClicked);
    m_buttons[kind] = button;
    m_buttonOrder.push_back(button);
  }

  relayoutButtons();
}

void ToolPanel::relayoutButtons() {
  if (m_buttonGrid == nullptr) {
    return;
  }
  while (m_buttonGrid->count() > 0) {
    QLayoutItem* item = m_buttonGrid->takeAt(0);
    delete item;
  }

  const int columns = columnCountForWidth(m_buttonGridHost->width());
  for (int i = 0; i < static_cast<int>(m_buttonOrder.size()); ++i) {
    const int row = i / columns;
    const int col = i % columns;
    m_buttonGrid->addWidget(m_buttonOrder[static_cast<std::size_t>(i)], row, col);
  }
  for (int c = 0; c < columns; ++c) {
    m_buttonGrid->setColumnStretch(c, 1);
  }
}

} // namespace app::panels
