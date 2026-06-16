#include "app/panels/LayerPanel.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <unordered_set>

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
#include <QTimer>
#include <QVBoxLayout>
#include <QDropEvent>
#include <QListWidgetItem>

#include "app/bridge/AppController.h"
#include "app/ui/IconLoader.h"
#include "app/ui/system/DesignSystem.h"

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
constexpr int kEditTargetRole = Qt::UserRole + 14;  ///< 0=Image, 1=Mask
constexpr int kLayerIdRole     = Qt::UserRole + 15;  ///< レイヤーの安定ID
constexpr int kParentIdRole    = Qt::UserRole + 16;  ///< 親フォルダの安定ID（0=ルート）
constexpr int kDepthRole       = Qt::UserRole + 17;  ///< 階層の深さ（0=ルート）
constexpr int kExpandedRole    = Qt::UserRole + 18;  ///< フォルダが展開中かどうか
constexpr int kExpandToggleRole= Qt::UserRole + 19;  ///< デリゲート→パネルへ折りたたみトグル要求
constexpr int kEffVisibleRole  = Qt::UserRole + 20;  ///< 親チェーン込みの effective visibility
namespace DS = app::ui::system;

constexpr int kIndentWidth        = DS::layer::kIndentW;         ///< 深さ1段あたりのインデント幅（px）
constexpr int kLayerRowHeight     = DS::layer::kRowHeight;
constexpr int kLayerThumbWidth    = DS::layer::kThumbW;
constexpr int kLayerThumbHeight   = DS::layer::kThumbH;
constexpr int kMaskThumbWidth     = DS::layer::kMaskThumbW;
constexpr int kMaskThumbHeight    = DS::layer::kMaskThumbH;
constexpr int kThumbGap           = DS::layer::kThumbGap;
constexpr int kVisibilitySlotWidth = DS::layer::kVisibilitySlotW;
constexpr int kActiveSlotWidth    = DS::layer::kActiveSlotW;
constexpr int kStatusAreaWidth    = DS::layer::kStatusAreaW;

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
    case core::BlendMode::Normal:      return QStringLiteral("通常");
    case core::BlendMode::Dissolve:    return QStringLiteral("ディザ合成");
    case core::BlendMode::Darken:      return QStringLiteral("暗く");
    case core::BlendMode::Multiply:    return QStringLiteral("乗算");
    case core::BlendMode::ColorBurn:   return QStringLiteral("焼き込みカラー");
    case core::BlendMode::LinearBurn:  return QStringLiteral("焼き込み（リニア）");
    case core::BlendMode::DarkerColor: return QStringLiteral("カラー比較（暗）");
    case core::BlendMode::Lighten:     return QStringLiteral("明るく");
    case core::BlendMode::Screen:      return QStringLiteral("スクリーン");
    case core::BlendMode::ColorDodge:  return QStringLiteral("覆い焼きカラー");
    case core::BlendMode::LinearDodge: return QStringLiteral("加算");
    case core::BlendMode::LighterColor:return QStringLiteral("カラー比較（明）");
    case core::BlendMode::Overlay:     return QStringLiteral("オーバーレイ");
    case core::BlendMode::SoftLight:   return QStringLiteral("ソフトライト");
    case core::BlendMode::HardLight:   return QStringLiteral("ハードライト");
    case core::BlendMode::VividLight:  return QStringLiteral("ビビッドライト");
    case core::BlendMode::LinearLight: return QStringLiteral("リニアライト");
    case core::BlendMode::PinLight:    return QStringLiteral("ピンライト");
    case core::BlendMode::HardMix:     return QStringLiteral("ハードミックス");
    case core::BlendMode::Difference:  return QStringLiteral("差の絶対値");
    case core::BlendMode::Exclusion:   return QStringLiteral("除外");
    case core::BlendMode::Subtract:    return QStringLiteral("減算");
    case core::BlendMode::Divide:      return QStringLiteral("除算");
    case core::BlendMode::Hue:         return QStringLiteral("色相");
    case core::BlendMode::HslSat:      return QStringLiteral("彩度");
    case core::BlendMode::HslColor:    return QStringLiteral("カラー");
    case core::BlendMode::Luminosity:  return QStringLiteral("輝度");
    default:                           return QStringLiteral("通常");
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
  return dark ? QColor(52, 52, 52) : QColor(76, 76, 76);
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

QPixmap maskThumbnailPixmap(
    const app::bridge::AppController* controller,
    std::size_t layerIndex) {
  QImage image(kMaskThumbWidth, kMaskThumbHeight, QImage::Format_ARGB32_Premultiplied);
  if (layerIndex < controller->document().layerCount()) {
    const core::Layer& layer = controller->document().layerAt(layerIndex);
    if (layer.hasMask()) {
      const core::PixelBuffer& maskBuf = layer.maskBuffer();
      for (int y = 0; y < kMaskThumbHeight; ++y) {
        const int sy = std::clamp((y * maskBuf.height()) / kMaskThumbHeight, 0, maskBuf.height() - 1);
        for (int x = 0; x < kMaskThumbWidth; ++x) {
          const int sx = std::clamp((x * maskBuf.width()) / kMaskThumbWidth, 0, maskBuf.width() - 1);
          const std::uint8_t g = maskBuf.pixel(sx, sy).r;
          image.setPixelColor(x, y, QColor(g, g, g));
        }
      }
    } else {
      image.fill(QColor(40, 40, 40));
    }
  } else {
    image.fill(QColor(40, 40, 40));
  }
  QPixmap pm = QPixmap::fromImage(image);
  QPainter border(&pm);
  border.setPen(QPen(QColor(40, 46, 56), 1.0));
  border.setBrush(Qt::NoBrush);
  border.drawRect(QRect(0, 0, kMaskThumbWidth - 1, kMaskThumbHeight - 1));
  border.end();
  return pm;
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

QRect layerMaskThumbnailRect(const QRect& rect) {
  const QRect img = layerThumbnailRect(rect);
  return QRect(img.right() + kThumbGap, rect.top() + (rect.height() - kMaskThumbHeight) / 2, kMaskThumbWidth, kMaskThumbHeight);
}

QRect layerNameRect(const QRect& rect, bool hasMask) {
  const int nameLeft = hasMask
      ? layerMaskThumbnailRect(rect).right() + 4
      : layerThumbnailRect(rect).right() + 4;
  const int nameRight = rect.right() - kStatusAreaWidth - 2;
  return QRect(nameLeft, rect.top(), std::max(0, nameRight - nameLeft), rect.height());
}

QRect layerStatusRect(const QRect& rect) {
  return QRect(rect.right() - kStatusAreaWidth, rect.top(), kStatusAreaWidth, rect.height());
}

// --- 行レイアウト構造体 ---------------------------------------------------------
// CSP型左ゾーン(eye→expand→typeIcon)→thumb→mask→name→stateIcon 順序
// paint() と editorEvent() の両方がこれを参照する。
struct LayerRowRects {
  QRect content;    // インデント後の全コンテンツ領域
  QRect indent;     // 深さインデント帯 (depth=0 なら zero-width)
  QRect eye;        // 表示アイコン（左ゾーン1）
  QRect thumb;      // サムネイル
  QRect mask;       // マスクサムネイル（isNull() = マスクなし）
  QRect name;       // 名前テキスト領域
  QRect stateIcon;  // 右端ステータスアイコン帯（lock 系）
};

LayerRowRects computeRowRects(const QRect& itemRect, int depth, bool hasMask) {
  LayerRowRects r;

  // ── インデント ────────────────────────────────────────────────────────────
  const int indentW = depth * kIndentWidth;
  r.indent  = QRect(itemRect.left(), itemRect.top(), indentW, itemRect.height());
  r.content = depth > 0 ? itemRect.adjusted(indentW, 0, 0, 0) : itemRect;

  const int cy = itemRect.top() + itemRect.height() / 2;
  int x = r.content.left() + 2;  // 2px left margin

  // ── 左ゾーン: eye ───────────────────────────────────────────────────────────
  const int eyeS  = DS::icon::kMedium;                 // 16px
  r.eye    = QRect(x, cy - eyeS / 2, eyeS, eyeS);
  x += kVisibilitySlotWidth;


  // ── 中央: thumb → mask ────────────────────────────────────────────────────
  r.thumb = QRect(x,
                  itemRect.top() + (itemRect.height() - kLayerThumbHeight) / 2,
                  kLayerThumbWidth, kLayerThumbHeight);
  x = r.thumb.right() + kThumbGap;

  if (hasMask) {
    r.mask = QRect(x,
                   itemRect.top() + (itemRect.height() - kMaskThumbHeight) / 2,
                   kMaskThumbWidth, kMaskThumbHeight);
    x = r.mask.right() + kThumbGap;
  }

  // ── 右ゾーン: stateIcon（右固定）──────────────────────────────────────────
  r.stateIcon = QRect(r.content.right() - kStatusAreaWidth,
                      itemRect.top(), kStatusAreaWidth, itemRect.height());

  // ── name: thumb/mask 右 〜 stateIcon 左 ──────────────────────────────────
  const int nameRight = r.stateIcon.left() - 2;
  r.name = QRect(x, itemRect.top(), std::max(0, nameRight - x), itemRect.height());

  return r;
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
    const bool active   = index.data(kActiveRole).toBool();
    const bool visible  = index.data(kVisibilityRole).toBool();
    const bool isPaper  = index.data(kPaperRole).toBool();

    // ── 行全体ハイライト（active = 明青、selected = 暗青、alternate = 濃グレー）
    QColor background(Qt::transparent);
    if (active) {
      background = QColor(DS::layer::kActiveBg);
    } else if (selected) {
      background = QColor(DS::layer::kSelectedBg);
    } else if (option.features.testFlag(QStyleOptionViewItem::Alternate)) {
      background = QColor(DS::layer::kAlternateBg);
    }
    if (background.alpha() > 0) {
      painter->fillRect(rect, background);
    }

    // 行区切り線
    painter->setPen(QPen(QColor(DS::layer::kRowDivider), 1));
    painter->drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());

    // ── 全 Rect を先頭で一括計算（paint / editorEvent 共通）─────────────────
    const int depth    = index.data(kDepthRole).toInt();
    const bool hasMask = index.data(kHasMaskRole).toBool();
    const auto kind    = static_cast<core::LayerKind>(index.data(kKindRole).toInt());
    const LayerRowRects rr = computeRowRects(rect, depth, hasMask);

    const int editTarget   = index.data(kEditTargetRole).toInt();
    const bool editingMask  = hasMask && (editTarget == 1);
    const bool editingImage = !editingMask;

    // ── クリッピングインジケーター（強化版）──────────────────────────────────
    // 行左端から4px幅のオレンジ縦バーで前景に強調
    if (index.data(kClippedRole).toBool() && !isPaper) {
      QColor clipPrimary(DS::layer::kClipBar);
      clipPrimary.setAlpha(230);
      painter->fillRect(QRect(rr.content.left(), rect.top(), 4, rect.height()), clipPrimary);
      // サムネイル直前にも細バーで二重強調
      QColor clipSecondary(DS::layer::kClipBar);
      clipSecondary.setAlpha(150);
      painter->fillRect(QRect(rr.thumb.left() - 3, rr.thumb.top(), 2, rr.thumb.height()), clipSecondary);
    }

    // ── 左ゾーン 1: 目玉アイコン（表示/非表示）─────────────────────────────
    const QIcon eyeIcon = visible ? app::ui::icon(QStringLiteral("visibility"))
                                  : app::ui::icon(QStringLiteral("visibility_off"));
    eyeIcon.paint(painter, rr.eye, Qt::AlignCenter, QIcon::Normal);

        // ── 中央: メインサムネイル───────────────────────────────────────────────
    const QIcon thumbnail = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    thumbnail.paint(painter, rr.thumb, Qt::AlignCenter, QIcon::Normal);

    // フォルダ展開/折りたたみ chevron をサムネイル右下にオーバーレイ
    if (kind == core::LayerKind::Folder) {
      const bool expanded = index.data(kExpandedRole).toBool();
      QFont chFont = painter->font();
      chFont.setPointSize(6);
      painter->setFont(chFont);
      painter->setPen(QColor(DS::layer::kChevron));
      const QRect badge(rr.thumb.right() - 10, rr.thumb.bottom() - 10, 10, 10);
      painter->drawText(badge, Qt::AlignCenter, expanded ? QStringLiteral("▼") : QStringLiteral("▶"));
    }
    // サムネイル枠（編集中は青ハイライト）
    painter->setBrush(Qt::NoBrush);
    if (editingImage && !isPaper) {
      painter->setPen(QPen(QColor(DS::layer::kFocusBorder), 2));
    } else {
      painter->setPen(QPen(QColor(DS::layer::kThumbBorder), 1));
    }
    painter->drawRect(rr.thumb.adjusted(0, 0, -1, -1));

    // ── 中央: マスクサムネイル（横配置）────────────────────────────────────
    if (hasMask) {
      const QPixmap maskPm = qvariant_cast<QPixmap>(index.data(Qt::UserRole + 100));
      painter->drawPixmap(rr.mask, maskPm);

      painter->setBrush(Qt::NoBrush);
      if (editingMask) {
        painter->setPen(QPen(QColor(DS::layer::kFocusBorder), 2));
      } else {
        const bool maskEnabled = index.data(kMaskEnabledRole).toBool();
        painter->setPen(QPen(maskEnabled ? QColor(DS::layer::kThumbBorder)
                                         : QColor(DS::layer::kMaskDisabled), 1));
      }
      painter->drawRect(rr.mask.adjusted(0, 0, -1, -1));

      if (!index.data(kMaskEnabledRole).toBool()) {
        QColor maskX(DS::layer::kMaskDisabled);
        maskX.setAlpha(180);
        painter->setPen(QPen(maskX, 1));
        painter->drawLine(rr.mask.topLeft(), rr.mask.bottomRight());
        painter->drawLine(rr.mask.topRight(), rr.mask.bottomLeft());
      }
    }

    // ── 中央: レイヤー名─────────────────────────────────────────────────────
    QFont nameFont = option.font;
    nameFont.setBold(active);
    painter->setFont(nameFont);
    painter->setPen(visible ? QColor(DS::layer::kNameVisible)
                            : QColor(DS::layer::kNameHidden));
    painter->drawText(rr.name, Qt::AlignVCenter | Qt::AlignLeft, layerPaintName(index));

    // ── 右ゾーン: lock アイコン（右固定・右から左へ積む）───────────────────
    {
      const int iconS = DS::icon::kMedium;
      const int iconY = rect.top() + (rect.height() - iconS) / 2;
      int iconX = rr.stateIcon.right() - 2;

      if (index.data(kLockedRole).toBool()) {
        iconX -= iconS;
        app::ui::icon(QStringLiteral("lock")).paint(painter,
            QRect(iconX, iconY, iconS, iconS), Qt::AlignCenter, QIcon::Normal);
        iconX -= 2;
      }
      if (index.data(kAlphaLockedRole).toBool()) {
        iconX -= iconS;
        app::ui::icon(QStringLiteral("lock_alpha")).paint(painter,
            QRect(iconX, iconY, iconS, iconS), Qt::AlignCenter, QIcon::Normal);
        iconX -= 2;
      }
      if (index.data(kPositionLockedRole).toBool()) {
        iconX -= iconS;
        app::ui::icon(QStringLiteral("lock_position")).paint(painter,
            QRect(iconX, iconY, iconS, iconS), Qt::AlignCenter, QIcon::Normal);
      }
    }

    // ── 親フォルダ非表示オーバーレイ（最前面）──────────────────────────────
    const bool effVisible = index.data(kEffVisibleRole).toBool();
    if (!effVisible && visible) {
      QColor dimOverlay(DS::layer::kDimOverlay);
      dimOverlay.setAlpha(70);
      painter->fillRect(rr.content, dimOverlay);
    }

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
      if (mouseEvent->button() == Qt::LeftButton) {
        // paint() と同一の rect 計算（hit test と描画位置をゼロオフセットで一致させる）
        const int depth   = index.data(kDepthRole).toInt();
        const bool hasMask = index.data(kHasMaskRole).toBool();
        const LayerRowRects rr = computeRowRects(option.rect, depth, hasMask);
        const QPoint pos = mouseEvent->pos();

        // 表示切替
        if (rr.eye.contains(pos)) {
          const bool vis = index.data(kVisibilityRole).toBool();
          model->setData(index, vis ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole);
          return true;
        }

        // フォルダ展開/折りたたみ（chevron スロット または thumb クリック）
        const auto ekind = static_cast<core::LayerKind>(index.data(kKindRole).toInt());
        if (ekind == core::LayerKind::Folder &&
            rr.thumb.contains(pos)) {
          model->setData(index, true, kExpandToggleRole);
          return true;
        }

        // マスクサムネイルクリック（Shift=有効/無効トグル、通常=編集対象切替）
        if (hasMask && rr.mask.contains(pos)) {
          if (mouseEvent->modifiers().testFlag(Qt::ShiftModifier)) {
            const bool enabled = index.data(kMaskEnabledRole).toBool();
            model->setData(index, !enabled ? 2 : 3, kEditTargetRole);  // 2=enable, 3=disable
          } else {
            model->setData(index, 1, kEditTargetRole);  // switch to mask
          }
          return true;
        }

        // 画像サムネイルクリック → 編集対象を画像へ
        if (rr.thumb.contains(pos)) {
          model->setData(index, 0, kEditTargetRole);
          return true;
        }
      }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
  }

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
    Q_UNUSED(option);
    if (index.data(kPaperRole).toBool()) {
      return nullptr;
    }
    auto* editor = new QLineEdit(parent);
    editor->setFrame(false);
    editor->setStyleSheet(
        QStringLiteral("QLineEdit {"
        "  background: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 2px;"
        "  padding: 0px 2px;"
        "}")
        .arg(DS::layer::kEditorBg,
             DS::layer::kEditorText,
             DS::layer::kEditorBorder));
    return editor;
  }

  void setEditorData(QWidget* editor, const QModelIndex& index) const override {
    auto* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (lineEdit == nullptr) {
      return;
    }
    lineEdit->setText(layerPaintName(index));
    lineEdit->selectAll();
  }

  void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override {
    auto* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (lineEdit == nullptr) {
      return;
    }
    const QString text = lineEdit->text().trimmed();
    if (!text.isEmpty()) {
      model->setData(index, text, Qt::EditRole);
    }
  }

  void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
    const int depth    = index.data(kDepthRole).toInt();
    const bool hasMask = index.data(kHasMaskRole).toBool();
    const LayerRowRects rr = computeRowRects(option.rect, depth, hasMask);
    editor->setGeometry(rr.name);
  }
};

/// フォルダ行の中央にドロップしたとき reparent コールバックを呼ぶ QListWidget 派生クラス
class CustomLayerList : public QListWidget {
public:
  explicit CustomLayerList(QWidget* parent = nullptr) : QListWidget(parent) {}
  std::function<bool(int fromRow, uint32_t targetFolderId)> onFolderDrop;

protected:
  void dropEvent(QDropEvent* e) override {
    const QPoint pos = e->position().toPoint();
    QListWidgetItem* targetItem = itemAt(pos);
    if (targetItem != nullptr) {
      const QRect itemRect = visualItemRect(targetItem);
      const int margin = itemRect.height() / 5;
      const bool onItemCenter = (pos.y() >= itemRect.top() + margin &&
                                 pos.y() <= itemRect.bottom() - margin);
      if (onItemCenter) {
        const auto kind = static_cast<core::LayerKind>(targetItem->data(kKindRole).toInt());
        if (kind == core::LayerKind::Folder && onFolderDrop) {
          const QList<QListWidgetItem*> sel = selectedItems();
          if (!sel.isEmpty()) {
            const int fromRow = row(sel.first());
            const int toRow   = row(targetItem);
            if (fromRow != toRow) {
              const uint32_t folderId = static_cast<uint32_t>(targetItem->data(kLayerIdRole).toUInt());
              onFolderDrop(fromRow, folderId);
              e->acceptProposedAction();
              return;
            }
          }
        }
      }
    }
    QListWidget::dropEvent(e);
  }
};

} // namespace

LayerPanel::LayerPanel(QWidget* parent)
    : QWidget(parent),
      m_headerLabel(nullptr), // removed: must not exist with text matching dock windowTitle "レイヤー"
      m_filterEdit(new QLineEdit(this)),
      m_layerList(new CustomLayerList(this)),
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
      m_primaryGroup(new QGroupBox(QString(), this)),
      m_stateGroup(new QGroupBox(QString(), this)),
      m_primaryGrid(new QGridLayout()),
      m_stateGrid(new QGridLayout()) {
  // m_headerLabel is nullptr — not created
  m_primaryGroup->setTitle(QString());
  m_stateGroup->setTitle(QString());

  m_filterEdit->setPlaceholderText(QStringLiteral("レイヤーを検索..."));
  m_filterEdit->setClearButtonEnabled(true);

  m_layerList->setAlternatingRowColors(true);
  m_layerList->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_layerList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
  m_layerList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_layerList->setUniformItemSizes(true);
  m_layerList->setDragEnabled(true);
  m_layerList->viewport()->setAcceptDrops(true);
  m_layerList->setDropIndicatorShown(true);
  m_layerList->setDragDropMode(QAbstractItemView::InternalMove);
  m_layerList->setDefaultDropAction(Qt::MoveAction);
  m_layerList->setContextMenuPolicy(Qt::CustomContextMenu);
  m_layerList->setSpacing(1);
  m_layerList->setUniformItemSizes(true);
  m_layerList->setItemDelegate(new LayerItemDelegate(m_layerList));
  m_layerList->setMinimumHeight(DS::layer::kMinListH);
  m_layerList->setStyleSheet(
      QStringLiteral(
          "QListWidget::item { min-height: %1px; padding: 0px; border-bottom: 1px solid %2; }"
          "QListWidget::item:selected { background: %3; color: #ffffff; }"
          "QListWidget::item:drop { border-top: 2px solid %4; background: %5; }"
          "QListWidget::indicator { width: %6px; height: %6px; }"
          "QListWidget::indicator:checked { image: url(:/icons/16/visibility.svg); }"
          "QListWidget::indicator:unchecked { image: url(:/icons/16/visibility_off.svg); }")
      .arg(DS::layer::kRowHeight)
      .arg(DS::layer::kListItemBorder)
      .arg(DS::layer::kListItemSelected)
      .arg(DS::layer::kListDropBorder)
      .arg(DS::layer::kListDropBg)
      .arg(DS::icon::kSmall));

  m_opacitySlider->setRange(0, 100);
  m_opacitySlider->setValue(100);
  m_opacitySlider->setToolTip(QStringLiteral("アクティブレイヤーの不透明度を調整"));
  m_opacitySlider->setFixedHeight(DS::layer::kOpacitySliderH);
  m_opacitySlider->setStyleSheet(
      "QSlider::groove:horizontal { height: 4px; background: #2a2e3e; border-radius: 2px; }"
      "QSlider::sub-page:horizontal { background: #4e8ef7; border-radius: 2px; }"
      "QSlider::handle:horizontal { width: 3px; height: 8px; margin: -2px 0;"
      " background: #c8ccd6; border-radius: 1px; }");
  m_opacitySpin->setRange(0, 100);
  m_opacitySpin->setValue(100);
  m_opacitySpin->setSuffix(QStringLiteral("%"));
  m_opacitySpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
  m_opacitySpin->setAlignment(Qt::AlignRight);
  m_opacitySpin->setFixedWidth(56);   // 48 was too narrow for '100%'
  m_opacityLabel->hide();               // not in layout — must be hidden explicitly

  auto addBlend = [&](const QString& label, core::BlendMode mode) {
    m_blendModeCombo->addItem(label, static_cast<int>(mode));
  };
  addBlend(QStringLiteral("通常"),               core::BlendMode::Normal);
  m_blendModeCombo->insertSeparator(m_blendModeCombo->count());
  addBlend(QStringLiteral("暗く"),               core::BlendMode::Darken);
  addBlend(QStringLiteral("乗算"),               core::BlendMode::Multiply);
  addBlend(QStringLiteral("焼き込みカラー"),     core::BlendMode::ColorBurn);
  addBlend(QStringLiteral("焼き込み（リニア）"), core::BlendMode::LinearBurn);
  m_blendModeCombo->insertSeparator(m_blendModeCombo->count());
  addBlend(QStringLiteral("明るく"),             core::BlendMode::Lighten);
  addBlend(QStringLiteral("スクリーン"),         core::BlendMode::Screen);
  addBlend(QStringLiteral("覆い焼きカラー"),     core::BlendMode::ColorDodge);
  addBlend(QStringLiteral("加算"),               core::BlendMode::LinearDodge);
  m_blendModeCombo->insertSeparator(m_blendModeCombo->count());
  addBlend(QStringLiteral("オーバーレイ"),       core::BlendMode::Overlay);
  addBlend(QStringLiteral("ソフトライト"),       core::BlendMode::SoftLight);
  addBlend(QStringLiteral("ハードライト"),       core::BlendMode::HardLight);
  addBlend(QStringLiteral("ビビッドライト"),     core::BlendMode::VividLight);
  addBlend(QStringLiteral("リニアライト"),       core::BlendMode::LinearLight);
  m_blendModeCombo->insertSeparator(m_blendModeCombo->count());
  addBlend(QStringLiteral("差の絶対値"),         core::BlendMode::Difference);
  addBlend(QStringLiteral("除外"),               core::BlendMode::Exclusion);
  addBlend(QStringLiteral("減算"),               core::BlendMode::Subtract);
  addBlend(QStringLiteral("除算"),               core::BlendMode::Divide);
  m_blendModeCombo->insertSeparator(m_blendModeCombo->count());
  addBlend(QStringLiteral("色相"),               core::BlendMode::Hue);
  addBlend(QStringLiteral("彩度"),               core::BlendMode::HslSat);
  addBlend(QStringLiteral("カラー"),             core::BlendMode::HslColor);
  addBlend(QStringLiteral("輝度"),               core::BlendMode::Luminosity);

  auto initButton = [](QPushButton* button, const QString& iconName, const QString& fullText) {
    button->setIcon(app::ui::icon(iconName));
    button->setProperty("fullText", fullText);
    button->setProperty("shortText", QString());
    button->setIconSize(QSize(DS::icon::kSmall, DS::icon::kSmall));
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setMinimumHeight(DS::button::kHeightM);
    button->setText(fullText);
  };

  initButton(m_addRasterButton, QStringLiteral("layer_raster_add"), QStringLiteral("ラスタ追加"));
  initButton(m_addVectorButton, QStringLiteral("layer_vector_add"), QStringLiteral("ベクター追加"));
  initButton(m_addFolderButton, QStringLiteral("folder_add"), QStringLiteral("フォルダ追加"));
  initButton(m_duplicateButton, QStringLiteral("duplicate"), QStringLiteral("複製"));
  initButton(m_upButton, QStringLiteral("up"), QStringLiteral("上へ"));
  initButton(m_downButton, QStringLiteral("down"), QStringLiteral("下へ"));
  initButton(m_deleteButton, QStringLiteral("delete"), QStringLiteral("削除"));
  initButton(m_clipButton, QStringLiteral("clip"), QStringLiteral("クリップ"));
  initButton(m_maskButton, QStringLiteral("mask"), QStringLiteral("マスク"));
  initButton(m_removeMaskButton, QStringLiteral("mask_remove"), QStringLiteral("マスク解除"));
  initButton(m_lockButton, QStringLiteral("lock"), QStringLiteral("ロック"));
  initButton(m_lockAlphaButton, QStringLiteral("lock_alpha"), QStringLiteral("透明保護"));
  initButton(m_lockPositionButton, QStringLiteral("lock_position"), QStringLiteral("位置固定"));
  // トグルボタンは checkable にして ON/OFF 状態を視覚的に表示する
  m_clipButton->setCheckable(true);
  m_lockButton->setCheckable(true);
  m_lockAlphaButton->setCheckable(true);
  m_lockPositionButton->setCheckable(true);
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
    button->setFixedSize(20, 20);
    button->setIconSize(QSize(19, 19));
    button->setFocusPolicy(Qt::NoFocus);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    button->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            " margin: 0px; padding: 0px;"
            " border: 1px solid %1; border-radius: 0px;"
            " background: %3; color: %4;"
            "}"
            "QPushButton:hover { background: %5; border-color: %6; }"
            "QPushButton:pressed { background: %7; border-color: %8; }"
            "QPushButton:checked { background: %9; border-color: %10; }"
            "QPushButton:disabled { color: %11; border-color: %12; background: %13; }")
        .arg(DS::layer::kIconBtnBorder)
        .arg(2)
        .arg(DS::layer::kIconBtnBg)
        .arg(DS::theme::text::kLabel)
        .arg(DS::layer::kIconBtnHoverBg)
        .arg(DS::layer::kIconBtnHoverBorder)
        .arg(DS::layer::kIconBtnPressedBg)
        .arg(DS::layer::kIconBtnPressedBorder)
        .arg(DS::layer::kIconBtnCheckedBg)
        .arg(DS::layer::kIconBtnCheckedBorder)
        .arg(DS::layer::kIconBtnDisabledText)
        .arg(DS::layer::kIconBtnDisabledBorder)
        .arg(DS::layer::kIconBtnDisabledBg));
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


  auto tuneLayerButtonGroup = [](QGroupBox* group, QGridLayout* grid) {
    if (group == nullptr || grid == nullptr) {
      return;
    }

    group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    group->setStyleSheet(QStringLiteral(
        "QGroupBox {"
        " margin-top: 0px;"
        " padding: 0px 0px 0px 0px;"
        " border: none;"
        " background: transparent;"
        "}"));

    grid->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    grid->setContentsMargins(DS::margin::kNone, DS::margin::kNone,
                              DS::margin::kNone, DS::margin::kNone);
    grid->setHorizontalSpacing(DS::spacing::k1);
    grid->setVerticalSpacing(DS::spacing::k1);
    for (int col = 0; col < 8; ++col) {
      grid->setColumnStretch(col, 0);
    }
    for (int row = 0; row < 4; ++row) {
      grid->setRowStretch(row, 0);
    }
  };

  tuneLayerButtonGroup(m_primaryGroup, m_primaryGrid);
  tuneLayerButtonGroup(m_stateGroup, m_stateGrid);

  // CSP order — Row 0: state/lock; Row 1: create/manage
  // CSP order: clip → lockAlpha → lock → lockPosition → mask
  m_primaryGrid->addWidget(m_clipButton,        0, 0, Qt::AlignLeft | Qt::AlignTop);
  m_primaryGrid->addWidget(m_lockAlphaButton,   0, 1, Qt::AlignLeft | Qt::AlignTop);
  m_primaryGrid->addWidget(m_lockButton,        0, 2, Qt::AlignLeft | Qt::AlignTop);
  m_primaryGrid->addWidget(m_lockPositionButton,0, 3, Qt::AlignLeft | Qt::AlignTop);
  m_primaryGrid->addWidget(m_maskButton,        0, 4, Qt::AlignLeft | Qt::AlignTop);
  m_primaryGroup->setLayout(m_primaryGrid);

  // CSP: all state buttons in one row (raster→vector→folder→dup→delete→up→down→removeMask)
  m_stateGrid->addWidget(m_addRasterButton, 0, 0, Qt::AlignLeft | Qt::AlignTop);
  m_stateGrid->addWidget(m_addVectorButton, 0, 1, Qt::AlignLeft | Qt::AlignTop);
  m_stateGrid->addWidget(m_addFolderButton, 0, 2, Qt::AlignLeft | Qt::AlignTop);
  m_stateGrid->addWidget(m_duplicateButton, 0, 3, Qt::AlignLeft | Qt::AlignTop);
  m_stateGrid->addWidget(m_deleteButton,    0, 4, Qt::AlignLeft | Qt::AlignTop);
  m_stateGrid->addWidget(m_upButton,        0, 5, Qt::AlignLeft | Qt::AlignTop);
  m_stateGrid->addWidget(m_downButton,      0, 6, Qt::AlignLeft | Qt::AlignTop);
  m_stateGrid->addWidget(m_removeMaskButton,0, 7, Qt::AlignLeft | Qt::AlignTop);
  m_stateGroup->setLayout(m_stateGrid);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(DS::margin::kNormal, DS::margin::kNormal,
                              DS::margin::kNormal, DS::margin::kNormal);
  layout->setSpacing(DS::spacing::kNarrow);
  auto* opacityRow = new QHBoxLayout();
  opacityRow->setContentsMargins(DS::margin::kNone, DS::margin::kNone,
                                  DS::margin::kNone, DS::margin::kNone);
  opacityRow->setSpacing(DS::spacing::kNarrow);
  opacityRow->addWidget(m_opacitySlider, 1);
  opacityRow->addWidget(m_opacitySpin, 0);
  // m_headerLabel deleted — never add to layout
  // Bottom button container — zero gap between primaryGroup and stateGroup
  auto* bottomButtons = new QWidget(this);
  auto* bbl = new QVBoxLayout(bottomButtons);
  bbl->setContentsMargins(0, 0, 0, 0);
  bbl->setSpacing(0);
  bbl->addWidget(m_primaryGroup);   // lock/alpha/clip/mask/position
  bbl->addWidget(m_stateGroup);     // add/duplicate/delete/up/down

  layout->addWidget(m_filterEdit);
  layout->addWidget(m_blendModeCombo);   // CSP: blend mode
  layout->addLayout(opacityRow);
  layout->addWidget(bottomButtons);       // lock/clip + add/delete above list
  layout->addWidget(m_layerList, 1);      // list at the very bottom

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
  connect(m_layerList, &QListWidget::itemSelectionChanged, this, &LayerPanel::onLayerItemSelectionChanged);
  auto* layerThumbnailRefreshTimer = new QTimer(this);
  layerThumbnailRefreshTimer->setObjectName(QStringLiteral("layerThumbnailRefreshTimer"));
  layerThumbnailRefreshTimer->setSingleShot(true);
  layerThumbnailRefreshTimer->setInterval(33);
  connect(layerThumbnailRefreshTimer, &QTimer::timeout, this, &LayerPanel::refreshLayers);

  connect(m_filterEdit, &QLineEdit::textChanged, this, &LayerPanel::onFilterTextChanged);
  connect(m_layerList, &QListWidget::customContextMenuRequested, this, &LayerPanel::onLayerContextMenuRequested);
  connect(m_layerList->model(), &QAbstractItemModel::rowsMoved, this, &LayerPanel::onLayerRowsMoved);
  // Delegate edit-target / mask toggle signals
  connect(m_layerList->model(), &QAbstractItemModel::dataChanged,
          this, [this](const QModelIndex& topLeft, const QModelIndex&, const QVector<int>& roles) {
    if (m_controller == nullptr || m_isRefreshing) return;
    if (roles.contains(kExpandToggleRole)) {
      const QListWidgetItem* item = m_layerList->item(topLeft.row());
      if (item != nullptr) {
        const uint32_t fid = static_cast<uint32_t>(item->data(kLayerIdRole).toUInt());
        if (m_collapsedFolderIds.count(fid)) {
          m_collapsedFolderIds.erase(fid);
        } else {
          m_collapsedFolderIds.insert(fid);
        }
        QTimer::singleShot(0, this, &LayerPanel::refreshLayers);
      }
      return;
    }
    if (!roles.contains(kEditTargetRole)) return;
    const QListWidgetItem* item = m_layerList->item(topLeft.row());
    if (item == nullptr) return;
    const int val = item->data(kEditTargetRole).toInt();
    if (val == 0) {
      m_controller->setEditTarget(app::ui::UiState::EditTarget::Image);
    } else if (val == 1) {
      m_controller->setEditTarget(app::ui::UiState::EditTarget::Mask);
    } else if (val == 2) {
      m_controller->enableLayerMask(true);
    } else if (val == 3) {
      m_controller->enableLayerMask(false);
    }
  });
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
    button->setFixedSize(20, 20);
    button->setIconSize(QSize(19, 19));
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  }

  m_compactButtons = true;
}void LayerPanel::applyResponsiveMode() {
  const bool compactWidth  = width()  < DS::layerPanel::kCompactWidth;
  const bool compactHeight = height() < DS::layerPanel::kCompactHeight;

  applyButtonCompactMode(compactWidth);

  if (m_stateGroup != nullptr) {
    m_stateGroup->setVisible(true);
  }

  if (m_primaryGrid != nullptr) {
    m_primaryGrid->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    for (int col = 0; col < 8; ++col) {
      m_primaryGrid->setColumnStretch(col, 0);
    }
    for (int row = 0; row < 4; ++row) {
      m_primaryGrid->setRowStretch(row, 0);
    }
  }

  if (m_stateGrid != nullptr) {
    m_stateGrid->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    for (int col = 0; col < 8; ++col) {
      m_stateGrid->setColumnStretch(col, 0);
    }
    for (int row = 0; row < 4; ++row) {
      m_stateGrid->setRowStretch(row, 0);
    }
  }

  if (m_layerList != nullptr) {
    m_layerList->setMinimumHeight(compactHeight ? DS::layer::kMinListHCompact : DS::layer::kMinListH);
  }
}void LayerPanel::setController(app::bridge::AppController* controller) {
  if (m_controller != nullptr) {
    disconnect(m_controller, nullptr, this, nullptr);
  }

  m_controller = controller;
  if (m_controller == nullptr) {
    return;
  }

  connect(m_controller, &app::bridge::AppController::layersChanged, this, &LayerPanel::refreshLayers);

  // フォルダへのドロップで reparent
  if (auto* customList = static_cast<CustomLayerList*>(m_layerList)) {
    customList->onFolderDrop = [this](int fromRow, uint32_t folderId) -> bool {
      if (m_controller == nullptr) return false;
      const std::size_t layerIdx = layerIndexFromRow(fromRow);
      m_controller->setLayerParent(layerIdx, folderId);
      return true;
    };
  }

  connect(m_controller, &app::bridge::AppController::documentChanged, this, [this]() {
    if (m_isRefreshing) {
      return;
    }

    auto* timer = findChild<QTimer*>(QStringLiteral("layerThumbnailRefreshTimer"));
    if (timer != nullptr) {
      timer->start();
      return;
    }

    refreshLayers();
  });
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
  // 各レイヤーの深さを計算（parentId チェーンを逆向きに辿る、順序非依存、循環安全）
  //
  // アルゴリズム:
  //   1. parentId の高速ルックアップマップを構築
  //   2. 各レイヤーについて root または計算済みノードに到達するまでチェーンを収集
  //   3. チェーンの末尾（root 側）から深さを埋めてメモ化
  //   4. 循環は「チェーン長が全レイヤー数を超えたら停止して 0 にフォールバック」で安全処理
  //
  // これにより Document 内のフォルダ/子の並び順に依存しない。
  std::unordered_map<uint32_t, uint32_t> parentOf;  // layerId → parentId
  parentOf.reserve(models.size());
  for (const auto& m : models) {
    if (!m.paperLayer && m.layerId != 0) {
      parentOf[m.layerId] = m.parentId;
    }
  }

  std::unordered_map<uint32_t, int> idToDepth;
  idToDepth.reserve(models.size());

  const int kCycleGuard = static_cast<int>(models.size()) + 1;

  for (const auto& m : models) {
    if (m.paperLayer || m.layerId == 0) {
      continue;
    }
    if (idToDepth.count(m.layerId)) {
      continue;  // 既に計算済み（別レイヤーのチェーン処理で埋まっている場合）
    }

    // root または計算済みノードに到達するまでチェーンを積む
    std::vector<uint32_t> chain;
    uint32_t cur = m.layerId;

    while (cur != 0 && !idToDepth.count(cur)) {
      if (static_cast<int>(chain.size()) >= kCycleGuard) {
        // 循環ガード発動: チェーン内を全て depth=0 に設定してスキップ
        for (const uint32_t cid : chain) {
          idToDepth.emplace(cid, 0);
        }
        chain.clear();
        break;
      }
      chain.push_back(cur);
      const auto pit = parentOf.find(cur);
      cur = (pit != parentOf.end()) ? pit->second : 0;
    }

    if (chain.empty()) {
      continue;
    }

    // チェーンの末尾が接続するノードの深さを決定
    // cur == 0 の場合は root に到達 → baseDepth = -1（chain.back() の深さは 0）
    // cur が計算済みの場合 → chain.back() の深さは idToDepth[cur] + 1
    const int baseDepth = idToDepth.count(cur) ? idToDepth.at(cur) : -1;

    // chain[i] の深さ = baseDepth + 1 + (chain.size() - 1 - i)
    for (int i = static_cast<int>(chain.size()) - 1; i >= 0; --i) {
      idToDepth[chain[static_cast<std::size_t>(i)]] =
          baseDepth + 1 + (static_cast<int>(chain.size()) - 1 - i);
    }
  }

  // ── 折りたたみによる非表示セット ────────────────────────────────────────
  std::unordered_set<uint32_t> hiddenByCollapse;
  for (const auto& m : models) {
    if (m.paperLayer || m.layerId == 0) continue;
    uint32_t cur = m.parentId;
    bool hidden = false;
    const int kGuard = static_cast<int>(models.size()) + 1;
    int guard = 0;
    while (cur != 0 && !hidden && guard < kGuard) {
      if (m_collapsedFolderIds.count(cur)) { hidden = true; }
      const auto pit = parentOf.find(cur);
      cur = (pit != parentOf.end()) ? pit->second : 0;
      ++guard;
    }
    if (hidden) hiddenByCollapse.insert(m.layerId);
  }

  // ── 親チェーンを含む effective visibility マップ ──────────────────────
  std::unordered_map<uint32_t, bool> effectivelyVisible;
  effectivelyVisible.reserve(models.size());
  for (const auto& m : models) {
    if (m.paperLayer) continue;
    effectivelyVisible[m.layerId] = m.visible;
  }
  for (const auto& m : models) {
    if (m.paperLayer || m.layerId == 0) continue;
    bool ev = effectivelyVisible[m.layerId];
    uint32_t cur = m.parentId;
    const int kGuard2 = static_cast<int>(models.size()) + 1;
    int guard2 = 0;
    while (cur != 0 && ev && guard2 < kGuard2) {
      const auto it = effectivelyVisible.find(cur);
      if (it != effectivelyVisible.end() && !it->second) ev = false;
      const auto pit = parentOf.find(cur);
      cur = (pit != parentOf.end()) ? pit->second : 0;
      ++guard2;
    }
    effectivelyVisible[m.layerId] = ev;
  }

  for (std::size_t layerIndex = models.size(); layerIndex-- > 0;) {
    const auto& model = models[layerIndex];
    if (hasFilter) {
      const QString layerName = QString::fromStdString(model.name);
      if (!layerName.contains(filterText, Qt::CaseInsensitive) && !(model.paperLayer && QStringLiteral("用紙").contains(filterText))) {
        continue;
      }
    }
    // 折りたたまれたフォルダ内のレイヤーはスキップ
    if (!model.paperLayer && model.layerId != 0 && hiddenByCollapse.count(model.layerId)) {
      continue;
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
    // editTarget: 0=Image, 1=Mask (only meaningful for active layer)
    const int editTargetVal = (model.active && m_controller != nullptr)
        ? (m_controller->editTarget() == app::ui::UiState::EditTarget::Mask ? 1 : 0)
        : 0;
    item->setData(kEditTargetRole, editTargetVal);
    // Mask thumbnail pixmap
    if (model.hasMask && !model.paperLayer) {
      const std::size_t realLayerIdx = static_cast<std::size_t>(
          model.paperLayer ? -1 : static_cast<int>(layerIndex - 1));
      item->setData(Qt::UserRole + 100,
                    QVariant::fromValue(maskThumbnailPixmap(m_controller, realLayerIdx)));
    }
    item->setData(kLockedRole, model.locked);
    item->setData(kAlphaLockedRole, model.alphaLocked);
    item->setData(kPositionLockedRole, model.positionLocked);
    item->setData(kBlendModeRole, static_cast<int>(model.blendMode));
    item->setData(kPaperRole, model.paperLayer);
    item->setData(kActiveRole, model.active);

    // 階層表示用ロール
    const int itemDepth = (!model.paperLayer && model.layerId != 0)
        ? [&]() -> int {
            const auto it = idToDepth.find(model.layerId);
            return it != idToDepth.end() ? it->second : 0;
          }()
        : 0;
    item->setData(kLayerIdRole,  static_cast<uint>(model.layerId));
    item->setData(kParentIdRole, static_cast<uint>(model.parentId));
    item->setData(kDepthRole,    itemDepth);

    // フォルダ展開状態（Folder 以外は常に true）
    const bool isExpanded = (model.kind != core::LayerKind::Folder) ||
                            !m_collapsedFolderIds.count(model.layerId);
    item->setData(kExpandedRole, isExpanded);
    // 親チェーンを含む effective visibility
    const bool effVis = (!model.paperLayer && model.layerId != 0)
        ? (effectivelyVisible.count(model.layerId) ? effectivelyVisible.at(model.layerId) : model.visible)
        : model.visible;
    item->setData(kEffVisibleRole, effVis);

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

  // QSignalBlocker はまだ有効 — setSelected はシグナルを発しない
  // コントローラの選択セットに基づいてアイテムの選択状態を復元する
  if (m_controller != nullptr) {
    const auto& selectedIds = m_controller->selectedLayerIds();
    if (!selectedIds.empty()) {
      for (int r = 0; r < m_layerList->count(); ++r) {
        QListWidgetItem* rowItem = m_layerList->item(r);
        if (rowItem == nullptr) {
          continue;
        }
        const auto lid = static_cast<uint32_t>(rowItem->data(kLayerIdRole).toUInt());
        if (lid != 0 && selectedIds.count(lid) > 0) {
          rowItem->setSelected(true);
        }
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

  const QList<QListWidgetItem*> selectedItems = m_layerList->selectedItems();

  // マルチ選択時: layerId を収集して一括削除（1回の rerender + layersChanged）
  if (selectedItems.size() > 1) {
    std::vector<uint32_t> idsToDelete;
    idsToDelete.reserve(static_cast<std::size_t>(selectedItems.size()));
    for (QListWidgetItem* selItem : selectedItems) {
      if (selItem->data(kPaperRole).toBool()) {
        continue;
      }
      const auto lid = static_cast<uint32_t>(selItem->data(kLayerIdRole).toUInt());
      if (lid != 0) {
        idsToDelete.push_back(lid);
      }
    }
    if (!idsToDelete.empty()) {
      m_controller->removeLayersByIds(idsToDelete);
    }
    return;
  }

  // シングル選択: 従来の動作を維持
  QListWidgetItem* currentItem = m_layerList->currentItem();
  if (currentItem == nullptr) {
    return;
  }
  const int layerIndex = currentItem->data(kLayerIndexRole).toInt();
  if (layerIndex < 0) {
    // 用紙レイヤー: 非表示にすることで「削除」と同等の効果を持たせる
    m_controller->setPaperVisible(false);
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
  menu.addSeparator();
  // フォルダに移動サブメニュー（フォルダレイヤーが存在する場合のみ有効）
  QMenu* moveToFolderMenu = menu.addMenu(app::ui::icon(QStringLiteral("folder")), QStringLiteral("フォルダに移動"));
  {
    const auto allModels = m_controller->layerViewModels();
    bool hasFolders = false;
    for (std::size_t mi = 0; mi < allModels.size(); ++mi) {
      const auto& fm = allModels[mi];
      if (!fm.paperLayer && fm.kind == core::LayerKind::Folder) {
        hasFolders = true;
        QAction* folderAction = moveToFolderMenu->addAction(
            app::ui::icon(QStringLiteral("folder")),
            QString::fromStdString(fm.name));
        folderAction->setData(static_cast<uint>(fm.layerId));
      }
    }
    if (hasFolders) {
      moveToFolderMenu->addSeparator();
    }
    // 「フォルダから外す」選択肢（ルートに移動）
    QAction* removeFromFolderAction = moveToFolderMenu->addAction(QStringLiteral("フォルダから外す（ルートへ）"));
    removeFromFolderAction->setData(static_cast<uint>(0));
    moveToFolderMenu->setEnabled(canEditLayer);
  }

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
  if (selected == toggleVisibleAction && hasSelection) {
    const QList<QListWidgetItem*> visItems = m_layerList->selectedItems();
    if (visItems.size() > 1) {
      // マルチ選択: 全選択アイテムの表示を切り替える
      for (QListWidgetItem* vi : visItems) {
        const int idx = vi->data(kLayerIndexRole).toInt();
        if (idx >= 0) {
          const bool nowVisible = vi->data(kVisibilityRole).toBool();
          m_controller->setLayerVisible(static_cast<std::size_t>(idx), !nowVisible);
        }
      }
    } else if (m_layerList->currentItem() != nullptr) {
      QListWidgetItem* current = m_layerList->currentItem();
      current->setCheckState(current->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
    }
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
  // フォルダに移動サブメニューのアクション
  if (moveToFolderMenu->actions().contains(selected) && m_layerList->currentItem() != nullptr) {
    const int layerIndex = m_layerList->currentItem()->data(kLayerIndexRole).toInt();
    if (layerIndex >= 0) {
      const uint32_t newParentId = static_cast<uint32_t>(selected->data().toUInt());
      m_controller->setLayerParent(static_cast<std::size_t>(layerIndex), newParentId);
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

void LayerPanel::onLayerItemSelectionChanged() {
  if (m_controller == nullptr || m_isRefreshing) {
    return;
  }
  // 現在の視覚的選択セットを layerId に変換して AppController に同期する
  std::unordered_set<uint32_t> ids;
  for (QListWidgetItem* selItem : m_layerList->selectedItems()) {
    const auto lid = static_cast<uint32_t>(selItem->data(kLayerIdRole).toUInt());
    if (lid != 0) {
      ids.insert(lid);
    }
  }
  m_controller->setSelectedLayerIds(ids);
  refreshButtonState();
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

  QListWidgetItem* currentItem = m_layerList->currentItem();
  const bool hasSelection = currentItem != nullptr;
  const bool canDelete = true;
  const int current = m_layerList->currentRow();
  const int lastRow = static_cast<int>(m_controller->document().layerCount()) - 1;
  const bool paperSelected = currentItem != nullptr && currentItem->data(kPaperRole).toBool();

  const bool canMoveUp = current > 0;
  const bool canMoveDown = current >= 0 && current < lastRow;

  // 用紙レイヤー: 削除(→非表示化)は許可。移動は常に背面固定なので不可。
  // opacity/blendMode は用紙には非適用。
  m_deleteButton->setEnabled(hasSelection && (paperSelected || canDelete));
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
        const int opacityPct = static_cast<int>(
            std::lround(std::clamp(layer.opacity(), 0.0F, 1.0F) * 100.0F));
        const QSignalBlocker sliderBlocker(m_opacitySlider);
        const QSignalBlocker spinBlocker(m_opacitySpin);
        m_opacitySlider->setValue(opacityPct);
        m_opacitySpin->setValue(opacityPct);
        m_opacityLabel->setText(QStringLiteral("不透明度: %1%").arg(opacityPct));
      }
    }
  }

  m_clipButton->setEnabled(canClipOrMask);
  m_maskButton->setEnabled(canClipOrMask);
  m_removeMaskButton->setEnabled(canClipOrMask && hasMask);
  m_lockButton->setEnabled(hasSelection && kind != core::LayerKind::Folder);
  m_lockAlphaButton->setEnabled(hasSelection && kind == core::LayerKind::Raster);
  m_lockPositionButton->setEnabled(hasSelection && kind != core::LayerKind::Folder);

  // トグル状態をボタンの checked 状態に反映（setChecked は clicked を emit しない）
  {
    const QSignalBlocker b1(m_clipButton);
    const QSignalBlocker b2(m_lockButton);
    const QSignalBlocker b3(m_lockAlphaButton);
    const QSignalBlocker b4(m_lockPositionButton);
    m_clipButton->setChecked(clipped);
    m_lockButton->setChecked(locked);
    m_lockAlphaButton->setChecked(alphaLocked);
    m_lockPositionButton->setChecked(positionLocked);
  }

}

} // namespace app::panels
