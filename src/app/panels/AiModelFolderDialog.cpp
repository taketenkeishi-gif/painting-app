#include "app/panels/AiModelFolderDialog.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

namespace app::panels {

// ─────────────────────────────────────────────────────────────────────────────
// 静的ユーティリティ
// ─────────────────────────────────────────────────────────────────────────────
QStringList AiModelFolderDialog::loadFolders() {
    QSettings s("taketenkeishi", "LayeredPaintApp");
    QStringList list = s.value(kSettingsKey).toStringList();

    // デフォルト: <exe>/models/ を常に含む
    const QString defaultDir =
        QCoreApplication::applicationDirPath() + "/models";
    if (!list.contains(defaultDir)) {
        list.prepend(defaultDir);
    }
    return list;
}

void AiModelFolderDialog::saveFolders(const QStringList& folders) {
    QSettings s("taketenkeishi", "LayeredPaintApp");
    // デフォルトフォルダは保存リストから除く（常に自動付加するため）
    const QString defaultDir =
        QCoreApplication::applicationDirPath() + "/models";
    QStringList toSave;
    for (const QString& f : folders) {
        if (f != defaultDir) toSave << f;
    }
    s.setValue(kSettingsKey, toSave);
}

// ─────────────────────────────────────────────────────────────────────────────
// コンストラクタ
// ─────────────────────────────────────────────────────────────────────────────
AiModelFolderDialog::AiModelFolderDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(QString::fromUtf8(u8"AI モデルフォルダ設定"));
    setMinimumWidth(520);
    setupUi();
}

// ─────────────────────────────────────────────────────────────────────────────
// UI 構築
// ─────────────────────────────────────────────────────────────────────────────
void AiModelFolderDialog::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setSpacing(10);
    root->setContentsMargins(14, 14, 14, 14);

    // ── 説明 ──────────────────────────────────────────────────────────────────
    auto* info = new QLabel(
        QString::fromUtf8(
            u8"登録したフォルダ内の <b>.onnx</b> ファイルをAIモデルとして使用できます。<br>"
            u8"ComfyUI の <code>models/upscale_models</code> などを追加すると便利です。"),
        this);
    info->setWordWrap(true);
    info->setStyleSheet(
        "color: #c8cde0; background: #1a1d27; border: 1px solid #2e3348;"
        "border-radius: 4px; padding: 8px 10px; font-size: 11px;");
    root->addWidget(info);

    // ── フォルダリスト ─────────────────────────────────────────────────────────
    auto* grp  = new QGroupBox(QString::fromUtf8(u8"モデル検索フォルダ"), this);
    auto* vbox = new QVBoxLayout(grp);

    m_list = new QListWidget(grp);
    m_list->setAlternatingRowColors(true);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setStyleSheet(
        "QListWidget { background: #0d0f14; border: 1px solid #2a2e3e;"
        "border-radius: 3px; color: #c8cde0; font-size: 11px; }"
        "QListWidget::item:selected { background: #1d4a8a; }"
        "QListWidget::item:alternate { background: #131620; }");
    m_list->setMinimumHeight(160);

    // 現在の設定を読み込んで表示
    for (const QString& folder : loadFolders()) {
        auto* item = new QListWidgetItem(folder, m_list);
        const QString defaultDir =
            QCoreApplication::applicationDirPath() + "/models";
        if (folder == defaultDir) {
            item->setForeground(QColor("#6a7484"));
            item->setToolTip(QString::fromUtf8(u8"デフォルトフォルダ（削除不可）"));
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        }
    }

    // ── ボタン行 ─────────────────────────────────────────────────────────────
    auto* btnRow = new QHBoxLayout();
    m_addBtn = new QPushButton(
        QString::fromUtf8(u8"フォルダを追加..."), grp);
    m_addBtn->setStyleSheet(
        "QPushButton { background: #1e2130; border: 1px solid #3a3f58;"
        "border-radius: 4px; color: #c8cde0; padding: 4px 14px; }"
        "QPushButton:hover { background: #252b40; }");
    m_removeBtn = new QPushButton(
        QString::fromUtf8(u8"選択を削除"), grp);
    m_removeBtn->setStyleSheet(m_addBtn->styleSheet());
    m_removeBtn->setEnabled(false);
    btnRow->addWidget(m_addBtn);
    btnRow->addWidget(m_removeBtn);
    btnRow->addStretch();

    vbox->addWidget(m_list);
    vbox->addLayout(btnRow);
    root->addWidget(grp);

    // ── ダイアログボタン ───────────────────────────────────────────────────────
    auto* dlgBtns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    dlgBtns->button(QDialogButtonBox::Ok)->setText(
        QString::fromUtf8(u8"OK"));
    root->addWidget(dlgBtns);

    // ── 接続 ─────────────────────────────────────────────────────────────────
    connect(m_addBtn,    &QPushButton::clicked, this, &AiModelFolderDialog::onAddFolder);
    connect(m_removeBtn, &QPushButton::clicked, this, &AiModelFolderDialog::onRemoveFolder);
    connect(m_list, &QListWidget::itemSelectionChanged, this, [this]() {
        const bool hasSelection = !m_list->selectedItems().isEmpty();
        m_removeBtn->setEnabled(hasSelection);
    });
    connect(dlgBtns, &QDialogButtonBox::accepted, this, &AiModelFolderDialog::onAccepted);
    connect(dlgBtns, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

// ─────────────────────────────────────────────────────────────────────────────
// スロット
// ─────────────────────────────────────────────────────────────────────────────
void AiModelFolderDialog::onAddFolder() {
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        QString::fromUtf8(u8"モデルフォルダを選択"),
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) return;

    // 重複チェック
    for (int i = 0; i < m_list->count(); ++i) {
        if (m_list->item(i)->text() == dir) return;
    }
    m_list->addItem(dir);
}

void AiModelFolderDialog::onRemoveFolder() {
    const auto selected = m_list->selectedItems();
    for (QListWidgetItem* item : selected) {
        delete m_list->takeItem(m_list->row(item));
    }
}

void AiModelFolderDialog::onAccepted() {
    QStringList folders;
    for (int i = 0; i < m_list->count(); ++i) {
        folders << m_list->item(i)->text();
    }
    saveFolders(folders);
    accept();
}

} // namespace app::panels
