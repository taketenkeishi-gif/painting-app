#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/selection/SelectionMask.h"

namespace core {

// ─────────────────────────────────────────────────────────────────────────────
// SelectionResult
//
// SelectionEngine が Provider から受け取り、呼び出し元に返す選択結果。
// Classic 選択では mask のみ。AI/SAM 選択では追加マップが埋まる。
//
// ── フィールド説明 ─────────────────────────────────────────────────────────
//  mask          : 最終マスク (0=非選択 255=完全選択, 中間値=フェザー)
//  confidenceMap : ピクセルごとの確信度 [0.0-1.0]。AI のみ。
//  edgeMap       : エッジ強度 [0-255]。エッジスナップ用。
//  objectIdMap   : インスタンス ID (0=背景)。PSD 分解用。
//  semanticLabels: セグメント領域のラベル + 確信度。
// ─────────────────────────────────────────────────────────────────────────────
struct SelectionResult {

  // ── コアマスク (常に有効) ──────────────────────────────────────────────────
  SelectionMask mask;

  // ── AI 選択用メタデータ ───────────────────────────────────────────────────

  /// ピクセルごとの確信度マップ [0.0-1.0]。
  /// サイズ = mask.width() * mask.height()。空 = Classic 選択。
  std::vector<float> confidenceMap;

  /// エッジ強度マップ [0-255]。SelectionRefiner のエッジスナップ用。
  /// 空 = 未計算。
  std::vector<std::uint8_t> edgeMap;

  /// インスタンス ID マップ (0=背景, 1, 2, …=オブジェクト)。
  /// PSD 自動レイヤー分解で使用予定。
  /// 空 = 未計算。
  std::vector<std::uint32_t> objectIdMap;

  // ── 意味ラベル ────────────────────────────────────────────────────────────
  struct SemanticRegion {
    std::uint32_t objectId   {0};     ///< objectIdMap 対応 ID
    std::string   label      {};      ///< "hair", "clothes", "skin", etc.
    float         confidence {0.0f};  ///< 確信度 [0.0-1.0]
  };
  /// セグメントされた各領域の意味ラベル。Semantic / AI 選択のみ。
  std::vector<SemanticRegion> semanticLabels;

  // ── 候補マスク (Object Select / AI のみ) ─────────────────────────────────
  /// Provider が複数の候補を返す場合 (SAM2 の multiple masks など)。
  /// candidates[0] が最も確信度の高い候補。
  /// Classic Provider は常に空。
  std::vector<SelectionMask> candidates;

  // ── メタデータ ────────────────────────────────────────────────────────────
  bool        fromAI        {false};  ///< AI Provider の結果か
  std::string providerName  {};       ///< 生成した Provider 名 ("Classic", "SAM2", …)

  // ── ヘルパー ──────────────────────────────────────────────────────────────
  bool hasConfidenceMap() const noexcept { return !confidenceMap.empty(); }
  bool hasEdgeMap()       const noexcept { return !edgeMap.empty(); }
  bool hasObjectIdMap()   const noexcept { return !objectIdMap.empty(); }
  bool hasSemanticLabels()const noexcept { return !semanticLabels.empty(); }
};

} // namespace core
