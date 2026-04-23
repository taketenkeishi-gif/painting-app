#include "app/panels/SubToolPanel.h"

#include <QAbstractItemView>
#include <QColor>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>

#include "app/bridge/AppController.h"

namespace app::panels {

namespace {
constexpr int kSubToolIdRole = Qt::UserRole;
constexpr int kSubToolEnabledRole = Qt::UserRole + 1;

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
      m_createButton(new QPushButton(QString::fromUtf8(u8"新規"), this)),
      m_duplicateButton(new QPushButton(QString::fromUtf8(u8"複製"), this)),
      m_saveButton(new QPushButton(QString::fromUtf8(u8"保存"), this)),
      m_renameButton(new QToolButton(this)),
      m_deleteButton(new QToolButton(this)),
      m_resetButton(new QToolButton(this)),
      m_subToolList(new QListWidget(this)) {
  setStyleSheet(
      "QLineEdit { min-height: 22px; }"
      "QPushButton, QToolButton { min-height: 22px; padding: 2px 6px; }");
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(4);

  m_toolNameLabel->setStyleSheet("font-weight: 700;");
  m_summaryLabel->setStyleSheet("color: #9fb4cf;");
  m_summaryLabel->setWordWrap(true);
  m_searchEdit->setPlaceholderText(QString::fromUtf8(u8"サブツールを検索..."));
  m_createButton->setToolTip(QString::fromUtf8(u8"新しいサブツールを作成"));
  m_duplicateButton->setToolTip(QString::fromUtf8(u8"現在のサブツールを複製"));
  m_saveButton->setToolTip(QString::fromUtf8(u8"現在のサブツール設定を保存"));
  m_renameButton->setText(QString::fromUtf8(u8"名前変更"));
  m_deleteButton->setText(QString::fromUtf8(u8"削除"));
  m_resetButton->setText(QString::fromUtf8(u8"初期化"));
  m_renameButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
  m_deleteButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
  m_resetButton->setToolButtonStyle(Qt::ToolButtonTextOnly);

  m_subToolList->setSelectionMode(QAbstractItemView::SingleSelection);
  m_subToolList->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_subToolList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_subToolList->setUniformItemSizes(true);
  m_subToolList->setSpacing(1);
  m_subToolList->setStyleSheet(
      "QListWidget::item { padding: 4px 6px; border-bottom: 1px solid #303a46; }"
      "QListWidget::item:hover { background: #26303d; }"
      "QListWidget::item:selected { background: #345985; color: #ffffff; }");

  m_searchRowLayout = new QHBoxLayout();
  m_searchRowLayout->setContentsMargins(0, 0, 0, 0);
  m_searchRowLayout->setSpacing(4);
  m_searchRowLayout->addWidget(m_createButton);
  m_searchRowLayout->addWidget(m_searchEdit, 1);
  m_searchRowLayout->addWidget(m_duplicateButton);

  layout->addWidget(m_toolNameLabel);
  layout->addWidget(m_summaryLabel);
  auto* listTitle = new QLabel(QString::fromUtf8(u8"プリセット一覧"), this);
  listTitle->setStyleSheet("font-weight:600; color:#c9d4e4;");
  layout->addWidget(listTitle);
  layout->addLayout(m_searchRowLayout);
  layout->addWidget(m_subToolList);

  auto* manageTitle = new QLabel(QString::fromUtf8(u8"管理"), this);
  manageTitle->setStyleSheet("font-weight:600; color:#c9d4e4;");
  layout->addWidget(manageTitle);
  m_manageRowLayout = new QHBoxLayout();
  m_manageRowLayout->setContentsMargins(0, 0, 0, 0);
  m_manageRowLayout->setSpacing(4);
  m_manageRowLayout->addWidget(m_saveButton);
  m_manageRowLayout->addWidget(m_renameButton);
  m_manageRowLayout->addWidget(m_deleteButton);
  m_manageRowLayout->addWidget(m_resetButton);
  layout->addLayout(m_manageRowLayout);

  connect(m_subToolList, &QListWidget::currentRowChanged, this, &SubToolPanel::onCurrentSubToolChanged);
  connect(m_searchEdit, &QLineEdit::textChanged, this, &SubToolPanel::onFilterTextChanged);
  connect(m_createButton, &QPushButton::clicked, this, &SubToolPanel::onCreateClicked);
  connect(m_duplicateButton, &QPushButton::clicked, this, &SubToolPanel::onDuplicateClicked);
  connect(m_saveButton, &QPushButton::clicked, this, &SubToolPanel::onSaveClicked);
  connect(m_renameButton, &QToolButton::clicked, this, &SubToolPanel::onRenameClicked);
  connect(m_deleteButton, &QToolButton::clicked, this, &SubToolPanel::onDeleteClicked);
  connect(m_resetButton, &QToolButton::clicked, this, &SubToolPanel::onResetClicked);
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
  if (m_searchRowLayout == nullptr || m_manageRowLayout == nullptr) {
    return;
  }
  const bool compact = width() < 260;
  m_searchRowLayout->setDirection(compact ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
  m_manageRowLayout->setDirection(compact ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
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
    auto* row = new QListWidgetItem(
        item.enabled ? localizedName : QString::fromUtf8(u8"%1（非対応）").arg(localizedName),
        m_subToolList);
    row->setData(kSubToolIdRole, subToolId);
    row->setData(kSubToolEnabledRole, item.enabled);
    row->setFlags(item.enabled ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable) : Qt::NoItemFlags);
    row->setForeground(item.enabled ? palette().windowText().color() : QColor(130, 136, 148));
    row->setToolTip(
        item.enabled
            ? (item.hint.empty() ? localizedName : QString::fromStdString(item.hint))
            : QString::fromUtf8(u8"現在のレイヤー種別では使用できません"));
    row->setSizeHint(QSize(row->sizeHint().width(), 24));
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
  m_duplicateButton->setEnabled(hasRows);
  m_saveButton->setEnabled(hasRows);
  m_renameButton->setEnabled(hasRows && hasEnabledRow);
  m_deleteButton->setEnabled(hasRows && hasEnabledRow);
  m_resetButton->setEnabled(hasRows && hasEnabledRow);
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
