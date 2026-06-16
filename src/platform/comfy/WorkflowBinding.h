#pragma once

#include <QHash>
#include <QJsonValue>
#include <QString>

namespace platform::comfy {

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowBinding
//
// ワークフロー JSON 内の 1 ノードに対するパッチ命令。
//
// 使用例:
//   auto b = WorkflowBinding::loadImage("4", "input.png");
//   auto b = WorkflowBinding::kSampler("7").seed(42).steps(20).cfg(7.5);
//   auto b = WorkflowBinding::clipText("2", "a cat");
//   auto b = WorkflowBinding::controlNet("10").strength(0.8).image("ctrl.png");
//
// ノードIDが不明な場合は classType + occurrence で検索:
//   auto b = WorkflowBinding::byClass("CLIPTextEncode", 0).set("text", "positive");
// ─────────────────────────────────────────────────────────────────────────────
class WorkflowBinding {
public:
    enum class Target {
        LoadImage,
        SaveImage,
        CLIPTextEncode,
        KSampler,
        ControlNet,
        Generic,
    };

    // ── ファクトリ (nodeId 指定) ──────────────────────────────────────────────
    static WorkflowBinding loadImage (const QString& nodeId, const QString& filename);
    static WorkflowBinding saveImage (const QString& nodeId, const QString& filenamePrefix);
    static WorkflowBinding clipText  (const QString& nodeId, const QString& text);
    static WorkflowBinding kSampler  (const QString& nodeId);
    static WorkflowBinding controlNet(const QString& nodeId);

    // ── ファクトリ (classType 検索) ───────────────────────────────────────────
    // occurrence: 同じ classType が複数ある場合のインデックス (0 始まり)
    static WorkflowBinding byClass(const QString& classType, int occurrence = 0);

    // ── KSampler パラメータ (fluent) ─────────────────────────────────────────
    WorkflowBinding& seed      (int    seed);
    WorkflowBinding& steps     (int    steps);
    WorkflowBinding& cfg       (double cfg);
    WorkflowBinding& denoise   (double denoise);
    WorkflowBinding& sampler   (const QString& samplerName);
    WorkflowBinding& scheduler (const QString& schedulerName);

    // ── ControlNet パラメータ (fluent) ───────────────────────────────────────
    WorkflowBinding& strength  (double strength);
    WorkflowBinding& image     (const QString& filename);

    // ── 汎用フィールドパッチ (fluent / escape hatch) ─────────────────────────
    WorkflowBinding& set(const QString& key, const QJsonValue& value);

    // ── アクセサ ─────────────────────────────────────────────────────────────
    Target          target    () const noexcept { return m_target; }
    const QString&  nodeId    () const noexcept { return m_nodeId; }
    const QString&  classType () const noexcept { return m_classType; }
    int             occurrence() const noexcept { return m_occurrence; }

    // patches: "input key" → QJsonValue
    const QHash<QString, QJsonValue>& patches() const noexcept { return m_patches; }

private:
    explicit WorkflowBinding(Target target, const QString& nodeId,
                              const QString& classType = {}, int occurrence = 0);

    Target  m_target     {Target::Generic};
    QString m_nodeId;     // 空なら classType + occurrence で検索
    QString m_classType;
    int     m_occurrence {0};
    QHash<QString, QJsonValue> m_patches;
};

} // namespace platform::comfy
