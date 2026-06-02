#include "app/panels/SubToolPanel.h"

#include <cmath>

#include <QAbstractItemView>
#include <QAction>
#include <QColor>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QToolButton>
#include <QVBoxLayout>

#include "app/bridge/AppController.h"
#include "app/ui/IconLoader.h"

namespace app::panels {

namespace {
constexpr int kSubToolIdRole = Qt::UserRole;
constexpr int kSubToolEnabledRole = Qt::UserRole + 1;

static bool isSelectionSubTool(const QString& id) {
  return id == "rect_default" || id == "lasso_default" || id == "poly_lasso"
      || id == "auto_select" || id == "object_select";
}

static void drawSelectionIcon(QPainter* painter, const QString& id, const QRectF& r, bool selected, bool enabled) {
  const qreal alpha = enabled ? 1.0 : 0.35;
  QColor dashColor = selected ? QColor(255, 255, 255, int(200 * alpha))
                              : QColor(190, 215, 255, int(170 * alpha));
  QColor dotColor  = selected ? QColor(255, 255, 255, int(240 * alpha))
                              : QColor(140, 180, 255, int(200 * alpha));

  QPen dashPen(dashColor, 1.5, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
  dashPen.setDashPattern({4, 3});

  if (id == "rect_default") {
    // Dashed rectangle centred in preview area
    const qreal m = 6.0;
    const QRectF box(r.left() + m, r.top() + m, r.width() - m * 2, r.height() - m * 2);
    painter->setPen(dashPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(box);
    // Corner dots
    painter->setPen(Qt::NoPen);
    painter->setBrush(dotColor);
    for (QPointF c : {box.topLeft(), box.topRight(), box.bottomLeft(), box.bottomRight()})
      painter->drawEllipse(c, 2.5, 2.5);

  } else if (id == "lasso_default") {
    // Freehand wavy closed dashed curve
    const qreal cx = r.center().x(), cy = r.center().y();
    const qreal rx = r.width() * 0.36, ry = r.height() * 0.32;
    QPainterPath path;
    path.moveTo(cx,         cy - ry);
    path.cubicTo(cx + rx * 1.3, cy - ry * 0.4, cx + rx * 1.1, cy + ry * 0.6, cx + rx * 0.1, cy + ry);
    path.cubicTo(cx - rx * 0.9, cy + ry * 1.2, cx - rx * 1.4, cy + ry * 0.2, cx - rx * 1.0, cy - ry * 0.4);
    path.cubicTo(cx - rx * 0.7, cy - ry * 1.0, cx - rx * 0.1, cy - ry * 1.1, cx, cy - ry);
    painter->setPen(dashPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path);

  } else if (id == "poly_lasso") {
    // Polygon outline with vertex dots
    const qreal cx = r.center().x(), cy = r.center().y();
    const qreal s  = qMin(r.width(), r.height()) * 0.38;
    QVector<QPointF> pts = {
        {cx,          cy - s},
        {cx + s,      cy - s * 0.2},
        {cx + s * 0.6, cy + s},
        {cx - s * 0.5, cy + s},
        {cx - s,      cy - s * 0.3},
    };
    QPainterPath poly;
    poly.moveTo(pts[0]);
    for (int i = 1; i < pts.size(); ++i) poly.lineTo(pts[i]);
    poly.closeSubpath();
    painter->setPen(dashPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(poly);
    painter->setPen(Qt::NoPen);
    painter->setBrush(dotColor);
    for (const QPointF& p : pts) painter->drawEllipse(p, 2.5, 2.5);

  } else if (id == "auto_select") {
    // Magic-wand: wand stick + sparkle rays
    const qreal cx = r.center().x() - r.width() * 0.06;
    const qreal cy = r.center().y() + r.height() * 0.06;
    // Wand stick
    QPen stickPen(dashColor, 2.0, Qt::SolidLine, Qt::RoundCap);
    painter->setPen(stickPen);
    painter->drawLine(QPointF(cx, cy), QPointF(r.right() - 4, r.bottom() - 4));
    // Sparkle star at top
    painter->setPen(QPen(dotColor, 1.5, Qt::SolidLine, Qt::RoundCap));
    const double sx = r.left() + r.width() * 0.30;
    const double sy = r.top()  + r.height() * 0.28;
    const double sr = r.width() * 0.20;
    const double sr2 = sr * 0.45;
    for (int i = 0; i < 8; ++i) {
      const double angle = i * M_PI / 4.0;
      const double rr = (i % 2 == 0) ? sr : sr2;
      painter->drawLine(
          QPointF(sx, sy),
          QPointF(sx + std::cos(angle) * rr, sy + std::sin(angle) * rr));
    }
    painter->setPen(Qt::NoPen);
    painter->setBrush(dotColor);
    painter->drawEllipse(QPointF(sx, sy), 2.2, 2.2);

  } else if (id == "object_select") {
    // Rectangle with light selection fill
    const qreal m = 7.0;
    const QRectF box(r.left() + m, r.top() + m, r.width() - m * 2, r.height() - m * 2);
    QColor fill = selected ? QColor(80, 140, 255, 50) : QColor(60, 110, 220, 35);
    painter->setBrush(fill);
    painter->setPen(dashPen);
    painter->drawRect(box);
    painter->setBrush(Qt::NoBrush);
  }
}

QPixmap makeStrokePreview(
    const QString& subToolId,
    const QColor& color,
    const QSize& size,
    bool enabled) {
  QPixmap pixmap(size);
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const QString id = subToolId.toLower();
  if (isSelectionSubTool(id)) {
    const QRectF r(0, 0, size.width(), size.height());
    drawSelectionIcon(&painter, id, r, false, enabled);
    return pixmap;
  }

  QColor penColor = color;
  if (!enabled) {
    penColor = QColor(120, 126, 137);
  }
  qreal width = 2.6;
  if (id.contains("hard")) {
    width = 3.6;
  } else if (id.contains("soft") || id.contains("airbrush")) {
    width = 2.2;
    penColor.setAlpha(200);
  } else if (id.contains("vector")) {
    width = 2.0;
  } else if (id.contains("fill")) {
    width = 5.2;
  }

  if (id.contains("eraser")) {
    penColor = QColor(210, 218, 232, enabled ? 220 : 130);
  }

  QPen pen(penColor, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  painter.setPen(pen);

  const qreal w = static_cast<qreal>(size.width());
  const qreal h = static_cast<qreal>(size.height());
  QPainterPath path;
  path.moveTo(2.0, h * 0.72);
  path.cubicTo(w * 0.28, h * 0.20, w * 0.62, h * 0.88, w - 2.0, h * 0.34);
  painter.drawPath(path);
  return pixmap;
}

QWidget* makeSubToolRowWidget(
    QListWidget* list,
    const QString& subToolId,
    const QString& localizedName,
    bool enabled) {
  auto* rowWidget = new QWidget(list);
  rowWidget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  auto* rowLayout = new QHBoxLayout(rowWidget);
  rowLayout->setContentsMargins(3, 1, 3, 1);
  rowLayout->setSpacing(6);

  auto* preview = new QLabel(rowWidget);
  preview->setFixedSize(56, 16);
  preview->setPixmap(makeStrokePreview(subToolId, QColor(233, 238, 248), preview->size(), enabled));
  preview->setStyleSheet("background: transparent;");

  auto* text = new QLabel(localizedName, rowWidget);
  text->setStyleSheet(enabled ? "color: #dfe6f5;" : "color: #818a97;");
  text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  rowLayout->addWidget(preview, 0, Qt::AlignVCenter);
  rowLayout->addWidget(text, 1, Qt::AlignVCenter);
  return rowWidget;
}

class SubToolDelegate : public QStyledItemDelegate {
public:
  using QStyledItemDelegate::QStyledItemDelegate;

  void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered  = option.state & QStyle::State_MouseOver;
    const bool enabled  = index.data(kSubToolEnabledRole).toBool();
    const QString id    = index.data(kSubToolIdRole).toString().toLower();

    const QRect rect = option.rect.adjusted(2, 2, -2, -2);

    // ── Card background ──────────────────────────────────────────────────
    QColor bg = selected ? QColor(0x1d, 0x3a, 0x7a)
              : hovered  ? QColor(0x27, 0x2c, 0x3c)
              :             QColor(0x1a, 0x1d, 0x27);
    if (!enabled && !selected) {
      bg = QColor(0x16, 0x18, 0x20);
    }
    QPainterPath cardPath;
    cardPath.addRoundedRect(rect, 5, 5);
    painter->fillPath(cardPath, bg);

    // Card border
    QColor border = selected ? QColor(0x4e, 0x8e, 0xf7, 200)
                  : hovered  ? QColor(0x36, 0x3d, 0x54, 180)
                  :             QColor(0x2a, 0x2e, 0x3e, 120);
    painter->setPen(QPen(border, selected ? 1.5 : 1.0));
    painter->drawPath(cardPath);

    // ── Compact row layout: mini-stroke icon left, text right ────────────
    const int iconW = 28;
    const QRectF iconRect(rect.left() + 2.0, rect.top() + 2.0, iconW - 4.0, rect.height() - 4.0);
    const QRect  textRect(rect.left() + iconW + 2, rect.top(), rect.width() - iconW - 4, rect.height());

    // Mini stroke preview in icon area
    painter->save();
    painter->setClipRect(iconRect);
    painter->setRenderHint(QPainter::Antialiasing, true);
    if (isSelectionSubTool(id)) {
      drawSelectionIcon(painter, id, iconRect, selected, enabled);
    } else {
      QColor stroke = selected ? QColor(255, 255, 255, 200) : QColor(210, 225, 248, 160);
      qreal penW = 2.0;
      if (id.contains("hard")) { penW = 3.5; stroke.setAlpha(selected ? 220 : 180); }
      else if (id.contains("soft")) { penW = 5.0; stroke.setAlpha(selected ? 110 : 75); }
      else if (id.contains("airbrush")) { penW = 7.0; stroke.setAlpha(selected ? 80 : 55); }
      else if (id.contains("fill")) { penW = 8.0; stroke.setAlpha(selected ? 110 : 70); }
      else if (id.contains("vector")) { penW = 1.5; stroke.setAlpha(selected ? 230 : 170); }
      else if (id.contains("eraser")) { stroke = QColor(230, 235, 245, selected ? 160 : 105); penW = 3.0; }
      if (!enabled) stroke.setAlpha(stroke.alpha() / 4);
      painter->setPen(QPen(stroke, penW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
      QPainterPath path;
      path.moveTo(iconRect.left(), iconRect.center().y() + iconRect.height() * 0.15);
      path.cubicTo(
          iconRect.left() + iconRect.width() * 0.25, iconRect.top() + 1,
          iconRect.left() + iconRect.width() * 0.65, iconRect.bottom() - 1,
          iconRect.right(), iconRect.center().y() - iconRect.height() * 0.12);
      painter->drawPath(path);
    }
    painter->restore();

    // Tool name text
    const QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen(enabled ? (selected ? QColor("#edf0f9") : QColor("#c5cde0"))
                            : QColor("#4a5268"));
    QFont f = painter->font();
    f.setPointSizeF(8.5);
    f.setBold(selected);
    painter->setFont(f);
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);

    painter->restore();
  }

  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(86, 32);
  }
};

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

QString subToolNameJa(QString id, const QString& displayName) {
  id = id.toLower();
  if (id == "brush_normal") return QString::fromUtf8(u8"通常ブラシ");
  if (id == "brush_hard") return QString::fromUtf8(u8"硬めブラシ");
  if (id == "brush_soft") return QString::fromUtf8(u8"やわらかブラシ");
  if (id == "brush_airbrush") return QString::fromUtf8(u8"エアブラシ");
  if (id == "eraser_normal") return QString::fromUtf8(u8"通常消しゴム");
  if (id == "eraser_soft") return QString::fromUtf8(u8"やわらか消しゴム");
  if (id == "eraser_vector_touch") return QString::fromUtf8(u8"ベクター消しゴム（触れた部分）");
  if (id == "eraser_vector_intersection") return QString::fromUtf8(u8"ベクター消しゴム（交点まで）");
  if (id == "eraser_vector_trim") return QString::fromUtf8(u8"ベクター消しゴム（はみ出し）");
  if (id == "line_raster") return QString::fromUtf8(u8"ラスタ直線");
  if (id == "line_vector") return QString::fromUtf8(u8"ベクター直線");
  if (id == "line_vector_snap") return QString::fromUtf8(u8"ベクター直線（角度スナップ）");
  if (id == "line_vector_thick") return QString::fromUtf8(u8"ベクター直線（太）");
  if (id == "line_vector_thin") return QString::fromUtf8(u8"ベクター直線（細）");
  if (id == "fill_contiguous") return QString::fromUtf8(u8"塗りつぶし（連結）");
  if (id == "fill_gapclose") return QString::fromUtf8(u8"塗りつぶし（隙間閉じ）");
  if (id == "rect_default") return QString::fromUtf8(u8"矩形選択");
  if (id == "lasso_default") return QString::fromUtf8(u8"なげなわ選択");
  if (id == "poly_lasso") return QString::fromUtf8(u8"多角形選択");
  if (id == "auto_select") return QString::fromUtf8(u8"自動選択");
  if (id == "object_select") return QString::fromUtf8(u8"オブジェクト選択");
  if (id == "move_layer_default") return QString::fromUtf8(u8"レイヤー移動");
  if (id == "hand_default") return QString::fromUtf8(u8"手のひら移動");
  if (id == "zoom_default") return QString::fromUtf8(u8"ズーム");
  if (id == "eyedropper_default") return QString::fromUtf8(u8"色取得");
  return displayName;
}

} // namespace

SubToolPanel::SubToolPanel(QWidget* parent)
    : QWidget(parent),
      m_toolNameLabel(new QLabel(QString::fromUtf8(u8"ツール: -"), this)),
      m_summaryLabel(new QLabel(QString::fromUtf8(u8"サブツール: -"), this)),
      m_searchEdit(new QLineEdit(this)),
      m_createButton(new QToolButton(this)),
      m_settingsButton(new QToolButton(this)),
      m_settingsMenu(new QMenu(this)),
      m_subToolList(new QListWidget(this)) {
  setStyleSheet(
      "QWidget { background: #2e2e2e; color: #d4d4d4; }"
      "QLineEdit { min-height: 20px; background: #202020; border: 1px solid #333; color: #d4d4d4; border-radius: 1px; padding: 1px 4px; }"
      "QToolButton { min-height: 20px; min-width: 20px; padding: 1px; background: #3a3a3a; border: 1px solid #1e1e1e; border-radius: 2px; }"
      "QToolButton:hover { background: #484848; }");
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(1, 1, 1, 1);
  layout->setSpacing(1);

  // Tool/subtool name labels: compact, single-line
  m_toolNameLabel->setStyleSheet(
      "font-weight: 700; font-size: 11px; color: #e0e0e0;"
      "padding: 1px 4px; border-bottom: 1px solid #1a1a1a;");
  m_toolNameLabel->setFixedHeight(18);
  m_summaryLabel->setStyleSheet("color: #909090; font-size: 10px; padding: 0 4px;");
  m_summaryLabel->setWordWrap(false);
  m_summaryLabel->setFixedHeight(16);
  m_searchEdit->setPlaceholderText(QString::fromUtf8(u8"サブツールを検索..."));
  m_searchEdit->setFixedHeight(20);
  m_createButton->setAutoRaise(true);
  m_createButton->setIcon(app::ui::icon("layer_add", 16));
  m_createButton->setIconSize(QSize(14, 14));
  m_createButton->setFixedSize(20, 20);
  m_createButton->setToolTip(QString::fromUtf8(u8"新しいサブツールを作成"));
  m_settingsButton->setAutoRaise(true);
  m_settingsButton->setIcon(app::ui::icon("settings", 16));
  m_settingsButton->setIconSize(QSize(14, 14));
  m_settingsButton->setFixedSize(20, 20);
  m_settingsButton->setToolTip(QString::fromUtf8(u8"サブツール設定"));
  m_settingsButton->setPopupMode(QToolButton::InstantPopup);
  m_settingsButton->setMenu(m_settingsMenu);

  auto* duplicateAction = m_settingsMenu->addAction(app::ui::icon("duplicate", 16), QString::fromUtf8(u8"複製"));
  auto* saveAction = m_settingsMenu->addAction(app::ui::icon("save", 16), QString::fromUtf8(u8"保存"));
  m_settingsMenu->addSeparator();
  auto* renameAction = m_settingsMenu->addAction(QString::fromUtf8(u8"名前変更"));
  auto* deleteAction = m_settingsMenu->addAction(app::ui::icon("delete", 16), QString::fromUtf8(u8"削除"));
  auto* resetAction = m_settingsMenu->addAction(QString::fromUtf8(u8"初期化"));

  // ListMode: single column, compact rows, spacing=0
  m_subToolList->setViewMode(QListWidget::ListMode);
  m_subToolList->setResizeMode(QListWidget::Fixed);
  m_subToolList->setMovement(QListWidget::Static);
  m_subToolList->setSelectionMode(QAbstractItemView::SingleSelection);
  m_subToolList->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_subToolList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_subToolList->setUniformItemSizes(true);
  m_subToolList->setItemDelegate(new SubToolDelegate(m_subToolList));
  m_subToolList->setSpacing(0);
  m_subToolList->setMouseTracking(true);
  m_subToolList->setWordWrap(false);
  m_subToolList->setStyleSheet(
      "QListWidget { background: #13151c; border: none; outline: none; padding: 0px; }"
      "QListWidget::item { border: none; padding: 0px; margin: 0px; background: transparent; }"
      "QListWidget::item:hover { background: transparent; }"
      "QListWidget::item:selected { background: transparent; }");

  // Search row: search field + action buttons, max 2px margins
  m_searchRowLayout = new QHBoxLayout();
  m_searchRowLayout->setContentsMargins(0, 0, 0, 0);
  m_searchRowLayout->setSpacing(2);
  m_searchRowLayout->addWidget(m_searchEdit, 1);

  // Compact header row: tool name + action buttons
  auto* headerRow = new QHBoxLayout();
  headerRow->setContentsMargins(0, 0, 0, 0);
  headerRow->setSpacing(2);
  headerRow->addWidget(m_toolNameLabel, 1);
  headerRow->addWidget(m_createButton);
  headerRow->addWidget(m_settingsButton);

  layout->addLayout(headerRow);
  layout->addWidget(m_summaryLabel);
  layout->addLayout(m_searchRowLayout);
  layout->addWidget(m_subToolList, 1);

  m_compactActionsLayout = new QHBoxLayout();
  m_compactActionsLayout->setContentsMargins(0, 0, 0, 0);
  m_compactActionsLayout->setSpacing(1);
  // buttons are now in headerRow, not here

  connect(m_subToolList, &QListWidget::currentRowChanged, this, &SubToolPanel::onCurrentSubToolChanged);
  connect(m_searchEdit, &QLineEdit::textChanged, this, &SubToolPanel::onFilterTextChanged);
  connect(m_createButton, &QToolButton::clicked, this, &SubToolPanel::onCreateClicked);
  connect(duplicateAction, &QAction::triggered, this, &SubToolPanel::onDuplicateClicked);
  connect(saveAction, &QAction::triggered, this, &SubToolPanel::onSaveClicked);
  connect(renameAction, &QAction::triggered, this, &SubToolPanel::onRenameClicked);
  connect(deleteAction, &QAction::triggered, this, &SubToolPanel::onDeleteClicked);
  connect(resetAction, &QAction::triggered, this, &SubToolPanel::onResetClicked);
}

void SubToolPanel::setController(app::bridge::AppController* controller) {
  if (m_controller != nullptr) {
    disconnect(m_controller, nullptr, this, nullptr);
  }

  m_controller = controller;
  if (m_controller == nullptr) {
    return;
  }

  connect(m_controller, &app::bridge::AppController::toolStateChanged, this, &SubToolPanel::refreshFromController);
  refreshFromController();
}

void SubToolPanel::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  applyResponsiveLayout();
}

void SubToolPanel::applyResponsiveLayout() {
  if (m_searchRowLayout == nullptr || m_compactActionsLayout == nullptr) {
    return;
  }
  const bool compact = width() < 220;
  m_searchRowLayout->setDirection(compact ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
  m_compactActionsLayout->setDirection(QBoxLayout::LeftToRight);
}

void SubToolPanel::refreshFromController() {
  if (m_controller == nullptr) {
    return;
  }

  const QSignalBlocker blocker(m_subToolList);
  m_refreshing = true;
  m_toolNameLabel->setText(QString::fromUtf8(u8"ツール: %1").arg(toolNameJa(m_controller->currentTool())));
  const QString currentSubToolName = subToolNameJa(
      QString::fromStdString(m_controller->currentSubToolId()),
      QString::fromStdString(m_controller->currentSubToolDisplayName()));
  m_summaryLabel->setText(QString::fromUtf8(u8"現在: %1").arg(currentSubToolName));
  m_subToolList->clear();
  const auto items = m_controller->subToolViewModels();
  const QString query = m_searchEdit->text().trimmed();
  bool hasEnabledRow = false;
  for (const auto& item : items) {
    const QString displayName = QString::fromStdString(item.name);
    const QString subToolId = QString::fromStdString(item.id);
    const QString localizedName = subToolNameJa(subToolId, displayName);
    const bool matches = query.isEmpty()
        || localizedName.contains(query, Qt::CaseInsensitive)
        || displayName.contains(query, Qt::CaseInsensitive)
        || subToolId.contains(query, Qt::CaseInsensitive);
    if (!matches) {
      continue;
    }
    auto* row = new QListWidgetItem(m_subToolList);
    const QString displayText =
        item.enabled ? localizedName : QString::fromUtf8(u8"%1（非対応）").arg(localizedName);
    row->setText(displayText);
    row->setData(kSubToolIdRole, subToolId);
    row->setData(kSubToolEnabledRole, item.enabled);
    row->setFlags(item.enabled ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable) : Qt::NoItemFlags);
    row->setToolTip(
        item.enabled
            ? (item.hint.empty() ? localizedName : QString::fromStdString(item.hint))
            : QString::fromUtf8(u8"現在のレイヤー種別では使用できません"));
    row->setSizeHint(QSize(86, 32));
    hasEnabledRow = hasEnabledRow || item.enabled;
    if (item.active) {
      m_subToolList->setCurrentItem(row);
    }
  }
  if (m_subToolList->count() > 0 && m_subToolList->currentRow() < 0) {
    for (int i = 0; i < m_subToolList->count(); ++i) {
      QListWidgetItem* row = m_subToolList->item(i);
      if (row != nullptr && row->data(kSubToolEnabledRole).toBool()) {
        m_subToolList->setCurrentRow(i);
        break;
      }
    }
  }
  const bool hasRows = m_subToolList->count() > 0;
  m_createButton->setEnabled(true);
  m_settingsButton->setEnabled(hasRows && hasEnabledRow);
  if (!hasEnabledRow && hasRows) {
    m_summaryLabel->setText(QString::fromUtf8(u8"現在のレイヤー種別で有効なサブツールがありません"));
  }
  m_refreshing = false;
}

void SubToolPanel::onCurrentSubToolChanged(int row) {
  if (m_controller == nullptr || m_refreshing || row < 0) {
    return;
  }
  auto* item = m_subToolList->item(row);
  if (item == nullptr) {
    return;
  }
  if (!item->data(kSubToolEnabledRole).toBool()) {
    return;
  }
  const QString id = item->data(kSubToolIdRole).toString();
  if (id.isEmpty()) {
    return;
  }
  m_controller->setCurrentSubTool(id.toStdString());
}

void SubToolPanel::onFilterTextChanged(const QString& text) {
  Q_UNUSED(text);
  refreshFromController();
}

void SubToolPanel::onCreateClicked() {
  if (m_controller == nullptr) {
    return;
  }
  if (!m_controller->createCurrentSubTool()) {
    return;
  }
  refreshFromController();
}

void SubToolPanel::onDuplicateClicked() {
  if (m_controller == nullptr) {
    return;
  }
  if (!m_controller->duplicateCurrentSubTool()) {
    return;
  }
  refreshFromController();
}

void SubToolPanel::onSaveClicked() {
  if (m_controller == nullptr) {
    return;
  }
  m_controller->saveSubToolSettings();
}

void SubToolPanel::onRenameClicked() {
  if (m_controller == nullptr) {
    return;
  }
  bool ok = false;
  const QString current = subToolNameJa(
      QString::fromStdString(m_controller->currentSubToolId()),
      QString::fromStdString(m_controller->currentSubToolDisplayName()));
  const QString renamed = QInputDialog::getText(
      this,
      QString::fromUtf8(u8"サブツール名前変更"),
      QString::fromUtf8(u8"新しい名前"),
      QLineEdit::Normal,
      current,
      &ok);
  if (!ok) {
    return;
  }
  if (m_controller->renameCurrentSubTool(renamed.toStdString())) {
    refreshFromController();
  }
}

void SubToolPanel::onDeleteClicked() {
  if (m_controller == nullptr) {
    return;
  }
  const QString name = QString::fromStdString(m_controller->currentSubToolDisplayName());
  const auto answer = QMessageBox::question(
      this,
      QString::fromUtf8(u8"サブツール削除"),
      QString::fromUtf8(u8"「%1」を削除しますか？").arg(name));
  if (answer != QMessageBox::Yes) {
    return;
  }
  if (m_controller->deleteCurrentSubTool()) {
    refreshFromController();
  }
}

void SubToolPanel::onResetClicked() {
  if (m_controller == nullptr) {
    return;
  }
  if (m_controller->resetCurrentSubTool()) {
    refreshFromController();
  }
}

} // namespace app::panels




