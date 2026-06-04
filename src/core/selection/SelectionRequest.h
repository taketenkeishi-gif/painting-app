#pragma once

#include <string>
#include <vector>

#include "core/common/Point.h"
#include "core/common/Rect.h"
#include "core/selection/SelectionMask.h"  // SelectionOp

namespace core {

// ─────────────────────────────────────────────────────────────────────────────
// SelectionRequest
//
// Tool が SelectionEngine に渡す「選択要求」。
// ツール側はジオメトリ・パラメーターを記述するだけで、
// 実際のラスタライズや AI 推論は Engine + Provider が担う。
// ─────────────────────────────────────────────────────────────────────────────
struct SelectionRequest {

  // ── 選択タイプ ──────────────────────────────────────────────────────────────
  enum class Type {
    Rectangle,     ///< 矩形選択
    Ellipse,       ///< 楕円選択（将来対応）
    FreeLasso,     ///< フリーハンドラッソ
    PolygonLasso,  ///< 多角形ラッソ（クリックで頂点を追加）
    MagicWand,     ///< 類似色自動選択（floodFill）
    Object,        ///< 非連続類似色 / 将来: SAM オブジェクト選択
    Subject,       ///< 被写体選択（将来: AI 全体セグメント）
    Semantic,      ///< 意味的選択「髪」「服」など（将来: AI）
    AI,            ///< 汎用 AI セグメンテーション（SAM2 / BiRefNet 等）
  };
  Type type {Type::Rectangle};

  // ── 既存選択との合成方法 ─────────────────────────────────────────────────
  SelectionOp op {SelectionOp::New};

  // ── ジオメトリ ────────────────────────────────────────────────────────────
  Rect               rect   {};        ///< Rectangle / Ellipse
  std::vector<Point> points {};        ///< FreeLasso / PolygonLasso の輪郭点列
  Point              seed   {0, 0};   ///< MagicWand の種点

  // ── 共通パラメーター ──────────────────────────────────────────────────────
  int  tolerance    {16};    ///< MagicWand 等の色差許容値
  int  feather      {0};     ///< フェザー半径 (px)
  bool antiAlias    {true};  ///< 輪郭アンチエイリアス
  bool edgeAware    {false}; ///< エッジスナップ利用
  int  gapClose     {0};     ///< ギャップ補完半径 (px)
  int  expandPixels {0};     ///< 確定後に拡張する量 (px)

  // ── 参照レイヤー ──────────────────────────────────────────────────────────
  enum class ReferenceMode {
    CurrentLayer,    ///< アクティブレイヤーのみ参照
    AllLayers,       ///< 合成済み画像を参照
    VisibleLayers,   ///< 表示中レイヤーの合成を参照（将来）
  };
  ReferenceMode referenceMode {ReferenceMode::AllLayers};

  bool contiguous {true};  ///< MagicWand: 連続領域のみ選択

  // ── Object Select / AI ────────────────────────────────────────────────────
  /// Object Select: ブラシストロークのヒント座標列。
  /// SAM2 等で「この領域を選択」のガイドとして使用。
  std::vector<Point> strokeHint {};

  /// 最大候補数。Provider は candidates[0..n-1] を返してよい。
  int maxCandidates {1};

  // ── 意味的選択（将来） ────────────────────────────────────────────────────
  std::string semanticLabel {};  ///< Semantic / AI: "hair", "clothes", etc.
};

} // namespace core
