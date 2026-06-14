#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>

#include "app/panels/WorkflowBindingDialog.h"

namespace app::panels {

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowType
//
// ワークフローの生成種別。
// UI / 将来の API 振り分けに使う。
// 決定優先順:
//   1. ユーザーが明示的に設定した保存済み type
//   2. import / legacy migration 時のみ inferType() でバインドから推定
// ─────────────────────────────────────────────────────────────────────────────
enum class WorkflowType { Txt2Img, Img2Img, Inpaint };

// ─────────────────────────────────────────────────────────────────────────────
// LoraEntry
//
// LoRA 一本分の設定。LoraLoader ノードへのパッチに使う。
// ─────────────────────────────────────────────────────────────────────────────
struct LoraEntry {
    QString name;            // LoRA ファイル名 (ComfyUI lora_name フィールド)
    double  modelStrength { 1.0 };
    double  clipStrength  { 1.0 };
    QString triggerWords; // 任意。プロンプトへの自動挿入用。

    bool isEmpty() const { return name.isEmpty(); }

    QJsonObject toJson() const;
    static LoraEntry fromJson(const QJsonObject& obj);
};

inline QString workflowTypeLabel(WorkflowType t) {
    switch (t) {
    case WorkflowType::Txt2Img: return QStringLiteral("txt2img");
    case WorkflowType::Img2Img: return QStringLiteral("img2img");
    case WorkflowType::Inpaint: return QStringLiteral("inpaint");
    }
    return QStringLiteral("txt2img");
}

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowPreset
//
// ワークフロー JSON ファイル + ノードバインド設定 + 種別を一体化したモデル。
// WorkflowPresetManager が永続化を担当する。
//
// ControlNet / LoRA / Upscale など将来の追加フィールドはここに拡張する。
// ─────────────────────────────────────────────────────────────────────────────
struct WorkflowPreset {
    QString               name;
    WorkflowType          type         { WorkflowType::Txt2Img };
    QString               workflowPath;
    WorkflowBindingConfig binding;

    // モデル設定 (省略可 — 空なら workflow.json のデフォルト値を使う)
    QString          checkpoint;   // CheckpointLoader の ckpt_name
    QList<LoraEntry> loras;        // LoraLoader スロット (順序 = ノード検出順)

    bool isValid() const { return !workflowPath.isEmpty(); }

    // バインド内容から type を推定する。
    // 用途: import 時 / 旧フォーマット移行時のみ。保存済み type には使わない。
    WorkflowType inferType() const {
        if (!binding.maskNodeId.isEmpty())       return WorkflowType::Inpaint;
        if (!binding.inputImageNodeId.isEmpty()) return WorkflowType::Img2Img;
        return WorkflowType::Txt2Img;
    }

    QJsonObject toJson() const;
    static WorkflowPreset fromJson(const QJsonObject& obj);
};

} // namespace app::panels
