#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "core/common/FPoint.h"
#include "core/layer/Layer.h"
#include "core/tools/ITool.h"

namespace core {

/// ベクター制御点編集ツール。
/// VectorPath の点を選択・ドラッグ移動・削除できる。
/// ラスタレイヤーには作用しない（ツール切替で除外済み）。
class VectorEditTool : public ITool {
public:
  ToolKind kind() const noexcept override { return ToolKind::VectorEdit; }
  std::string_view displayName() const noexcept override { return "Vector Edit"; }

  ToolResult onPointerPress(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel(ToolContext& context) override;
  ToolResult onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;
  ToolOverlayState overlay() const override;

  /// AppController から呼ばれる: 選択点を削除してアンドゥ用の変更を返す。
  /// アクティブレイヤーが VectorLayer の場合のみ動作。
  /// 戻り値 true = 変更あり。
  bool deleteSelectedPoints(Document& doc);

  /// 現在選択されている点が存在するか。
  bool hasSelection() const noexcept { return !m_selectedPoints.empty(); }

private:
  // hit-test radius (canvas px)
  static constexpr float kHitRadius = 8.0f;

  /// フラット化した全点リスト: {pathIdx, pointIdx}
  struct PointRef { int pathIdx; int pointIdx; };

  /// オーバーレイ計算用の平坦化キャッシュ（overlay() 呼び出しごとに設定済み）
  std::vector<PointRef>    m_pointRefs;   ///< path/point インデックス
  std::vector<FPoint>      m_allPoints;   ///< キャンバス座標

  /// 選択点セット: m_allPoints へのフラットインデックス
  std::unordered_set<int>  m_selectedPoints;

  bool   m_dragging     {false};
  FPoint m_dragStart    {0.0f, 0.0f};
  FPoint m_dragLast     {0.0f, 0.0f};

  /// ドラッグ開始前のレイヤースナップショット（アンドゥ用）
  std::vector<VectorPath>  m_beforePaths;
  bool m_hasBeforeSnapshot {false};

  /// アクティブレイヤーの VectorPath 一覧を返す（nullptr = 非ベクターレイヤー）
  static std::vector<VectorPath>* activeVectorPaths(Document& doc);

  /// 点のフラットリストを更新する
  void rebuildPointCache(const std::vector<VectorPath>& paths);

  /// canvas 座標 pt に最も近いフラットインデックスを返す。
  /// kHitRadius 以内に点がなければ -1。
  int hitTest(FPoint pt) const;
};

} // namespace core
