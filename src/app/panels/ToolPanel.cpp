#include "app/panels/ToolPanel.h"

#include <algorithm>
#include <vector>

#include <QGridLayout>
#include <QFrame>
#include <QScrollArea>
#include <QColor>
#include <QLinearGradient>
#include <QPainter>
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
    case core::ToolKind::Gradient:
      return QStringLiteral("グラデーション");
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
    case core::ToolKind::Gradient:
      return {};  // ショートカットなし（MainMenu で登録）
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
    case core::ToolKind::Gradient:
      return "gradient";
    default:
      return "brush";
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
      "QFrame { border: 1px solid #2a2e3e; border-radius: 3px; background: #272c3c; }");
  auto* chipLayout = new QVBoxLayout(chipFrameOut);
  chipLayout->setContentsMargins(0, 0, 0, 0);
  chipLayout->setSpacing(0);
  valueLabelOut = new QLabel("0", block);
  valueLabelOut->setAlignment(Qt::AlignCenter);
  valueLabelOut->setStyleSheet("font-size: 11px; font-weight: 700; color: #c5cde0; padding: 1px 3px;");
  chipLayout->addWidget(valueLabelOut);

  unitLabelOut = new QLabel(unitText, block);
  unitLabelOut->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
  unitLabelOut->setStyleSheet("font-size: 9px; font-weight: 600; color: #5a6480; padding-top: 0px;");

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
      "  border: 1px solid transparent;"
      "  background: transparent;"
      "  color: #7a86a3;"
      "  border-radius: 5px;"
      "  padding: 0;"
      "  min-width: 30px;"
      "  min-height: 30px;"
      "}"
      "QToolButton:hover { background: #272c3c; border-color: #363d54; color: #c5cde0; }"
      "QToolButton:checked { background: #1d3a7a; border-color: #4e8ef7; color: #edf0f9; }"
      "QToolButton:disabled { background: transparent; color: #4a5268; }"
      "QLabel { color: #7a86a3; }");

  m_rootLayout->setContentsMargins(0, 0, 0, 0);
  m_rootLayout->setSpacing(1);

  m_buttonGrid->setContentsMargins(4, 4, 4, 4);
  m_buttonGrid->setHorizontalSpacing(2);
  m_buttonGrid->setVerticalSpacing(2);
  m_buttonGrid->setAlignment(Qt::AlignLeft | Qt::AlignTop);

  // Host widget carries the grid layout.
  // minimumWidth = 0 so the host can be as narrow as the viewport.
  // Height is NOT forced to 0 — it stays at natural grid height so the
  // scroll area can show a scrollbar when buttons overflow vertically.
  m_buttonGridHost->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  m_buttonGridHost->setMinimumWidth(0);
  m_buttonGridHost->setStyleSheet(QStringLiteral("background: #1a1d27;"));
  m_buttonGridHost->setLayout(m_buttonGrid);

  // ── Internal button scroll area ───────────────────────────────────────────
  // Vertical scroll ON (buttons overflow downward when dock is narrow).
  // Horizontal scroll OFF (never clip-scroll sideways — columns shrink instead).
  // setWidgetResizable(true) makes m_buttonGridHost width track the viewport
  // width, which is exactly what columnCountForWidth() reads via viewport().
  m_buttonScrollArea = new QScrollArea(this);
  m_buttonScrollArea->setFrameShape(QFrame::NoFrame);
  m_buttonScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_buttonScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_buttonScrollArea->setWidgetResizable(true);
  m_buttonScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_buttonScrollArea->setWidget(m_buttonGridHost);

  // ToolPanel itself can be as narrow as needed; height fills whatever the
  // dock gives it (the internal scroll area handles button overflow).
  setMinimumWidth(0);

  m_rootLayout->addWidget(m_buttonScrollArea, 1);

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
  m_buttonScrollArea->setVisible(showButtons);
  m_quickHost->setVisible(showQuick);
  if (showButtons) {
    relayoutButtons();
  }
}

int ToolPanel::columnCountForWidth(int width) const noexcept {
  // Formula-based: how many 28px buttons (+ 2px gap each) fit in the usable
  // width after subtracting the 4px margin on each side (8px total)?
  constexpr int kBtn     = 28;
  constexpr int kGap     = 2;
  constexpr int kMargins = 8;   // 4px left + 4px right
  const int usable = width - kMargins;
  if (usable <= 0) return 1;
  // cols = floor((usable + gap) / (btn + gap))
  return std::max(1, (usable + kGap) / (kBtn + kGap));
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
        QStringLiteral("QFrame { border: 1px solid #363d54; border-radius: 4px; background: #272c3c; }"));
  }
  if (m_sizeValueLabel != nullptr) {
    m_sizeValueLabel->setStyleSheet(
        QStringLiteral("font-size: 11px; font-weight: 700; color: #edf0f9; padding: 1px 3px;"));
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
      core::ToolKind::Gradient,
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
    button->setIconSize(QSize(16, 16));
    button->setFixedSize(28, 28);
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
    if (item != nullptr && item->widget() != nullptr && item->widget()->property("toolSeparator").toBool()) {
      item->widget()->deleteLater();
    }
    delete item;
  }

  // Read the actual viewport width — setWidgetResizable(true) on the internal
  // scroll area keeps m_buttonGridHost width = viewport width, so this is the
  // authoritative "how wide are the buttons allowed to be" value.
  const int availableWidth = (m_buttonScrollArea && m_buttonScrollArea->viewport())
                             ? m_buttonScrollArea->viewport()->width()
                             : width();
  const int columns = std::max(1, columnCountForWidth(availableWidth));

  // Place buttons: top-left packed, no centering, no stretch.
  for (int i = 0; i < static_cast<int>(m_buttonOrder.size()); ++i) {
    const int logicalRow = i / columns;
    const int row = logicalRow * 2;   // even rows = buttons; odd rows = separators
    const int col = i % columns;
    m_buttonGrid->addWidget(
        m_buttonOrder[static_cast<std::size_t>(i)],
        row, col,
        Qt::AlignLeft | Qt::AlignTop);
  }

  // Separator lines between button rows (thin 1px dividers).
  const int rowCount = (static_cast<int>(m_buttonOrder.size()) + columns - 1) / columns;
  for (int r = 0; r < rowCount - 1; ++r) {
    auto* separator = new QFrame(m_buttonGridHost);
    separator->setProperty("toolSeparator", true);
    separator->setFixedHeight(1);
    separator->setMinimumHeight(1);
    separator->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    separator->setFrameShape(QFrame::NoFrame);
    separator->setStyleSheet(QStringLiteral("background: #2a2e3e; border: none;"));
    m_buttonGrid->addWidget(separator, r * 2 + 1, 0, 1, columns);
  }

  // No column or row stretch — buttons stay left-top, not distributed.
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
  // Bottom spacer row pushes buttons to the top.
  m_buttonGrid->setRowStretch(rowCount * 2, 1);

  m_buttonGridHost->setMinimumWidth(0);
  m_buttonGridHost->setMaximumWidth(QWIDGETSIZE_MAX);
}

} // namespace app::panels




