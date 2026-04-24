#include "app/panels/SubToolPanel.h"

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

QPixmap makeStrokePreview(
    const QString& subToolId,
    const QColor& color,
    const QSize& size,
    bool enabled) {
  QPixmap pixmap(size);
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);

  QColor penColor = color;
  if (!enabled) {
    penColor = QColor(120, 126, 137);
  }
  const QString id = subToolId.toLower();
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
    const bool enabled = index.data(kSubToolEnabledRole).toBool();
    const QString id = index.data(kSubToolIdRole).toString().toLower();

    QColor bg = selected ? QColor("#345985") : QColor("#1b2129");
    if (!enabled) {
      bg = QColor("#171b22");
    }
    painter->fillRect(option.rect, bg);

    QColor stroke = selected ? QColor(245, 250, 255, 140) : QColor(220, 230, 245, 88);
    qreal width = 2.4;
    if (id.contains("hard")) {
      width = 3.4;
      stroke.setAlpha(selected ? 170 : 120);
    } else if (id.contains("soft")) {
      width = 5.0;
      stroke.setAlpha(selected ? 90 : 55);
    } else if (id.contains("airbrush")) {
      width = 6.0;
      stroke.setAlpha(selected ? 75 : 45);
    } else if (id.contains("fill")) {
      width = 7.0;
      stroke.setAlpha(selected ? 95 : 60);
    } else if (id.contains("vector")) {
      width = 1.8;
      stroke.setAlpha(selected ? 180 : 120);
    } else if (id.contains("eraser")) {
      stroke = QColor(225, 232, 245, selected ? 120 : 80);
      width = 3.0;
    }

    QPen pen(stroke, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter->setPen(pen);

    const int fixedPreviewWidth = 132;
    QRectF r(
        option.rect.left() + 8,
        option.rect.top() + 5,
        fixedPreviewWidth,
        option.rect.height() - 10);
    QPainterPath path;
    path.moveTo(r.left(), r.center().y() + 3);
    path.cubicTo(r.left() + r.width() * 0.25, r.top(),
                 r.left() + r.width() * 0.60, r.bottom(),
                 r.right(), r.center().y() - 2);
    painter->drawPath(path);

    const QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen(enabled ? QColor("#edf4ff") : QColor("#7f8998"));
    painter->drawText(option.rect.adjusted(10, 0, -6, 0), Qt::AlignVCenter | Qt::AlignLeft, text);

    painter->restore();
  }

  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(120, 28);
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
  if (id == "auto_select") return QString::fromUtf8(u8"自動選択");
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
      "QLineEdit { min-height: 22px; }"
      "QToolButton { min-height: 20px; min-width: 20px; padding: 1px; }");
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(3);

  m_toolNameLabel->setStyleSheet("font-weight: 700;");
  m_summaryLabel->setStyleSheet("color: #9fb4cf;");
  m_summaryLabel->setWordWrap(true);
  m_searchEdit->setPlaceholderText(QString::fromUtf8(u8"サブツールを検索..."));
  m_createButton->setAutoRaise(true);
  m_createButton->setIcon(app::ui::icon("layer_add", 16));
  m_createButton->setIconSize(QSize(14, 14));
  m_createButton->setToolTip(QString::fromUtf8(u8"新しいサブツールを作成"));
  m_settingsButton->setAutoRaise(true);
  m_settingsButton->setIcon(app::ui::icon("settings", 16));
  m_settingsButton->setIconSize(QSize(14, 14));
  m_settingsButton->setToolTip(QString::fromUtf8(u8"サブツール設定"));
  m_settingsButton->setPopupMode(QToolButton::InstantPopup);
  m_settingsButton->setMenu(m_settingsMenu);

  auto* duplicateAction = m_settingsMenu->addAction(app::ui::icon("duplicate", 16), QString::fromUtf8(u8"複製"));
  auto* saveAction = m_settingsMenu->addAction(app::ui::icon("save", 16), QString::fromUtf8(u8"保存"));
  m_settingsMenu->addSeparator();
  auto* renameAction = m_settingsMenu->addAction(QString::fromUtf8(u8"名前変更"));
  auto* deleteAction = m_settingsMenu->addAction(app::ui::icon("delete", 16), QString::fromUtf8(u8"削除"));
  auto* resetAction = m_settingsMenu->addAction(QString::fromUtf8(u8"初期化"));

  m_subToolList->setSelectionMode(QAbstractItemView::SingleSelection);
  m_subToolList->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_subToolList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_subToolList->setUniformItemSizes(true);
  m_subToolList->setItemDelegate(new SubToolDelegate(m_subToolList));
  m_subToolList->setSpacing(1);
  m_subToolList->setStyleSheet(
      "QListWidget::item { padding: 1px 2px; border-bottom: 1px solid #303a46; }"
      "QListWidget::item:hover { background: #26303d; }"
      "QListWidget::item:selected { background: #345985; color: #ffffff; }");

  m_searchRowLayout = new QHBoxLayout();
  m_searchRowLayout->setContentsMargins(0, 0, 0, 0);
  m_searchRowLayout->setSpacing(3);
  m_searchRowLayout->addWidget(m_searchEdit, 1);
  auto* searchLabel = new QLabel(QString::fromUtf8(u8"一覧"), this);
  searchLabel->setStyleSheet("font-size:10px; color:#9fb1c8;");
  m_searchRowLayout->addWidget(searchLabel);

  layout->addWidget(m_toolNameLabel);
  layout->addWidget(m_summaryLabel);
  auto* listTitle = new QLabel(QString::fromUtf8(u8"プリセット一覧"), this);
  listTitle->setStyleSheet("font-weight:600; color:#c9d4e4;");
  layout->addWidget(listTitle);
  layout->addLayout(m_searchRowLayout);
  layout->addWidget(m_subToolList);

  m_compactActionsLayout = new QHBoxLayout();
  m_compactActionsLayout->setContentsMargins(0, 0, 0, 0);
  m_compactActionsLayout->setSpacing(1);
  m_compactActionsLayout->addStretch(1);
  m_compactActionsLayout->addWidget(m_createButton);
  m_compactActionsLayout->addWidget(m_settingsButton);
  layout->addLayout(m_compactActionsLayout);

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
    row->setSizeHint(QSize(row->sizeHint().width(), 28));
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

