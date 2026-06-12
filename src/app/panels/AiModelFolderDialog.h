#pragma once

#include <QDialog>
#include <QStringList>

class QListWidget;
class QPushButton;

namespace app::panels {

// ─────────────────────────────────────────────────────────────────────────────
// AiModelFolderDialog — AI モデル検索フォルダの管理ダイアログ
//
// QSettings("taketenkeishi","LayeredPaintApp") の "ai/modelSearchFolders" に
// フォルダパスのリストを永続保存する。
//
// 使い方:
//   AiModelFolderDialog dlg(parent);
//   dlg.exec();
//   // 以降 AiModelFolderDialog::loadFolders() で取得可能
// ─────────────────────────────────────────────────────────────────────────────
class AiModelFolderDialog : public QDialog {
    Q_OBJECT
public:
    explicit AiModelFolderDialog(QWidget* parent = nullptr);

    // QSettings から登録済みフォルダ一覧を取得（静的ユーティリティ）
    static QStringList loadFolders();
    // QSettings に保存
    static void saveFolders(const QStringList& folders);

private slots:
    void onAddFolder();
    void onRemoveFolder();
    void onAccepted();

private:
    void setupUi();

    QListWidget*  m_list       {nullptr};
    QPushButton*  m_addBtn     {nullptr};
    QPushButton*  m_removeBtn  {nullptr};

    static constexpr const char* kSettingsKey = "ai/modelSearchFolders";
};

} // namespace app::panels
