#pragma once

#include "core/selection/SelectionResult.h"

namespace core {

// ─────────────────────────────────────────────────────────────────────────────
// SelectionRefiner
//
// SelectionResult に後処理 (フェザー / 拡張 / 収縮 / スムージング / エッジ
// スナップ) を適用するユーティリティ。
//
// 使い方:
//   SelectionRefiner::RefinementOptions opts;
//   opts.featherRadius = 3;
//   opts.expand = 2;
//   SelectionRefiner::apply(result, opts);
//
// すべてのメソッドは static — インスタンス化不要。
// ─────────────────────────────────────────────────────────────────────────────
class SelectionRefiner {
public:
  SelectionRefiner() = delete;

  // ── オプション ────────────────────────────────────────────────────────────
  struct RefinementOptions {
    int  featherRadius         {0};     ///< フェザー半径 (px)
    int  expandRadius          {0};     ///< 選択拡張 (px)
    int  contractRadius        {0};     ///< 選択収縮 (px)
    int  smoothRadius          {0};     ///< スムージング半径 (px)
    bool edgeSnap              {false}; ///< エッジスナップ (edgeMap が必要)
    int  gapCloseRadius        {0};     ///< ギャップ補完: expand→contract (px)
    int  removeIslandsMinArea  {0};     ///< これ未満の独立島を除去 (px²)
    int  fillHolesMaxArea      {0};     ///< これ以下の穴を塗り潰す (px²)
    bool antiAliasEdge         {false}; ///< 境界 1px ガウス (エッジ AA)
  };

  /// result.mask に RefinementOptions を適用する。
  /// edgeSnap が true で result.edgeMap が空の場合は無視される。
  static void apply(SelectionResult& result, const RefinementOptions& opts);
};

} // namespace core
