#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>
#include <string_view>

#include "core/common/FPoint.h"
#include "core/common/Point.h"
#include "core/selection/SelectionMask.h"
#include "core/tools/ITool.h"

namespace core {

class RectSelectionTool : public ITool {
public:
  /// 多角形ラッソの1頂点。ベジェ曲線かコーナーかを保持。
  struct PolyLassoNode {
    FPoint anchor;
    FPoint handleOut;  ///< アウト方向ハンドル（anchor相対）。smooth=false なら未使用。
    bool   smooth {false};
  };

  enum class Mode {
    Rectangle,
    Lasso,
    PolygonLasso,   ///< クリックで頂点追加、ダブルクリックで確定
    AutoSelect,     ///< 類似色 flood fill (contiguous)
    ObjectSelect,   ///< 全体類似色 / SAM2 (non-contiguous, all layers)
  };

  ToolKind kind() const noexcept override { return ToolKind::RectSelection; }
  std::string_view displayName() const noexcept override { return "Selection"; }

  ToolResult onPointerPress  (ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerMove   (ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onPointerRelease(ToolContext& context, const ToolPointerEvent& event) override;
  ToolResult onCancel        (ToolContext& context) override;
  ToolResult onWheel         (ToolContext& context, int deltaSteps, const ToolPointerEvent& event) override;
  ToolOverlayState overlay() const override;

  void setMode(Mode mode) noexcept {
    if (m_mode != mode) {
      cancelPolygon();
    }
    m_mode = mode;
  }
  Mode mode() const noexcept { return m_mode; }
  bool isPolyInProgress() const noexcept { return m_polyInProgress; }
  /// Enter キー確定: 現在のノード列でそのまま選択を確定する
  ToolResult confirmPolygonLasso(ToolContext& context);

  void setAutoSelectThreshold   (int v)  noexcept { m_autoSelectThreshold    = std::clamp(v, 0, 255); }
  void setAutoSelectContiguous  (bool v) noexcept { m_autoSelectContiguous   = v; }
  void setAutoSelectReferAllLayers(bool v) noexcept { m_autoSelectReferAllLayers = v; }

  void setFeatherRadius      (int v)  noexcept { m_featherRadius = std::max(0, v); }
  void setSelectionAntiAlias (bool v) noexcept { m_antiAlias = v; }
  void setSelectionOp        (SelectionOp op) noexcept { m_selectionOp = op; }
  void setExpandPixels       (int v)  noexcept { m_expandPixels = std::max(0, v); }
  void setGapCloseRadius     (int v)  noexcept { m_gapCloseRadius = std::max(0, v); }
  void setEdgeAware          (bool v) noexcept { m_edgeAware = v; }
  int  featherRadius         () const noexcept { return m_featherRadius; }
  bool selectionAntiAlias    () const noexcept { return m_antiAlias; }
  SelectionOp selectionOp    () const noexcept { return m_selectionOp; }

private:
  SelectionOp opFromEvent(const ToolPointerEvent& e) const noexcept;
  static Rect normalizeRect(const Point& a, const Point& b);
  static Point constrainToSquare(const Point& start, const Point& current) noexcept;
  bool applyAutoSelect  (ToolContext& context, const Point& seed, SelectionOp op);
  bool applyObjectSelect(ToolContext& context, SelectionOp op);
  bool applyLasso       (ToolContext& context, SelectionOp op);
  bool applyPolygon     (ToolContext& context, SelectionOp op);
  void applyFeather     (ToolContext& context) const;
  void cancelPolygon    () noexcept;

  static bool isInsideSelection(const SelectionMask& sel, const Point& pt) noexcept;

  // ハンドルヒットテスト（8方向: 0-7 = TL/TC/TR/ML/MR/BL/BC/BR, -1 = なし）
  int hitTestHandle(const SelectionMask& sel, const Point& pt, int tolerance = 4) const noexcept;
  // 矩形リサイズ: ハンドルindex → 新矩形を計算
  Rect resizeRectByHandle(const Rect& origRect, int handleIdx, const Point& delta, bool constrainAspect) const noexcept;

  // ── モード ────────────────────────────────────────────────────────────────
  Mode         m_mode {Mode::Rectangle};

  // ── 矩形 / ラッソ共通 ─────────────────────────────────────────────────────
  bool         m_selecting {false};
  SelectionOp  m_opAtPress {SelectionOp::New};
  Point m_start {0, 0};
  Point m_current {0, 0};
  std::vector<Point> m_lassoPoints;

  // ── 選択マーキー移動 ───────────────────────────────────────────────────────
  bool  m_movingMarquee {false};
  Point m_moveOrigin {0, 0};
  int   m_moveDx {0};
  int   m_moveDy {0};
  std::vector<std::uint8_t> m_savedMask;
  int   m_savedMaskW {0};
  int   m_savedMaskH {0};

  // ── 選択範囲ハンドル（コーナー + エッジ中央） ────────────────────────────────
  bool  m_resizingHandle {false};
  int   m_activeHandle {-1};  // 0-7: TL/TC/TR/ML/MR/BL/BC/BR
  Rect  m_savedHandleRect;    // ドラッグ開始時の矩形

  // ── 多角形ラッソ ───────────────────────────────────────────────────────────
  bool  m_polyInProgress  {false};
  std::vector<PolyLassoNode> m_polyNodes;   ///< 確定ノード
  FPoint m_polyMouse      {0, 0};           ///< マウス追従用
  bool   m_polyPressing   {false};          ///< プレス中（未確定ノードを設置中）
  FPoint m_polyPressAnchor{0, 0};           ///< プレス位置（アンカー候補）
  FPoint m_polyDragHandle {0, 0};           ///< ドラッグ量（アンカー相対）
  bool   m_polyHasDrag    {false};          ///< ドラッグ閾値を超えたか

  // ── 自動選択 ──────────────────────────────────────────────────────────────
  int  m_autoSelectThreshold      {16};
  bool m_autoSelectContiguous      {true};
  bool m_autoSelectReferAllLayers  {true};

  // ── Object Select ストロークヒント ────────────────────────────────────────
  std::vector<Point> m_strokeHint; ///< ブラシストロークの座標列 (ObjectSelect)

  // ── フェザー / アンチエイリアス / 拡張 / エッジ ────────────────────────
  int         m_featherRadius   {0};
  bool        m_antiAlias       {true};
  SelectionOp m_selectionOp     {SelectionOp::New};
  int         m_expandPixels    {0};
  int         m_gapCloseRadius  {0};
  bool        m_edgeAware       {false};
};

} // namespace core
