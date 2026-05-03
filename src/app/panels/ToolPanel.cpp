#include "app/panels/ToolPanel.h"

#include <algorithm>
#include <vector>

#include <QGridLayout>
#include <QFrame>
#include <QColor>
#include <QEvent>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QStyle>
#include <QStyleOptionSlider>
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
#include "features/requested_tools/RequestedToolsPanel.h"

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
    case core::ToolKind::Pen:
      return "P";
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
    case core::ToolKind::Pen:
      return "pen_active";
    default:
      return "brush";
  }
}

QString defaultCoreSubToolId(core::ToolKind kind) {
  switch (kind) {
    case core::ToolKind::Brush:
      return QStringLiteral("brush_normal");
    case core::ToolKind::Eraser:
      return QStringLiteral("eraser_normal");
    case core::ToolKind::Eyedropper:
      return QStringLiteral("eyedropper_default");
    case core::ToolKind::Fill:
      return QStringLiteral("fill_contiguous");
    case core::ToolKind::Line:
      return QStringLiteral("line_raster");
    case core::ToolKind::RectSelection:
      return QStringLiteral("rect_default");
    case core::ToolKind::MoveLayer:
      return QStringLiteral("move_layer_default");
    case core::ToolKind::Hand:
      return QStringLiteral("hand_default");
    case core::ToolKind::Zoom:
      return QStringLiteral("zoom_default");
    case core::ToolKind::Pen:
      return QStringLiteral("pen_default");
    default:
      return {};
  }
}

QColor toQColor(const core::Color& color) {
  return QColor(color.r, color.g, color.b, color.a);
}

QString contrastTextColor(const QColor& base) {
  const double luminance = (0.2126 * base.redF()) + (0.7152 * base.greenF()) + (0.0722 * base.blueF());
  return luminance > 0.45 ? QStringLiteral("#101318") : QStringLiteral("#f6f9ff");
}


class NeutralValueSlider final : public QSlider {
public:
  explicit NeutralValueSlider(Qt::Orientation orientation, QWidget* parent = nullptr)
      : QSlider(orientation, parent) {}

protected:
  void paintEvent(QPaintEvent*) override {
    QStyleOptionSlider opt;
    initStyleOption(&opt);

    QRect track = rect().adjusted(width() / 2 - 4, 8, -(width() / 2 - 4), -8);
    track.setWidth(8);
    track.moveCenter(QPoint(width() / 2, track.center().y()));

    const double range = double(maximum() - minimum());
    const double ratio = range <= 0.0 ? 0.0 : double(value() - minimum()) / range;
    const int handleY = track.bottom() - int(ratio * track.height());

    QRect fillRect(track.left() + 1, handleY, track.width() - 2, track.bottom() - handleY);
    fillRect = fillRect.normalized().intersected(track.adjusted(1, 1, -1, -1));

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(QColor(70, 80, 96));
    painter.setBrush(QColor(31, 37, 46));
    painter.drawRoundedRect(track.adjusted(0, 0, -1, -1), 4, 4);

    if (fillRect.isValid()) {
      QLinearGradient gradient(fillRect.bottomLeft(), fillRect.topLeft());
      gradient.setColorAt(0.0, QColor(92, 100, 114));
      gradient.setColorAt(1.0, QColor(138, 148, 164));
      painter.fillRect(fillRect, gradient);
    }

    QRect handle(0, handleY - 8, 18, 16);
    handle.moveCenter(QPoint(width() / 2, handle.center().y()));

    painter.setPen(QColor(244, 247, 252));
    painter.setBrush(QColor(238, 243, 251));
    painter.drawRoundedRect(handle.adjusted(1, 1, -1, -1), 5, 5);

    painter.setPen(QColor(84, 96, 116));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(handle.adjusted(0, 0, -1, -1), 5, 5);
  }
};

class AlphaPreviewSlider final : public QSlider {
public:
  explicit AlphaPreviewSlider(Qt::Orientation orientation, QWidget* parent = nullptr)
      : QSlider(orientation, parent) {}

  void setPreviewColor(const QColor& color) {
    QColor next = color;
    if (!next.isValid()) {
      next = QColor(120, 166, 235);
    }
    next.setAlpha(255);
    if (m_previewColor == next) {
      return;
    }
    m_previewColor = next;
    update();
  }

protected:
  void paintEvent(QPaintEvent*) override {
    QStyleOptionSlider opt;
    initStyleOption(&opt);

    QRect track = rect().adjusted(width() / 2 - 4, 8, -(width() / 2 - 4), -8);
    track.setWidth(8);
    track.moveCenter(QPoint(width() / 2, track.center().y()));

    const double range = double(maximum() - minimum());
    const double ratio = range <= 0.0 ? 0.0 : double(value() - minimum()) / range;
    const int handleY = track.bottom() - int(ratio * track.height());

    QRect fillRect(track.left() + 1, handleY, track.width() - 2, track.bottom() - handleY);
    fillRect = fillRect.normalized().intersected(track.adjusted(1, 1, -1, -1));

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(QColor(70, 80, 96));
    painter.setBrush(QColor(31, 37, 46));
    painter.drawRoundedRect(track.adjusted(0, 0, -1, -1), 4, 4);

    if (fillRect.isValid()) {
      painter.save();
      painter.setClipRect(fillRect);

      const int cell = 4;
      const QColor checkA(188, 194, 204);
      const QColor checkB(92, 100, 114);
      for (int y = fillRect.top(); y <= fillRect.bottom(); y += cell) {
        for (int x = fillRect.left(); x <= fillRect.right(); x += cell) {
          const bool useA = ((x / cell) + (y / cell)) % 2 == 0;
          painter.fillRect(QRect(x, y, cell, cell).intersected(fillRect), useA ? checkA : checkB);
        }
      }

      QColor top = m_previewColor;
      top.setAlpha(215);
      QColor bottom(80, 86, 98, 225);
      QLinearGradient gradient(fillRect.bottomLeft(), fillRect.topLeft());
      gradient.setColorAt(0.0, bottom);
      gradient.setColorAt(1.0, top);
      painter.fillRect(fillRect, gradient);

      painter.restore();
    }

    QRect handle(0, handleY - 8, 18, 16);
    handle.moveCenter(QPoint(width() / 2, handle.center().y()));

    painter.setPen(QColor(244, 247, 252));
    painter.setBrush(QColor(238, 243, 251));
    painter.drawRoundedRect(handle.adjusted(1, 1, -1, -1), 5, 5);

    painter.setPen(QColor(84, 96, 116));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(handle.adjusted(0, 0, -1, -1), 5, 5);
  }

private:
  QColor m_previewColor {120, 166, 235};
};



QString sliderStyle(const QColor& accent, bool opacityMode) {
  QColor solid = accent.isValid() ? accent : QColor(120, 166, 235);
  if (solid.alpha() == 0) {
    solid = QColor(120, 166, 235);
  }
  solid.setAlpha(255);

  const QString handleBorder = opacityMode
      ? solid.lighter(125).name(QColor::HexRgb)
      : QStringLiteral("#667184");

  return QStringLiteral(
             "QSlider::groove:vertical {"
             "  width: 8px;"
             "  border: 0px;"
             "  background: transparent;"
             "}"
             "QSlider::sub-page:vertical {"
             "  background: transparent;"
             "  border: 0px;"
             "}"
             "QSlider::add-page:vertical {"
             "  background: transparent;"
             "  border: 0px;"
             "}"
             "QSlider::handle:vertical {"
             "  width: 18px;"
             "  height: 16px;"
             "  margin: -2px -6px;"
             "  border-radius: 5px;"
             "  border: 1px solid %1;"
             "  background: #eef3fb;"
             "}")
      .arg(handleBorder);
}

QWidget* makeQuickSliderBlock(
    QWidget* parent,
    const QString& unitText,
    bool opacityMode,
    QSlider*& sliderOut,
    QLabel*& valueLabelOut,
    QLabel*& unitLabelOut,
    QFrame*& chipFrameOut,
    int min,
    int max) {
  auto* block = new QWidget(parent);
  block->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  block->setMinimumWidth(32);
  auto* blockLayout = new QVBoxLayout(block);
  blockLayout->setContentsMargins(0, 0, 0, 0);
  blockLayout->setSpacing(1);

  chipFrameOut = new QFrame(block);
  chipFrameOut->setFixedSize(30, 20);
  chipFrameOut->setStyleSheet(
      "QFrame { border: 1px solid #5d687a; border-radius: 4px; background: #49648b; }");
  auto* chipLayout = new QVBoxLayout(chipFrameOut);
  chipLayout->setContentsMargins(0, 0, 0, 0);
  chipLayout->setSpacing(0);
  valueLabelOut = new QLabel("0", block);
  valueLabelOut->setAlignment(Qt::AlignCenter);
  valueLabelOut->setStyleSheet("font-size: 11px; font-weight: 700; color: #f6f9ff; padding: 1px 3px;");
  chipLayout->addWidget(valueLabelOut);

  unitLabelOut = new QLabel(unitText, block);
  unitLabelOut->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
  unitLabelOut->setStyleSheet("font-size: 9px; font-weight: 600; color: #97a4ba; padding-top: 0px;");

  auto* header = new QVBoxLayout();
  header->setContentsMargins(0, 0, 0, 0);
  header->setSpacing(0);
  header->addWidget(chipFrameOut, 0, Qt::AlignHCenter);
  header->addWidget(unitLabelOut, 0, Qt::AlignHCenter);

  sliderOut = opacityMode ? static_cast<QSlider*>(new AlphaPreviewSlider(Qt::Vertical, block)) : static_cast<QSlider*>(new NeutralValueSlider(Qt::Vertical, block));
  sliderOut->setRange(min, max);
  sliderOut->setInvertedAppearance(false);
  sliderOut->setInvertedControls(false);
  sliderOut->setFixedWidth(12);
  sliderOut->setMinimumHeight(120);
  sliderOut->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  sliderOut->setFocusPolicy(Qt::StrongFocus);
  sliderOut->setStyleSheet(sliderStyle(QColor(120, 166, 235), opacityMode));

  auto* sliderHolder = new QHBoxLayout();
  sliderHolder->setContentsMargins(0, 10, 0, 12);
  sliderHolder->setSpacing(0);
  sliderHolder->addStretch(1);
  sliderHolder->addWidget(sliderOut);
  sliderHolder->addStretch(1);

  blockLayout->addLayout(header);
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
      "  min-width: 28px;"
      "  min-height: 28px;"
      "}"
      "QToolButton:hover { background: #303846; border-color: #6f8fb7; }"
      "QToolButton:checked { background: #2d4f74; border-color: #93c1f3; color: #ffffff; }"
      "QToolButton:disabled { background: #1f232b; border-color: #2d3441; color: #6d7888; }"
      "QLabel { color: #c9d2df; }");

  m_rootLayout->setContentsMargins(0, 0, 0, 0);
  m_rootLayout->setSpacing(1);

  m_buttonGrid->setContentsMargins(0, 0, 0, 0);
  m_buttonGrid->setHorizontalSpacing(2);
  m_buttonGrid->setVerticalSpacing(1);
  m_buttonGrid->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  m_buttonGridHost->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
  m_buttonGridHost->setStyleSheet(QStringLiteral("background: #3a4658;"));
  m_buttonGridHost->setLayout(m_buttonGrid);
  m_rootLayout->addWidget(m_buttonGridHost, 0, Qt::AlignLeft | Qt::AlignTop);

  auto* quickWrap = new QHBoxLayout(m_quickHost);
  quickWrap->setContentsMargins(0, 3, 0, 4);
  quickWrap->setSpacing(1);
  quickWrap->addWidget(
      makeQuickSliderBlock(
          this,
          "px",
          false,
          m_sizeSlider,
          m_sizeValueLabel,
          m_sizeUnitLabel,
          m_sizeChip,
          1,
          128));
  quickWrap->addWidget(
      makeQuickSliderBlock(
          this,
          "%",
          true,
          m_opacitySlider,
          m_opacityValueLabel,
          m_opacityUnitLabel,
          m_opacityChip,
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
  const QString currentSubToolId = QString::fromStdString(m_controller->currentSubToolId());
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

  for (QToolButton* button : m_buttonOrder) {
    if (button == nullptr || !button->property("requestedToolPlaceholder").toBool()) {
      continue;
    }

    const QSignalBlocker blocker(button);
    const core::ToolKind kind = static_cast<core::ToolKind>(button->property("toolKind").toInt());
    const QString subToolId = button->property("subToolId").toString();
    const bool enabled = m_controller->canUseToolOnActiveLayer(kind);
    const bool active = (kind == current) && !subToolId.isEmpty() && (subToolId == currentSubToolId);
    button->setEnabled(enabled);
    button->setChecked(active);
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
    updateQuickSliderVisuals(toQColor(state.color));
    const bool sizeEnabled = m_controller->currentToolSupportsSize();
    const bool opacityEnabled = m_controller->currentToolSupportsOpacity();
    m_sizeSlider->setEnabled(sizeEnabled);
    m_opacitySlider->setEnabled(opacityEnabled);
    m_sizeValueLabel->setEnabled(sizeEnabled);
    m_opacityValueLabel->setEnabled(opacityEnabled);
    if (m_sizeChip != nullptr) {
      m_sizeChip->setEnabled(sizeEnabled);
    }
    if (m_opacityChip != nullptr) {
      m_opacityChip->setEnabled(opacityEnabled);
    }
    if (m_sizeUnitLabel != nullptr) {
      m_sizeUnitLabel->setEnabled(sizeEnabled);
    }
    if (m_opacityUnitLabel != nullptr) {
      m_opacityUnitLabel->setEnabled(opacityEnabled);
    }
    m_refreshingSliders = false;
  }
}

void ToolPanel::updateQuickSliderVisuals(const QColor& color) {
  const QColor accent = color;
  const QString textColor = contrastTextColor(accent);
  const QColor sizeAccent = accent.alpha() == 0 ? QColor(104, 142, 204) : accent;
  const QColor sizeBorder = sizeAccent.lighter(125);

  if (m_sizeChip != nullptr) {
    m_sizeChip->setStyleSheet(
        QStringLiteral("QFrame { border: 1px solid #5f697a; border-radius: 4px; background: #47505f; }"));
  }
  if (m_sizeValueLabel != nullptr) {
    m_sizeValueLabel->setStyleSheet(
        QStringLiteral("font-size: 11px; font-weight: 700; color: #f3f6fb; padding: 1px 3px;"));
  }
  if (m_sizeSlider != nullptr) {
    m_sizeSlider->setStyleSheet(sliderStyle(QColor(120, 126, 136), false));
  }

  if (m_opacityChip != nullptr) {
    m_opacityChip->setStyleSheet(
        QStringLiteral(
            "QFrame { border: 1px solid %1; border-radius: 4px; "
            "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #2c3340, stop:0.5 %2, stop:1 #2c3340); }")
            .arg(sizeBorder.name(QColor::HexRgb), sizeAccent.name(QColor::HexRgb)));
  }
  if (m_opacityValueLabel != nullptr) {
    m_opacityValueLabel->setStyleSheet(
        QStringLiteral("font-size: 11px; font-weight: 700; color: %1; padding: 1px 3px;").arg(textColor));
  }
  if (m_opacitySlider != nullptr) {
    if (auto* alphaSlider = dynamic_cast<AlphaPreviewSlider*>(m_opacitySlider)) {
      alphaSlider->setPreviewColor(sizeAccent);
    }
    m_opacitySlider->setStyleSheet(sliderStyle(sizeAccent, true));
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
  if (!m_controller->setCurrentTool(kind)) {
    return;
  }

  const QString subToolId = button->property("subToolId").toString();
  if (!subToolId.isEmpty()) {
    m_controller->setCurrentSubTool(subToolId.toStdString());
    return;
  }

  const QString coreSubToolId = defaultCoreSubToolId(kind);
  if (!coreSubToolId.isEmpty()) {
    m_controller->setCurrentSubTool(coreSubToolId.toStdString());
  }
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
    core::ToolKind::Zoom,
    core::ToolKind::Pen
};
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
    button->setIconSize(QSize(20, 20));
    button->setFixedSize(28, 28);
    button->setProperty("toolKind", static_cast<int>(kind));
    connect(button, &QToolButton::clicked, this, &ToolPanel::onToolButtonClicked);
    button->installEventFilter(this);
    m_buttons[kind] = button;
    m_buttonOrder.push_back(button);
  }

  for (const auto& requestedSpec : ::features::requested_tools::detail::requestedToolSpecs()) {
    auto* requestedButton = new QToolButton(this);
    requestedButton->setObjectName(QString::fromLatin1(requestedSpec.objectName));
    requestedButton->setProperty("requestedToolPlaceholder", true);
    requestedButton->setProperty("toolKind", static_cast<int>(requestedSpec.kind));
    requestedButton->setProperty("subToolId", QString::fromLatin1(requestedSpec.subToolId));
    requestedButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    requestedButton->setIcon(::features::requested_tools::detail::makeRequestedToolIcon(requestedSpec.subToolId));
    requestedButton->setIconSize(QSize(20, 20));
    requestedButton->setFixedSize(28, 28);
    requestedButton->setCheckable(true);
    requestedButton->setAutoExclusive(true);
    const QString requestedLabelJa = ::features::requested_tools::detail::requestedToolJaLabel(requestedSpec.subToolId);
    const QString label = requestedLabelJa.isEmpty() ? QString::fromLatin1(requestedSpec.label) : requestedLabelJa;
    requestedButton->setToolTip(label);
    requestedButton->setStatusTip(label);
    connect(requestedButton, &QToolButton::clicked, this, &ToolPanel::onToolButtonClicked);
    requestedButton->installEventFilter(this);
    m_buttonOrder.push_back(requestedButton);
  }

  loadButtonOrder();
  relayoutButtons();
}

void ToolPanel::relayoutButtons() {
  if (m_buttonGrid == nullptr) {
    return;
  }

  while (m_buttonGrid->count() > 0) {
    QLayoutItem* item = m_buttonGrid->takeAt(0);
    if (item != nullptr && item->widget() != nullptr && item->widget()->property("toolSeparator").toBool()) {
      item->widget()->deleteLater();
    }
    delete item;
  }

  const int availableWidth = std::max(width(), m_buttonGridHost != nullptr ? m_buttonGridHost->width() : 0);
  const int columns = std::max(1, columnCountForWidth(availableWidth));

  for (int i = 0; i < static_cast<int>(m_buttonOrder.size()); ++i) {
    const int logicalRow = i / columns;
    const int row = logicalRow * 2;
    const int col = i % columns;
    m_buttonGrid->addWidget(m_buttonOrder[static_cast<std::size_t>(i)], row, col, Qt::AlignLeft | Qt::AlignTop);
  }

  const int rowCount = (static_cast<int>(m_buttonOrder.size()) + columns - 1) / columns;
  for (int r = 0; r < rowCount - 1; ++r) {
    auto* separator = new QFrame(m_buttonGridHost);
    separator->setProperty("toolSeparator", true);
    separator->setFixedHeight(1);
    separator->setMinimumHeight(1);
    separator->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    separator->setFrameShape(QFrame::NoFrame);
    separator->setStyleSheet(QStringLiteral("background: #536173; border: none;"));
    m_buttonGrid->addWidget(separator, r * 2 + 1, 0, 1, columns);
  }

  for (int c = 0; c < columns; ++c) {
    m_buttonGrid->setColumnStretch(c, 0);
  }
  for (int r = 0; r < rowCount; ++r) {
    m_buttonGrid->setRowStretch(r * 2, 0);
    if (r < rowCount - 1) {
      m_buttonGrid->setRowStretch(r * 2 + 1, 0);
      m_buttonGrid->setRowMinimumHeight(r * 2 + 1, 1);
    }
  }

  const int buttonSize = m_buttonOrder.empty() ? 28 : m_buttonOrder.front()->width();
  const int gridWidth = columns * buttonSize + std::max(0, columns - 1) * 2;
  m_buttonGridHost->setFixedWidth(gridWidth);
  m_buttonGridHost->adjustSize();
}

bool ToolPanel::eventFilter(QObject* watched, QEvent* event) {
  auto* button = qobject_cast<QToolButton*>(watched);
  if (button == nullptr) {
    return QWidget::eventFilter(watched, event);
  }
  if (event->type() == QEvent::MouseButtonPress) {
    auto* mouse = static_cast<QMouseEvent*>(event);
    if (mouse->button() == Qt::LeftButton) {
      m_dragButton = button;
      m_dragStartPos = mouse->pos();
    }
  } else if (event->type() == QEvent::MouseMove) {
    if (m_dragButton == button) {
      auto* mouse = static_cast<QMouseEvent*>(event);
      if ((mouse->pos() - m_dragStartPos).manhattanLength() >= 8) {
        QWidget* targetWidget = childAt(button->mapTo(this, mouse->pos()));
        auto* target = qobject_cast<QToolButton*>(targetWidget);
        if (target != nullptr && target != button) {
          const int from = buttonIndex(button);
          const int to = buttonIndex(target);
          if (from >= 0 && to >= 0 && from != to) {
            moveButton(from, to);
            saveButtonOrder();
            relayoutButtons();
          }
        }
      }
    }
  } else if (event->type() == QEvent::MouseButtonRelease) {
    auto* mouse = static_cast<QMouseEvent*>(event);
    if (mouse->button() == Qt::LeftButton && m_dragButton == button) {
      m_dragButton = nullptr;
    }
  }
  return QWidget::eventFilter(watched, event);
}

int ToolPanel::buttonIndex(QToolButton* button) const {
  for (int i = 0; i < static_cast<int>(m_buttonOrder.size()); ++i) {
    if (m_buttonOrder[static_cast<std::size_t>(i)] == button) {
      return i;
    }
  }
  return -1;
}

void ToolPanel::moveButton(int from, int to) {
  if (from < 0 || to < 0 || from >= static_cast<int>(m_buttonOrder.size()) || to >= static_cast<int>(m_buttonOrder.size())) {
    return;
  }
  QToolButton* moved = m_buttonOrder[static_cast<std::size_t>(from)];
  m_buttonOrder.erase(m_buttonOrder.begin() + from);
  m_buttonOrder.insert(m_buttonOrder.begin() + to, moved);
}

void ToolPanel::saveButtonOrder() const {
  QSettings settings("taketenkeishi", "LayeredPaintApp");
  QStringList ids;
  for (QToolButton* button : m_buttonOrder) {
    if (button == nullptr) {
      continue;
    }
    const QString subToolId = button->property("subToolId").toString();
    if (!subToolId.isEmpty()) {
      ids.push_back(QStringLiteral("sub:") + subToolId);
    } else {
      ids.push_back(QStringLiteral("kind:") + QString::number(button->property("toolKind").toInt()));
    }
  }
  settings.setValue(QStringLiteral("toolPanel/buttonOrder"), ids);
}

void ToolPanel::loadButtonOrder() {
  QSettings settings("taketenkeishi", "LayeredPaintApp");
  const QStringList ids = settings.value(QStringLiteral("toolPanel/buttonOrder")).toStringList();
  if (ids.isEmpty()) {
    return;
  }
  std::vector<QToolButton*> reordered;
  reordered.reserve(m_buttonOrder.size());
  for (const QString& id : ids) {
    for (QToolButton* button : m_buttonOrder) {
      if (button == nullptr) {
        continue;
      }
      const QString subToolId = button->property("subToolId").toString();
      const QString token = !subToolId.isEmpty()
          ? (QStringLiteral("sub:") + subToolId)
          : (QStringLiteral("kind:") + QString::number(button->property("toolKind").toInt()));
      if (token == id && std::find(reordered.begin(), reordered.end(), button) == reordered.end()) {
        reordered.push_back(button);
        break;
      }
    }
  }
  for (QToolButton* button : m_buttonOrder) {
    if (std::find(reordered.begin(), reordered.end(), button) == reordered.end()) {
      reordered.push_back(button);
    }
  }
  m_buttonOrder = std::move(reordered);
}

} // namespace app::panels




