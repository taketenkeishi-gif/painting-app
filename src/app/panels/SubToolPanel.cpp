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

QString subToolNameJa(QString id, QString displayName) {
  id = id.toLower();
  if (id == "brush_normal") return "通常ブラシ";
  if (id == "brush_hard") return "硬めブラシ";
  if (id == "brush_soft") return "やわらかブラシ";
  if (id == "brush_airbrush") return "エアブラシ";
  if (id == "eraser_normal") return "通常消しゴム";
  if (id == "eraser_soft") return "やわらか消しゴム";
  if (id == "eraser_vector_whole") return "ベクター消去（接触線）";
  if (id == "line_raster") return "ラスタ直線";
  if (id == "line_vector") return "ベクター直線";
  if (id == "line_vector_snap") return "ベクター直線（角度スナップ）";
  if (id == "line_vector_thick") return "ベクター直線（太）";
  if (id == "line_vector_thin") return "ベクター直線（細）";
  if (id == "fill_contiguous") return "塗りつぶし（連結）";
  if (id == "fill_gapclose") return "塗りつぶし（隙間閉じ）";
  if (id == "rect_default") return "矩形選択";
  if (id == "lasso_default") return "なげなわ選択";
  if (id == "auto_select") return "自動選択";
  if (id == "move_layer_default") return "レイヤー移動";
  if (id == "hand_default") return "手のひら移動";
  if (id == "zoom_default") return "ズーム";
  if (id == "eyedropper_default") return "合成色を取得";
  return displayName;
}
} // namespace

SubToolPanel::SubToolPanel(QWidget* parent)
    : QWidget(parent),
      m_toolNameLabel(new QLabel("ツール: -", this)),
      m_summaryLabel(new QLabel("サブツール: -", this)),
      m_searchEdit(new QLineEdit(this)),
      m_duplicateButton(new QPushButton("複製", this)),
      m_renameButton(new QToolButton(this)),
      m_deleteButton(new QToolButton(this)),
      m_resetButton(new QToolButton(this)),
      m_subToolList(new QListWidget(this)) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(6, 6, 6, 6);
  layout->setSpacing(6);

  m_toolNameLabel->setStyleSheet("font-weight: 700;");
  m_summaryLabel->setStyleSheet("color: #9fb4cf;");
  m_searchEdit->setPlaceholderText("サブツールを検索...");
  m_duplicateButton->setToolTip("現在のプリセットを複製します。");
  m_searchEdit->setMinimumHeight(28);
  m_duplicateButton->setMinimumHeight(28);
  m_renameButton->setText("名前変更");
  m_deleteButton->setText("削除");
  m_resetButton->setText("初期化");
  m_renameButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
  m_deleteButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
  m_resetButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
  m_renameButton->setMinimumHeight(28);
  m_deleteButton->setMinimumHeight(28);
  m_resetButton->setMinimumHeight(28);

  m_subToolList->setSelectionMode(QAbstractItemView::SingleSelection);
  m_subToolList->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_subToolList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_subToolList->setUniformItemSizes(true);
  m_subToolList->setSpacing(2);
  m_subToolList->setStyleSheet(
      "QListWidget::item { padding: 6px 8px; border-bottom: 1px solid #303a46; }"
      "QListWidget::item:hover { background: #26303d; }"
      "QListWidget::item:selected { background: #345985; color: #ffffff; }");

  auto* searchRow = new QHBoxLayout();
  searchRow->setContentsMargins(0, 0, 0, 0);
  searchRow->setSpacing(6);
  searchRow->addWidget(m_searchEdit, 1);
  searchRow->addWidget(m_duplicateButton);

  layout->addWidget(m_toolNameLabel);
  layout->addWidget(m_summaryLabel);
  layout->addLayout(searchRow);
  layout->addWidget(m_subToolList);

  auto* manageRow = new QHBoxLayout();
  manageRow->setContentsMargins(0, 0, 0, 0);
  manageRow->setSpacing(6);
  manageRow->addWidget(m_renameButton);
  manageRow->addWidget(m_deleteButton);
  manageRow->addWidget(m_resetButton);
  layout->addLayout(manageRow);

  connect(m_subToolList, &QListWidget::currentRowChanged, this, &SubToolPanel::onCurrentSubToolChanged);
  connect(m_searchEdit, &QLineEdit::textChanged, this, &SubToolPanel::onFilterTextChanged);
  connect(m_duplicateButton, &QPushButton::clicked, this, &SubToolPanel::onDuplicateClicked);
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

void SubToolPanel::refreshFromController() {
  if (m_controller == nullptr) {
    return;
  }

  const QSignalBlocker blocker(m_subToolList);
  m_refreshing = true;
  m_toolNameLabel->setText(QString("ツール: %1").arg(toolNameJa(m_controller->currentTool())));
  m_summaryLabel->setText(
      QString("サブツール: %1").arg(subToolNameJa(QString::fromStdString(m_controller->currentSubToolId()),
                                                QString::fromStdString(m_controller->currentSubToolDisplayName()))));
  m_subToolList->clear();
  const auto items = m_controller->subToolViewModels();
  const QString query = m_searchEdit->text().trimmed();
  bool hasEnabledRow = false;
  for (const auto& item : items) {
    const QString displayName = QString::fromStdString(item.name);
    if (!query.isEmpty() && !displayName.contains(query, Qt::CaseInsensitive)) {
      continue;
    }
    const QString subToolId = QString::fromStdString(item.id);
    auto* row = new QListWidgetItem(subToolNameJa(subToolId, QString::fromStdString(item.name)), m_subToolList);
    row->setData(kSubToolIdRole, subToolId);
    row->setData(kSubToolEnabledRole, item.enabled);
    row->setFlags(item.enabled ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable) : Qt::NoItemFlags);
    row->setForeground(item.enabled ? palette().windowText().color() : QColor(130, 136, 148));
    row->setToolTip(item.hint.empty() ? displayName : QString::fromStdString(item.hint));
    row->setSizeHint(QSize(row->sizeHint().width(), 30));
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
  m_duplicateButton->setEnabled(hasRows);
  m_renameButton->setEnabled(hasRows && hasEnabledRow);
  m_deleteButton->setEnabled(hasRows && hasEnabledRow);
  m_resetButton->setEnabled(hasRows && hasEnabledRow);
  if (!hasEnabledRow && hasRows) {
    m_summaryLabel->setText("サブツール: （現在のレイヤー種別で利用可能なプリセットがありません）");
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

void SubToolPanel::onDuplicateClicked() {
  if (m_controller == nullptr) {
    return;
  }
  if (!m_controller->duplicateCurrentSubTool()) {
    return;
  }
  refreshFromController();
}

void SubToolPanel::onRenameClicked() {
  if (m_controller == nullptr) {
    return;
  }
  bool ok = false;
  const QString current = subToolNameJa(
      QString::fromStdString(m_controller->currentSubToolId()),
      QString::fromStdString(m_controller->currentSubToolDisplayName()));
  const QString renamed = QInputDialog::getText(this, "サブツール名の変更", "新しい名前", QLineEdit::Normal, current, &ok);
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
  const auto answer = QMessageBox::question(this, "サブツール削除", QString("「%1」を削除しますか？").arg(name));
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
