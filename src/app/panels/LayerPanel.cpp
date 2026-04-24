#include "app/panels/LayerPanel.h"

#include <algorithm>
#include <cmath>

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QColor>
#include <QComboBox>
#include <QFont>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QItemDelegate>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QStyledItemDelegate>
#include <QStyleOptionSlider>
#include <QVariant>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QStyle>
#include <QVBoxLayout>
#include <QListWidgetItem>

#include "app/bridge/AppController.h"
#include "app/ui/IconLoader.h"

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
constexpr int kBlendModeRole = Qt::UserRole + 11;
constexpr int kPaperRole = Qt::UserRole + 12;
constexpr int kActiveRole = Qt::UserRole + 13;

constexpr int kLayerRowHeight = 30;
constexpr int kLayerThumbWidth = 38;
constexpr int kLayerThumbHeight = 24;
constexpr int kVisibilitySlotWidth = 20;
constexpr int kActiveSlotWidth = 16;

QString layerKindText(core::LayerKind kind) {
  switch (kind) {
    case core::LayerKind::Raster:
      return QStringLiteral("ラスタ");
    case core::LayerKind::Vector:
      return QStringLiteral("ベクター");
    case core::LayerKind::Folder:
      return QStringLiteral("フォルダー");
    default:
      return QStringLiteral("不明");
  }
}

QString blendModeName(core::BlendMode mode) {
  switch (mode) {
    case core::BlendMode::Multiply:
      return QStringLiteral("乗算");
    case core::BlendMode::Add:
      return QStringLiteral("加算");
    case core::BlendMode::Normal:
    default:
      return QStringLiteral("通常");
  }
}

QColor toQColor(const core::Color& c) {
  return QColor(c.r, c.g, c.b, c.a);
}

QColor alphaBlend(const QColor& under, const core::Color& over) {
  const float a = static_cast<float>(over.a) / 255.0F;
  const float inv = 1.0F - a;
  return QColor(
      static_cast<int>(std::lround(static_cast<float>(over.r) * a + under.redF() * 255.0F * inv)),
      static_cast<int>(std::lround(static_cast<float>(over.g) * a + under.greenF() * 255.0F * inv)),
      static_cast<int>(std::lround(static_cast<float>(over.b) * a + under.blueF() * 255.0F * inv)));
}

QColor checkerColor(int x, int y) {
  const bool dark = (((x / 4) + (y / 4)) % 2) == 0;
  return dark ? QColor(62, 68, 78) : QColor(84, 92, 104);
}

QIcon layerThumbnailIcon(
    const app::bridge::AppController* controller,
    const app::bridge::LayerViewModel& model,
    std::size_t layerIndex) {
  constexpr int thumbW = kLayerThumbWidth;
  constexpr int thumbH = kLayerThumbHeight;
  QImage image(thumbW, thumbH, QImage::Format_ARGB32_Premultiplied);
  for (int y = 0; y < thumbH; ++y) {
    for (int x = 0; x < thumbW; ++x) {
      image.setPixelColor(x, y, checkerColor(x, y));
    }
  }

  if (model.paperLayer) {
    const QColor paper = toQColor(controller->paperColor());
    for (int y = 0; y < thumbH; ++y) {
      for (int x = 0; x < thumbW; ++x) {
        image.setPixelColor(x, y, paper);
      }
    }
  } else if (layerIndex < controller->document().layerCount()) {
    const core::Layer& layer = controller->document().layerAt(layerIndex);
    if (layer.kind() == core::LayerKind::Raster) {
      const core::PixelBuffer& buffer = layer.buffer();
      if (buffer.width() > 0 && buffer.height() > 0) {
        for (int y = 0; y < thumbH; ++y) {
          const int sy = std::clamp((y * buffer.height()) / thumbH, 0, buffer.height() - 1);
          for (int x = 0; x < thumbW; ++x) {
            const int sx = std::clamp((x * buffer.width()) / thumbW, 0, buffer.width() - 1);
            const core::Color src = buffer.pixel(sx, sy);
            image.setPixelColor(x, y, alphaBlend(image.pixelColor(x, y), src));
          }
        }
      }
    } else if (layer.kind() == core::LayerKind::Vector) {
      QPainter painter(&image);
      painter.setRenderHint(QPainter::Antialiasing, true);
      painter.setPen(QPen(QColor(208, 216, 230), 1.2));
      painter.drawLine(QPointF(3.0, thumbH - 4.0), QPointF(thumbW - 3.0, 3.5));
      painter.end();
    } else if (layer.kind() == core::LayerKind::Folder) {
      QPainter painter(&image);
      painter.fillRect(QRect(1, 2, thumbW - 2, thumbH - 4), QColor(102, 118, 142, 80));
      painter.end();
    }
  }

  QPixmap pixmap = QPixmap::fromImage(image);
  QPainter border(&pixmap);
  border.setPen(QPen(QColor(40, 46, 56), 1.0));
  border.setBrush(Qt::NoBrush);
  border.drawRect(QRect(0, 0, thumbW - 1, thumbH - 1));
  border.end();
  return QIcon(pixmap);
}

QString layerBadge(const app::bridge::LayerViewModel& model) {
  if (model.paperLayer) {
    return QStringLiteral("[P]");
  }
  switch (model.kind) {
    case core::LayerKind::Raster:
      return QStringLiteral("[R]");
    case core::LayerKind::Vector:
      return QStringLiteral("[V]");
    case core::LayerKind::Folder:
      return QStringLiteral("[F]");
    default:
      return QStringLiteral("[?]");
  }
}

QString decorateLayerName(const app::bridge::LayerViewModel& model) {
  if (model.paperLayer) {
    return QStringLiteral("用紙");
  }
  QString name = QString::fromStdString(model.name).trimmed();
  if (name.isEmpty()) {
    name = QStringLiteral("レイヤー");
  }
  return QStringLiteral("%1 %2").arg(layerBadge(model), name);
}

QString stripLayerDecorators(const QString& text) {
  QStringList parts = text.trimmed().split(QChar(' '), Qt::SkipEmptyParts);

  const QStringList prefixTokens {
      QStringLiteral("[R]"),
      QStringLiteral("[V]"),
      QStringLiteral("[F]"),
      QStringLiteral("[P]"),
      QStringLiteral("[?]"),
      QStringLiteral("▣"),
      QStringLiteral("◇"),
      QStringLiteral("▤"),
      QStringLiteral("□")};

  const QStringList stateTokens {
      QStringLiteral("非"),
      QStringLiteral("C"),
      QStringLiteral("L"),
      QStringLiteral("α"),
      QStringLiteral("P"),
      QStringLiteral("M"),
      QStringLiteral("M×")};

  while (!parts.isEmpty() && prefixTokens.contains(parts.front())) {
    parts.pop_front();
  }
  while (!parts.isEmpty() && stateTokens.contains(parts.back())) {
    parts.pop_back();
  }

  return parts.join(QChar(' ')).trimmed();
}class AlphaSlider : public QSlider {
 public:
  explicit AlphaSlider(Qt::Orientation orientation, QWidget* parent = nullptr)
      : QSlider(orientation, parent) {}

 protected:
  void paintEvent(QPaintEvent* event) override {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int grooveHeight = 10;
    const QRect grooveRect(6, (height() - grooveHeight) / 2, width() - 12, grooveHeight);
    if (grooveRect.width() <= 0 || grooveRect.height() <= 0) {
      return;
    }

    const int minValue = minimum();
    const int maxValue = maximum();
    const double ratio = maxValue == minValue
                             ? 0.0
                             : static_cast<double>(value() - minValue) /
                                   static_cast<double>(maxValue - minValue);

    const int usableWidth = grooveRect.width() - 1;
    const int fillWidth = qBound(0, static_cast<int>(std::lround(ratio * usableWidth)), usableWidth);

    const QRect fillRect(grooveRect.left(), grooveRect.top(), fillWidth, grooveRect.height());
    const QRect remainRect(
        grooveRect.left() + fillWidth,
        grooveRect.top(),
        grooveRect.width() - fillWidth,
        grooveRect.height());

    painter.setPen(Qt::NoPen);

    if (remainRect.width() > 0) {
      painter.setBrush(QColor(57, 67, 82));
      painter.drawRoundedRect(remainRect.adjusted(0, 0, -1, -1), 3, 3);
    }

    if (fillRect.width() > 0) {
      painter.save();
      painter.setClipRect(fillRect);

      const int cell = 4;
      for (int y = grooveRect.top(); y < grooveRect.bottom(); y += cell) {
        for (int x = grooveRect.left(); x < grooveRect.right(); x += cell) {
          const bool dark =
              (((x - grooveRect.left()) / cell) + ((y - grooveRect.top()) / cell)) % 2 == 0;
          painter.fillRect(
              QRect(x, y, cell, cell),
              dark ? QColor(74, 82, 94) : QColor(122, 132, 146));
        }
      }

      QLinearGradient gradient(fillRect.left(), 0, fillRect.right(), 0);
      gradient.setColorAt(0.0, QColor(127, 169, 232, 35));
      gradient.setColorAt(0.45, QColor(127, 169, 232, 125));
      gradient.setColorAt(1.0, QColor(127, 169, 232, 235));
      painter.fillRect(fillRect, gradient);

      painter.restore();
    }

    painter.setPen(QPen(QColor(43, 53, 67), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(grooveRect.adjusted(0, 0, -1, -1), 3, 3);

    const int handleCenterX = grooveRect.left() + fillWidth;
    const QRect handleRect(handleCenterX - 5, (height() - 18) / 2, 10, 18);

    painter.setPen(QPen(isEnabled() ? QColor(223, 232, 246) : QColor(92, 104, 120), 1));
    painter.setBrush(isEnabled() ? QColor(238, 243, 250) : QColor(120, 130, 145));
    painter.drawRoundedRect(handleRect, 3, 3);
  }};

QRect layerVisibilityRect(const QRect& rect) {
  return QRect(rect.left() + 4, rect.top() + (rect.height() - 16) / 2, 16, 16);
}

QRect layerActiveRect(const QRect& rect) {
  return QRect(rect.left() + 4 + kVisibilitySlotWidth, rect.top() + (rect.height() - 14) / 2, 14, 14);
}

QRect layerThumbnailRect(const QRect& rect) {
  const int left = rect.left() + 4 + kVisibilitySlotWidth + kActiveSlotWidth + 3;
  return QRect(left, rect.top() + (rect.height() - kLayerThumbHeight) / 2, kLayerThumbWidth, kLayerThumbHeight);
}

QRect layerNameRect(const QRect& rect) {
  const QRect thumb = layerThumbnailRect(rect);
  return QRect(thumb.right() + 7, rect.top(), rect.right() - thumb.right() - 10, rect.height());
}

QString layerPaintName(const QModelIndex& index) {
  if (index.data(kPaperRole).toBool()) {
    return QStringLiteral("用紙");
  }

  QString name = index.data(kNameRole).toString();
  if (name.trimmed().isEmpty()) {
    name = index.data(Qt::DisplayRole).toString();
  }

  name = stripLayerDecorators(name);
  if (name.isEmpty()) {
    name = QStringLiteral("レイヤー");
  }
  return name;
}class LayerItemDelegate : public QStyledItemDelegate {
 public:
  explicit LayerItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(120, kLayerRowHeight);
  }

  void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRect rect = option.rect;
    const bool selected = option.state.testFlag(QStyle::State_Selected);
    const bool active = index.data(kActiveRole).toBool();
    const bool visible = index.data(kVisibilityRole).toBool();

    QColor background = QColor(Qt::transparent);
    if (selected || active) {
      background = QColor(38, 65, 102);
    } else if (option.features.testFlag(QStyleOptionViewItem::Alternate)) {
      background = QColor(22, 29, 38);
    }

    if (background.alpha() > 0) {
      painter->fillRect(rect, background);
    }

    painter->setPen(QPen(QColor(43, 53, 67), 1));
    painter->drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());

    const QRect eyeRect = layerVisibilityRect(rect);
    const QIcon eyeIcon = visible ? app::ui::icon(QStringLiteral("visibility"))
                                  : app::ui::icon(QStringLiteral("visibility_off"));
    eyeIcon.paint(painter, eyeRect, Qt::AlignCenter, QIcon::Normal);

    const QRect activeRect = layerActiveRect(rect);
    if (active) {
      app::ui::icon(QStringLiteral("pen_active")).paint(painter, activeRect, Qt::AlignCenter, QIcon::Normal);
    }

    const QRect thumbRect = layerThumbnailRect(rect);
    const QIcon thumbnail = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    thumbnail.paint(painter, thumbRect, Qt::AlignCenter, QIcon::Normal);

    painter->setPen(QColor(42, 48, 58));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(thumbRect.adjusted(0, 0, -1, -1));

    QFont nameFont = option.font;
    nameFont.setBold(active);
    painter->setFont(nameFont);
    painter->setPen(visible ? QColor(242, 247, 255) : QColor(130, 142, 158));
    painter->drawText(layerNameRect(rect), Qt::AlignVCenter | Qt::AlignLeft, layerPaintName(index));

    painter->restore();
  }

  bool editorEvent(
      QEvent* event,
      QAbstractItemModel* model,
      const QStyleOptionViewItem& option,
      const QModelIndex& index) override {
    if (event == nullptr || model == nullptr || !index.isValid()) {
      return false;
    }

    if (event->type() == QEvent::MouseButtonRelease) {
      auto* mouseEvent = static_cast<QMouseEvent*>(event);
      if (mouseEvent->button() == Qt::LeftButton && layerVisibilityRect(option.rect).contains(mouseEvent->pos())) {
        const bool visible = index.data(kVisibilityRole).toBool();
        model->setData(index, visible ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole);
        return true;
      }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
  }
};

} // namespace

LayerPanel::LayerPanel(QWidget* parent)
    : QWidget(parent),
      m_headerLabel(new QLabel(QStringLiteral("レイヤー"), this)),
      m_filterEdit(new QLineEdit(this)),
      m_layerList(new QListWidget(this)),
      m_opacityLabel(new QLabel(QStringLiteral("不透明度: 100%"), this)),
      m_opacitySlider(new AlphaSlider(Qt::Horizontal, this)),
      m_opacitySpin(new QSpinBox(this)),
      m_blendModeCombo(new QComboBox(this)),
      m_addRasterButton(new QPushButton(QStringLiteral("ラスタ追加"), this)),
      m_addVectorButton(new QPushButton(QStringLiteral("ベクター追加"), this)),
      m_addFolderButton(new QPushButton(QStringLiteral("フォルダ追加"), this)),
      m_duplicateButton(new QPushButton(QStringLiteral("複製"), this)),
      m_upButton(new QPushButton(QStringLiteral("上へ"), this)),
      m_downButton(new QPushButton(QStringLiteral("下へ"), this)),
      m_deleteButton(new QPushButton(QStringLiteral("削除"), this)),
      m_clipButton(new QPushButton(QStringLiteral("クリップ"), this)),
      m_maskButton(new QPushButton(QStringLiteral("マスク"), this)),
      m_removeMaskButton(new QPushButton(QStringLiteral("マスク解除"), this)),
      m_lockButton(new QPushButton(QStringLiteral("ロック"), this)),
      m_lockAlphaButton(new QPushButton(QStringLiteral("透明保護"), this)),
      m_lockPositionButton(new QPushButton(QStringLiteral("位置固定"), this)),
      m_primaryGroup(new QGroupBox(QStringLiteral("頻用操作"), this)),
      m_stateGroup(new QGroupBox(QStringLiteral("状態操作"), this)),
      m_primaryGrid(new QGridLayout()),
      m_stateGrid(new QGridLayout()) {
  m_headerLabel->setStyleSheet("font-weight:700;");

  m_filterEdit->setPlaceholderText(QStringLiteral("レイヤーを検索..."));
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
  m_layerList->setContextMenuPolicy(Qt::CustomContextMenu);
  m_layerList->setSpacing(0);
  m_layerList->setUniformItemSizes(true);
  m_layerList->setItemDelegate(new LayerItemDelegate(m_layerList));
  m_layerList->setMinimumHeight(140);
  m_layerList->setStyleSheet(
      "QListWidget::item { min-height: 28px; padding: 3px 5px; border-bottom: 1px solid #313844; }"
      "QListWidget::item:selected { background: #2e4f79; color: #ffffff; }"
      "QListWidget::item:drop { border-top: 2px solid #7fb3ff; background: #243142; }"
      "QListWidget::indicator { width: 14px; height: 14px; }"
      "QListWidget::indicator:checked { image: url(:/icons/16/visibility.svg); }"
      "QListWidget::indicator:unchecked { image: url(:/icons/16/visibility_off.svg); }");

  m_opacitySlider->setRange(0, 100);
  m_opacitySlider->setValue(100);
  m_opacitySlider->setToolTip(QStringLiteral("アクティブレイヤーの不透明度を調整"));
  m_opacitySlider->setFixedHeight(22);
  m_opacitySpin->setRange(0, 100);
  m_opacitySpin->setValue(100);
  m_opacitySpin->setSuffix(QStringLiteral("%"));
  m_opacitySpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
  m_opacitySpin->setAlignment(Qt::AlignRight);
  m_opacitySpin->setFixedWidth(48);

  m_blendModeCombo->addItem(QStringLiteral("合成: 通常"), static_cast<int>(core::BlendMode::Normal));
  m_blendModeCombo->addItem(QStringLiteral("合成: 乗算"), static_cast<int>(core::BlendMode::Multiply));
  m_blendModeCombo->addItem(QStringLiteral("合成: 加算"), static_cast<int>(core::BlendMode::Add));

  auto initButton = [](QPushButton* button, const QString& iconName, const QString& fullText) {
    button->setIcon(app::ui::icon(iconName));
    button->setProperty("fullText", fullText);
    button->setProperty("shortText", QString());
    button->setIconSize(QSize(14, 14));
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setMinimumHeight(24);
    button->setText(fullText);
  };

  initButton(m_addRasterButton, QStringLiteral("layer_add"), QStringLiteral("ラスタ追加"));
  initButton(m_addVectorButton, QStringLiteral("vector_add"), QStringLiteral("ベクター追加"));
  initButton(m_addFolderButton, QStringLiteral("folder"), QStringLiteral("フォルダ追加"));
  initButton(m_duplicateButton, QStringLiteral("duplicate"), QStringLiteral("複製"));
  initButton(m_upButton, QStringLiteral("up"), QStringLiteral("上へ"));
  initButton(m_downButton, QStringLiteral("down"), QStringLiteral("下へ"));
  initButton(m_deleteButton, QStringLiteral("delete"), QStringLiteral("削除"));
  initButton(m_clipButton, QStringLiteral("clip"), QStringLiteral("クリップ"));
  initButton(m_maskButton, QStringLiteral("mask"), QStringLiteral("マスク"));
  initButton(m_removeMaskButton, QStringLiteral("mask_remove"), QStringLiteral("マスク解除"));
  initButton(m_lockButton, QStringLiteral("lock"), QStringLiteral("ロック"));
  initButton(m_lockAlphaButton, QStringLiteral("lock"), QStringLiteral("透明保護"));
  initButton(m_lockPositionButton, QStringLiteral("move"), QStringLiteral("位置固定"));
  m_upButton->setToolTip(QStringLiteral("選択中レイヤーを上（前面）へ移動"));
  m_downButton->setToolTip(QStringLiteral("選択中レイヤーを下（背面）へ移動"));

  auto forceLayerIconButton = [](QPushButton* button) {
    if (button == nullptr) {
      return;
    }

    const QString label = button->property("fullText").toString().isEmpty()
                              ? button->text()
                              : button->property("fullText").toString();
    button->setProperty("fullText", label);
    button->setToolTip(label);
    button->setText(QString());
    button->setFixedSize(28, 24);
    button->setIconSize(QSize(16, 16));
    button->setFocusPolicy(Qt::NoFocus);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    button->setStyleSheet(QStringLiteral(
        "QPushButton {"
        " margin: 0px;"
        " padding: 0px;"
        " border: 1px solid #3a4658;"
        " border-radius: 3px;"
        " background: #202a36;"
        " color: #d8e2f0;"
        "}"
        "QPushButton:hover { background: #263446; border-color: #55708f; }"
        "QPushButton:pressed { background: #2e4f79; border-color: #7fb3ff; }"
        "QPushButton:disabled { color: #5f6b7a; border-color: #2c3542; background: #18212c; }"));
  };

  for (QPushButton* button : QList<QPushButton*> {
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
           m_lockPositionButton}) {
    forceLayerIconButton(button);
  }


  m_primaryGrid->setContentsMargins(2, 2, 2, 2);
  m_primaryGrid->setHorizontalSpacing(3);
  m_primaryGrid->setVerticalSpacing(3);
  m_primaryGrid->addWidget(m_addRasterButton, 0, 0);
  m_primaryGrid->addWidget(m_addVectorButton, 0, 1);
  m_primaryGrid->addWidget(m_addFolderButton, 0, 2);
  m_primaryGrid->addWidget(m_duplicateButton, 0, 3);
  m_primaryGrid->addWidget(m_upButton, 1, 0);
  m_primaryGrid->addWidget(m_downButton, 1, 1);
  m_primaryGrid->addWidget(m_deleteButton, 1, 2);
  m_primaryGrid->setColumnStretch(3, 1);
  m_primaryGroup->setLayout(m_primaryGrid);

  m_stateGrid->setContentsMargins(2, 2, 2, 2);
  m_stateGrid->setHorizontalSpacing(3);
  m_stateGrid->setVerticalSpacing(3);
  m_stateGrid->addWidget(m_clipButton, 0, 0);
  m_stateGrid->addWidget(m_maskButton, 0, 1);
  m_stateGrid->addWidget(m_removeMaskButton, 0, 2);
  m_stateGrid->addWidget(m_lockButton, 1, 0);
  m_stateGrid->addWidget(m_lockAlphaButton, 1, 1);
  m_stateGrid->addWidget(m_lockPositionButton, 1, 2);
  m_stateGroup->setLayout(m_stateGrid);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(4);
  auto* opacityRow = new QHBoxLayout();
  opacityRow->setContentsMargins(0, 0, 0, 0);
  opacityRow->setSpacing(4);
  opacityRow->addWidget(m_opacitySlider, 1);
  opacityRow->addWidget(m_opacitySpin, 0);
  layout->addWidget(m_headerLabel);
  layout->addWidget(m_filterEdit);
  layout->addWidget(m_layerList, 1);
  layout->addWidget(m_opacityLabel);
  layout->addLayout(opacityRow);
  layout->addWidget(m_blendModeCombo);
  layout->addWidget(m_primaryGroup);
  layout->addWidget(m_stateGroup);
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
  connect(m_layerList, &QListWidget::customContextMenuRequested, this, &LayerPanel::onLayerContextMenuRequested);
  connect(m_layerList->model(), &QAbstractItemModel::rowsMoved, this, &LayerPanel::onLayerRowsMoved);
  connect(m_opacitySlider, &QSlider::valueChanged, this, &LayerPanel::onOpacityChanged);
  connect(m_opacitySpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
    if (m_isRefreshing) {
      return;
    }
    const QSignalBlocker blocker(m_opacitySlider);
    m_opacitySlider->setValue(value);
    onOpacityChanged(value);
  });
  connect(m_blendModeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &LayerPanel::onBlendModeChanged);

  applyResponsiveMode();
}

void LayerPanel::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  applyResponsiveMode();
}

void LayerPanel::applyButtonCompactMode(bool compact) {
  Q_UNUSED(compact);

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
    if (button == nullptr) {
      continue;
    }

    const QString full = button->property("fullText").toString();
    button->setText(QString());
    button->setToolTip(full);
    button->setFixedSize(28, 24);
    button->setIconSize(QSize(16, 16));
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  }

  m_compactButtons = true;
}void LayerPanel::applyResponsiveMode() {
  const bool compactWidth = width() < 330;
  const bool compactHeight = height() < 560;

  applyButtonCompactMode(compactWidth);

  if (m_stateGroup != nullptr) {
    m_stateGroup->setVisible(!compactHeight);
  }

  if (m_primaryGrid != nullptr) {
    const int columns = compactWidth ? 3 : 4;
    m_primaryGrid->setColumnStretch(0, 1);
    m_primaryGrid->setColumnStretch(1, 1);
    m_primaryGrid->setColumnStretch(2, 1);
    m_primaryGrid->setColumnStretch(3, columns == 4 ? 1 : 0);
  }

  if (m_layerList != nullptr) {
    m_layerList->setMinimumHeight(compactHeight ? 90 : 140);
  }
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
  for (std::size_t cleanupIndex = models.size(); cleanupIndex-- > 0;) {
    const auto& cleanupModel = models[cleanupIndex];
    if (cleanupModel.paperLayer || cleanupIndex == 0) {
      continue;
    }

    const QString rawName = QString::fromStdString(cleanupModel.name).trimmed();
    const QString cleanName = stripLayerDecorators(rawName);
    if (!cleanName.isEmpty() && cleanName != rawName) {
      m_isRefreshing = false;
      m_controller->renameLayer(static_cast<std::size_t>(cleanupIndex - 1), cleanName.toStdString());
      return;
    }
  }
  for (std::size_t layerIndex = models.size(); layerIndex-- > 0;) {
    const auto& model = models[layerIndex];
    if (hasFilter) {
      const QString layerName = QString::fromStdString(model.name);
      if (!layerName.contains(filterText, Qt::CaseInsensitive) && !(model.paperLayer && QStringLiteral("用紙").contains(filterText))) {
        continue;
      }
    }

    auto* item = new QListWidgetItem(m_layerList);
    item->setText(stripLayerDecorators(decorateLayerName(model)));
    const std::size_t documentIndex = model.paperLayer ? 0 : (layerIndex > 0 ? layerIndex - 1 : 0);
    item->setIcon(layerThumbnailIcon(m_controller, model, documentIndex));

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    if (!model.paperLayer) {
      flags |= Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }
    item->setFlags(flags);
    item->setCheckState(model.visible ? Qt::Checked : Qt::Unchecked);
    item->setData(kNameRole, QString::fromStdString(model.name));
    item->setData(kVisibilityRole, model.visible);
    item->setData(kKindRole, static_cast<int>(model.kind));
    item->setData(kLayerIndexRole, model.paperLayer ? -1 : static_cast<int>(layerIndex - 1));
    item->setData(kClippedRole, model.clippedToBelow);
    item->setData(kHasMaskRole, model.hasMask);
    item->setData(kMaskEnabledRole, model.maskEnabled);
    item->setData(kLockedRole, model.locked);
    item->setData(kAlphaLockedRole, model.alphaLocked);
    item->setData(kPositionLockedRole, model.positionLocked);
    item->setData(kBlendModeRole, static_cast<int>(model.blendMode));
    item->setData(kPaperRole, model.paperLayer);
    item->setData(kActiveRole, model.active);

    const QString stateSummary = QStringLiteral("表示:%1  クリップ:%2  マスク:%3  ロック:%4")
                                     .arg(model.visible ? QStringLiteral("ON") : QStringLiteral("OFF"))
                                     .arg(model.clippedToBelow ? QStringLiteral("ON") : QStringLiteral("OFF"))
                                     .arg(model.hasMask ? (model.maskEnabled ? QStringLiteral("ON") : QStringLiteral("無効")) : QStringLiteral("なし"))
                                     .arg(model.locked ? QStringLiteral("ON") : QStringLiteral("OFF"));
    const QString tooltip = model.paperLayer
        ? QStringLiteral("用紙レイヤー: 表示/非表示のみ変更できます")
        : QStringLiteral("%1 / 合成: %2\n%3").arg(layerKindText(model.kind), blendModeName(model.blendMode), stateSummary);
    item->setToolTip(tooltip);
    item->setSizeHint(QSize(item->sizeHint().width(), kLayerRowHeight));

    QFont font = item->font();
    font.setBold(model.active);
    item->setFont(font);
    item->setBackground(model.active ? QColor(48, 79, 130) : QColor(Qt::transparent));

    if (model.active) {
      m_layerList->setCurrentItem(item);
      const QSignalBlocker sliderBlocker(m_opacitySlider);
      const QSignalBlocker spinBlocker(m_opacitySpin);
      const QSignalBlocker blendBlocker(m_blendModeCombo);
      m_opacitySlider->setValue(model.opacityPercent);
      m_opacitySpin->setValue(model.opacityPercent);
      m_opacityLabel->setText(QStringLiteral("不透明度: %1%").arg(model.opacityPercent));
      const int blendIndex = m_blendModeCombo->findData(static_cast<int>(model.blendMode));
      if (blendIndex >= 0) {
        m_blendModeCombo->setCurrentIndex(blendIndex);
      }
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
    refreshButtonState();
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
  const bool isPaper = item->data(kPaperRole).toBool();
  if (isPaper) {
    const bool visible = item->checkState() == Qt::Checked;
    m_controller->setPaperVisible(visible);
    refreshButtonState();
    return;
  }

  const std::size_t layerIndex =
      layerIndexValue >= 0 ? static_cast<std::size_t>(layerIndexValue) : layerIndexFromRow(row);

  const QString oldName = stripLayerDecorators(item->data(kNameRole).toString());
  QString newName = stripLayerDecorators(item->text());
  if (newName.isEmpty()) {
    const QSignalBlocker signalBlocker(m_layerList);
    item->setText(oldName.isEmpty() ? QStringLiteral("レイヤー") : oldName);
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
}void LayerPanel::onOpacityChanged(int value) {
  if (m_controller == nullptr || m_isRefreshing) {
    return;
  }
  QListWidgetItem* currentItem = m_layerList->currentItem();
  if (currentItem != nullptr && currentItem->data(kPaperRole).toBool()) {
    return;
  }
  m_opacityLabel->setText(QStringLiteral("不透明度: %1%").arg(value));
  const QSignalBlocker spinBlocker(m_opacitySpin);
  m_opacitySpin->setValue(value);
  m_controller->setActiveLayerOpacity(value);
}

void LayerPanel::onBlendModeChanged(int index) {
  if (m_controller == nullptr || m_isRefreshing) {
    return;
  }
  QListWidgetItem* currentItem = m_layerList->currentItem();
  if (currentItem == nullptr || currentItem->data(kPaperRole).toBool()) {
    return;
  }
  const QVariant modeData = m_blendModeCombo->itemData(index);
  if (!modeData.isValid()) {
    return;
  }
  m_controller->setActiveLayerBlendMode(static_cast<core::BlendMode>(modeData.toInt()));
}

void LayerPanel::onFilterTextChanged(const QString& text) {
  Q_UNUSED(text);
  refreshLayers();
}

void LayerPanel::onLayerContextMenuRequested(const QPoint& pos) {
  if (m_controller == nullptr) {
    return;
  }

  QListWidgetItem* item = m_layerList->itemAt(pos);
  if (item != nullptr) {
    m_layerList->setCurrentItem(item);
  }

  const bool hasSelection = m_layerList->currentItem() != nullptr;
  const bool paperSelected = hasSelection && m_layerList->currentItem()->data(kPaperRole).toBool();
  const bool canEditLayer = hasSelection && !paperSelected;

  QMenu menu(this);
  QAction* renameAction = menu.addAction(app::ui::icon(QStringLiteral("layer")), QStringLiteral("名前変更"));
  QAction* duplicateAction = menu.addAction(app::ui::icon(QStringLiteral("duplicate")), QStringLiteral("複製"));
  QAction* deleteAction = menu.addAction(app::ui::icon(QStringLiteral("delete")), QStringLiteral("削除"));
  menu.addSeparator();
  QAction* addRasterAction = menu.addAction(app::ui::icon(QStringLiteral("layer_add")), QStringLiteral("新規ラスターレイヤー"));
  QAction* addVectorAction = menu.addAction(app::ui::icon(QStringLiteral("vector_add")), QStringLiteral("新規ベクターレイヤー"));
  QAction* addFolderAction = menu.addAction(app::ui::icon(QStringLiteral("folder")), QStringLiteral("新規フォルダー"));
  menu.addSeparator();
  QAction* moveUpAction = menu.addAction(app::ui::icon(QStringLiteral("up")), QStringLiteral("上へ移動"));
  QAction* moveDownAction = menu.addAction(app::ui::icon(QStringLiteral("down")), QStringLiteral("下へ移動"));
  QAction* toggleVisibleAction = menu.addAction(app::ui::icon(QStringLiteral("visibility")), QStringLiteral("表示/非表示を切替"));
  QAction* toggleClipAction = menu.addAction(app::ui::icon(QStringLiteral("clip")), QStringLiteral("クリッピング切替"));
  QAction* toggleLockAction = menu.addAction(app::ui::icon(QStringLiteral("lock")), QStringLiteral("ロック切替"));
  QAction* mergeDownAction = menu.addAction(QStringLiteral("下のレイヤーと結合"));
  QAction* rasterizeAction = menu.addAction(QStringLiteral("ラスタライズ"));

  renameAction->setEnabled(canEditLayer);
  duplicateAction->setEnabled(canEditLayer);
  deleteAction->setEnabled(canEditLayer);
  moveUpAction->setEnabled(canEditLayer && m_upButton->isEnabled());
  moveDownAction->setEnabled(canEditLayer && m_downButton->isEnabled());
  toggleVisibleAction->setEnabled(hasSelection);
  toggleClipAction->setEnabled(canEditLayer && m_clipButton->isEnabled());
  toggleLockAction->setEnabled(canEditLayer && m_lockButton->isEnabled());
  mergeDownAction->setEnabled(canEditLayer);
  rasterizeAction->setEnabled(canEditLayer);

  if (paperSelected) {
    toggleVisibleAction->setText(QStringLiteral("用紙の表示/非表示を切替"));
  }

  QAction* selected = menu.exec(m_layerList->viewport()->mapToGlobal(pos));
  if (selected == nullptr) {
    return;
  }

  if (selected == renameAction && m_layerList->currentItem() != nullptr) {
    m_layerList->editItem(m_layerList->currentItem());
    return;
  }
  if (selected == duplicateAction) {
    onDuplicateLayerClicked();
    return;
  }
  if (selected == deleteAction) {
    onDeleteLayerClicked();
    return;
  }
  if (selected == addRasterAction) {
    onAddRasterLayerClicked();
    return;
  }
  if (selected == addVectorAction) {
    onAddVectorLayerClicked();
    return;
  }
  if (selected == addFolderAction) {
    onAddFolderLayerClicked();
    return;
  }
  if (selected == moveUpAction) {
    onMoveLayerUpClicked();
    return;
  }
  if (selected == moveDownAction) {
    onMoveLayerDownClicked();
    return;
  }
  if (selected == toggleVisibleAction && hasSelection && m_layerList->currentItem() != nullptr) {
    QListWidgetItem* current = m_layerList->currentItem();
    current->setCheckState(current->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
    return;
  }
  if (selected == toggleClipAction) {
    onToggleClipClicked();
    return;
  }
  if (selected == toggleLockAction) {
    onToggleLockClicked();
    return;
  }
  if (selected == mergeDownAction && m_layerList->currentItem() != nullptr) {
    const int layerIndex = m_layerList->currentItem()->data(kLayerIndexRole).toInt();
    if (layerIndex >= 0) {
      m_controller->mergeLayerDown(static_cast<std::size_t>(layerIndex));
    }
    return;
  }
  if (selected == rasterizeAction && m_layerList->currentItem() != nullptr) {
    const int layerIndex = m_layerList->currentItem()->data(kLayerIndexRole).toInt();
    if (layerIndex >= 0) {
      m_controller->rasterizeLayer(static_cast<std::size_t>(layerIndex));
    }
    return;
  }
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
  const std::size_t layerCount = m_controller->document().layerCount();
  if (fromLayerIndex >= layerCount || toLayerIndex >= layerCount) {
    refreshLayers();
    return;
  }
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
  if (row < 0 || row >= static_cast<int>(count)) {
    return count;
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
    m_opacitySpin->setEnabled(false);
    m_blendModeCombo->setEnabled(false);
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
  QListWidgetItem* currentItem = current >= 0 ? m_layerList->item(current) : nullptr;
  const bool paperSelected = currentItem != nullptr && currentItem->data(kPaperRole).toBool();

  const bool canMoveUp = current > 0;
  const bool canMoveDown = current >= 0 && current < lastRow;

  m_deleteButton->setEnabled(hasSelection && canDelete && !paperSelected);
  m_duplicateButton->setEnabled(hasSelection && !paperSelected);
  m_upButton->setEnabled(canMoveUp && !paperSelected);
  m_downButton->setEnabled(canMoveDown && !paperSelected);
  m_opacitySlider->setEnabled(hasSelection && !paperSelected);
  m_opacitySpin->setEnabled(hasSelection && !paperSelected);
  m_blendModeCombo->setEnabled(hasSelection && !paperSelected);

  if (paperSelected) {
    m_opacityLabel->setText(QStringLiteral("用紙レイヤー（背景色）"));
  }

  bool canClipOrMask = false;
  bool hasMask = false;
  bool maskEnabled = false;
  bool clipped = false;
  bool locked = false;
  bool alphaLocked = false;
  bool positionLocked = false;
  core::LayerKind kind = core::LayerKind::Raster;

  if (hasSelection && !paperSelected) {
    QListWidgetItem* activeItem = m_layerList->item(current);
    if (activeItem != nullptr) {
      const int layerIndex = activeItem->data(kLayerIndexRole).toInt();
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
        const QSignalBlocker blendBlocker(m_blendModeCombo);
        const int blendIndex = m_blendModeCombo->findData(static_cast<int>(layer.blendMode()));
        if (blendIndex >= 0) {
          m_blendModeCombo->setCurrentIndex(blendIndex);
        }
      }
    }
  }

  m_clipButton->setEnabled(canClipOrMask);
  m_maskButton->setEnabled(canClipOrMask);
  m_removeMaskButton->setEnabled(canClipOrMask && hasMask);
  m_lockButton->setEnabled(hasSelection && kind != core::LayerKind::Folder);
  m_lockAlphaButton->setEnabled(hasSelection && kind == core::LayerKind::Raster);
  m_lockPositionButton->setEnabled(hasSelection && kind != core::LayerKind::Folder);

}

} // namespace app::panels
