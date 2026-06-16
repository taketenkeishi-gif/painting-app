#pragma once

#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QString>

#include "app/ai/WorkflowPreset.h"

namespace app::panels {

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowPresetManager
//
// プリセット一覧の永続化・Workflow JSON キャッシュを一元管理する。
// UI 非依存 — AiPanel 以外からも利用可能。
//
// ── type の扱い ────────────────────────────────────────────────────────────────
//   - addOrUpdate() は受け取った preset の type をそのまま保存する
//   - import / legacy migration 時のみ preset.inferType() を呼ぶ
//   - Binding 変更では既存 type を維持する（呼び元の責任）
// ─────────────────────────────────────────────────────────────────────────────
class WorkflowPresetManager {
public:
    WorkflowPresetManager() = default;

    // QSettings に保存されたプリセットと最終選択インデックスを読み込む。
    // 旧フォーマット (ai/recentWorkflows) が存在する場合は自動移行する。
    void load();

    // 現在の状態を QSettings に書き出す。
    void save() const;

    // ── プリセット操作 ────────────────────────────────────────────────────────

    const QList<WorkflowPreset>& presets() const { return m_presets; }

    // 同一 workflowPath が存在すれば上書き、なければ先頭に追加する。
    // type は preset に設定済みの値をそのまま保存する。
    void addOrUpdate(const WorkflowPreset& preset);

    void removeAt(int index);

    int indexByPath(const QString& path) const;
    int indexByName(const QString& name) const;

    // ── 最後に使ったプリセット ────────────────────────────────────────────────

    int  lastUsedIndex() const { return m_lastUsedIndex; }
    void setLastUsedIndex(int i);   // QSettings にも即時書き込み

    // ── Workflow JSON キャッシュ ──────────────────────────────────────────────

    QJsonObject cachedWorkflow(const QString& path) const;
    void        cacheWorkflow(const QString& path, const QJsonObject& json);

private:
    static constexpr int kMaxPresets = 20;

    QList<WorkflowPreset>              m_presets;
    int                                m_lastUsedIndex { -1 };
    mutable QMap<QString, QJsonObject> m_workflowCache;
};

} // namespace app::panels
