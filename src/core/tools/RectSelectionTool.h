#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>
#include <string_view>

#include "core/common/Point.h"
#include "core/selection/SelectionMask.h"
#include "core/tools/ITool.h"

namespace core {

class RectSelectionTool : public ITool {
public:
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
      m_committedLassoPoints.clear();
      cancelPolygon();
    }
    m_mode = mode;
  }
  Mode mode() const noexcept { return m_mode; }

  void setAutoSelectThreshold   (int v)  noexcept { m_autoSelectThreshold    = std::clamp(v, 0, 255); }
  void setAutoSelectContiguous  (bool v) noexcept { m_autoSelectContiguous   = v; }
  void setAutoSelectReferAllLayers(bool v) noexcept { m_autoSelectReferAllLayers = v; }

  void setFeatherRadius      (int v)  noexcept { m_featherRadius = std::max(0, v); }
  void setSelectionAntiAlias (bool v) noexcept { m_antiAlias = v; }
  int  featherRadius         () const noexcept { return m_featherRadius; }
  bool selectionAntiAlias    () const noexcept { return m_antiAlias; }

private:
  static SelectionOp opFromEvent(const ToolPointerEvent& e) noexcept;
  static Rect normalizeRect(const Point& a, const Point& b);
  static Point constrainToSquare(const Point& start, const Point& current) noexcept;
  bool applyAutoSelect  (ToolContext& context, const Point& seed, SelectionOp op);
  bool applyLasso       (ToolContext& context, SelectionOp op);
  bool applyPolygon     (ToolContext& context, SelectionOp op);
  void applyFeather     (ToolContext& context) const;
  void cancelPolygon    () noexcept;

  static bool isInsideSelection(const SelectionMask& sel, const Point& pt) noexcept;

  /// Scan-line polygon fill (高速)
  static std::vector<std::uint8_t> scanFillPolygon(
      const std::vector<Point>& poly, int width, int height);
  static std::vector<std::uint8_t> antiAliasedFillPolygon(
      const std::vector<Point>& poly, int width, int height);

  static int colorDistance(const Color& a, const Color& b) noexcept;

  // ── モード ────────────────────────────────────────────────────────────────
  Mode         m_mode {Mode::Rectangle};

  // ── 矩形 / ラッソ共通 ─────────────────────────────────────────────────────
  bool         m_selecting {false};
  SelectionOp  m_opAtPress {SelectionOp::New};
  Point m_start {0, 0};
  Point m_current {0, 0};
  std::vector<Point> m_lassoPoints;
  std::vector<Point> m_committedLassoPoints;

  // ── 選択マーキー移動 ───────────────────────────────────────────────────────
  bool  m_movingMarquee {false};
  Point m_moveOrigin {0, 0};
  int   m_moveDx {0};
  int   m_moveDy {0};
  std::vector<std::uint8_t> m_savedMask;
  int   m_savedMaskW {0};
  int   m_savedMaskH {0};

  // ── 多角形ラッソ ───────────────────────────────────────────────────────────
  bool m_polyInProgress {false};
  std::vector<Point> m_polyPoints;  // 確定頂点
  Point m_polyMouse {0, 0};         // マウス追従用

  // ── 自動選択 ──────────────────────────────────────────────────────────────
  int  m_autoSelectThreshold      {16};
  bool m_autoSelectContiguous      {true};
  bool m_autoSelectReferAllLayers  {true};

  // ── フェザー / アンチエイリアス ─────────────────────────────────────────
  int  m_featherRadius {0};
  bool m_antiAlias     {true};
};

} // namespace core
