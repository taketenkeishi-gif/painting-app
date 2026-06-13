#pragma once

#include <QJsonObject>
#include <QStringList>

#include "platform/comfy/WorkflowBinding.h"

namespace platform::comfy {

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowDocument
//
// ComfyUI workflow JSON のロード・パッチ・エクスポート。
//
// ── 完了フロー ───────────────────────────────────────────────────────────────
//   auto doc = WorkflowDocument::load("my_workflow.json");
//   doc.apply(WorkflowBinding::loadImage("4", uploadedFilename));
//   doc.apply(WorkflowBinding::kSampler("7").seed(42).steps(20).cfg(7.5));
//   doc.apply(WorkflowBinding::clipText("2", "positive prompt"));
//   doc.apply(WorkflowBinding::clipText("3", "negative prompt"));
//   client.queuePrompt(doc.toJson());
//
// ── classType 検索 (ノードID不明時) ─────────────────────────────────────────
//   doc.apply(WorkflowBinding::byClass("CLIPTextEncode", 0).set("text", "positive"));
//   doc.apply(WorkflowBinding::byClass("CLIPTextEncode", 1).set("text", "negative"));
// ─────────────────────────────────────────────────────────────────────────────
class WorkflowDocument {
public:
    WorkflowDocument() = default;

    /// ファイルから JSON をロード。失敗時は !isValid()。
    static WorkflowDocument load(const QString& filePath, bool* ok = nullptr);

    /// QJsonObject から構築（ComfyUI API 形式またはファイル形式どちらも可）
    static WorkflowDocument fromJson(const QJsonObject& obj);

    bool isValid() const noexcept { return !m_workflow.isEmpty(); }

    // ── パッチ操作 ────────────────────────────────────────────────────────────

    /// WorkflowBinding を適用。対象ノードが見つからない場合は false。
    bool apply(const WorkflowBinding& binding);

    /// 直接パッチ (escape hatch)
    bool setInput(const QString& nodeId, const QString& key, const QJsonValue& value);

    // ── クエリ ────────────────────────────────────────────────────────────────

    /// 全ノード ID 一覧
    QStringList nodeIds() const;

    /// classType が一致するノード ID 一覧
    QStringList findNodesByClass(const QString& classType) const;

    /// ノードの class_type を返す。存在しない場合は空文字。
    QString nodeClass(const QString& nodeId) const;

    /// ノードの inputs オブジェクトを返す。
    QJsonObject nodeInputs(const QString& nodeId) const;

    // ── 全ノードスキャン (class_type 非依存) ──────────────────────────────────

    /// 特定の入力名を持つノード ID 一覧を返す。
    QStringList findNodesByInputName(const QString& inputName) const;

    /// steps / cfg / denoise / seed / noise_seed を 2 つ以上持つノード一覧。
    QStringList findSamplerNodes() const;

    /// インペイントワークフロー内でマスク画像を供給するノード ID を返す。
    /// ImageToMask ノードの image 入力を逆引きして上流 LoadImage を特定する。
    /// 見つからない場合は空文字。
    QString findMaskSourceNode() const;

    /// text / prompt / positive / tags / editor_state を持つノード一覧。
    QStringList findPromptNodes() const;

    /// ckpt_name / checkpoint / model_name を持つノード一覧。
    QStringList findCheckpointNodes() const;

    // ── 値読み取り (inputs / widgets_values + SDXLTagEditor 対応) ────────────

    /// ポジティブテキストを返す。
    /// editor_state → prefix + mainTags / text / prompt / positive / tags
    QString readPositiveText(const QString& nodeId) const;

    /// ネガティブテキストを返す。
    /// editor_state → negative タグ / text / prompt / negative
    QString readNegativeText(const QString& nodeId) const;

    /// 後方互換エイリアス (readPositiveText)。
    QString readClipText(const QString& nodeId) const;

    /// Sampler 系パラメーターを返す (seed/noise_seed / steps / cfg/guidance_scale / denoise/strength)。
    struct KSamplerParams {
        int    seed    {-1};
        int    steps   {20};
        double cfg     {7.5};
        double denoise {0.75};
    };
    KSamplerParams readKSamplerParams(const QString& nodeId) const;

    /// 最初に見つかったチェックポイント名を返す (class_type 非依存スキャン)。
    QString readCheckpointName() const;

    // ── エクスポート ──────────────────────────────────────────────────────────

    /// ComfyUiClient::queuePrompt() へ直接渡せる QJsonObject を返す。
    QJsonObject toJson() const { return m_workflow; }

    /// ロードエラーの理由を返す。
    QString errorString() const { return m_error; }

private:
    QJsonObject m_workflow;
    QString     m_error;
};

} // namespace platform::comfy
