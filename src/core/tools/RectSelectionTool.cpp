#include "core/tools/RectSelectionTool.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>


#include "core/selection/SelectionEngine.h"
#include "core/selection/SelectionRequest.h"

namespace core {

// ── 修飾キーからオペレーション決定 ───────────────────────────────────────
SelectionOp RectSelectionTool::opFromEvent(const ToolPointerEvent& e) const noexcept {
  // Modifier keys temporarily override the persistent op
  if (e.shift && e.alt) return SelectionOp::Intersect;
  if (e.shift)          return SelectionOp::Add;
  if (e.alt)            return SelectionOp::Subtract;
  return m_selectionOp;
}

// ── 矩形正規化 ────────────────────────────────────────────────────────────
Rect RectSelectionTool::normalizeRect(const Point& a, const Point& b) {
  const int left   = std::min(a.x, b.x);
  const int top    = std::min(a.y, b.y);
  const int right  = std::max(a.x, b.x);
  const int bottom = std::max(a.y, b.y);
  return Rect {left, top, right - left + 1, bottom - top + 1};
}

// ── Shift制約: 正方形 ────────────────────────────────────────────────────
Point RectSelectionTool::constrainToSquare(const Point& start, const Point& cur) noexcept {
  const int dx = cur.x - start.x;
  const int dy = cur.y - start.y;
  const int side = std::max(std::abs(dx), std::abs(dy));
  return Point {
    start.x + (dx >= 0 ? side : -side),
    start.y + (dy >= 0 ? side : -side)
  };
}

// ── 選択範囲内かチェック ──────────────────────────────────────────────────
bool RectSelectionTool::isInsideSelection(const SelectionMask& sel, const Point& pt) noexcept {
  return sel.hasSelection() && sel.contains(pt.x, pt.y);
}

// ── 多角形ラッソリセット ──────────────────────────────────────────────────
void RectSelectionTool::cancelPolygon() noexcept {
  m_polyInProgress  = false;
  m_polyPressing    = false;
  m_polyHasDrag     = false;
  m_polyNodes.clear();
}

// Enter キー確定: ノードをポップせずそのまま選択確定
ToolResult RectSelectionTool::confirmPolygonLasso(ToolContext& context) {
  ToolResult result;
  if (m_polyNodes.size() >= 3) {
    result.selectionChanged = applyPolygon(context, m_opAtPress);
    if (result.selectionChanged) applyFeather(context);
    result.viewportChanged = true;
  }
  cancelPolygon();
  return result;
}

// ベジェノード列を密な Point 列に平坦化（既存ラスタライザに渡す）
static std::vector<core::Point> flattenPolyNodes(
    const std::vector<RectSelectionTool::PolyLassoNode>& nodes)
{
  std::vector<core::Point> pts;
  const int N = static_cast<int>(nodes.size());
  if (N == 0) return pts;
  pts.reserve(N * 8);
  for (int i = 0; i < N; ++i) {
    const auto& A = nodes[i];
    const auto& B = nodes[(i + 1) % N];
    if (!A.smooth && !B.smooth) {
      pts.push_back({static_cast<int>(A.anchor.x), static_cast<int>(A.anchor.y)});
    } else {
      const core::FPoint p0 = A.anchor;
      const core::FPoint p1 = A.smooth
          ? core::FPoint{A.anchor.x + A.handleOut.x, A.anchor.y + A.handleOut.y}
          : A.anchor;
      const core::FPoint p2 = B.smooth
          ? core::FPoint{B.anchor.x - B.handleOut.x, B.anchor.y - B.handleOut.y}
          : B.anchor;
      const core::FPoint p3 = B.anchor;
      const float dx = p3.x - p0.x, dy = p3.y - p0.y;
      const int steps = std::max(8, static_cast<int>(std::sqrt(dx*dx + dy*dy) * 0.5f));
      for (int j = 0; j < steps; ++j) {
        const float t = static_cast<float>(j) / static_cast<float>(steps);
        const float u = 1.f - t;
        const float x = u*u*u*p0.x + 3.f*u*u*t*p1.x + 3.f*u*t*t*p2.x + t*t*t*p3.x;
        const float y = u*u*u*p0.y + 3.f*u*u*t*p1.y + 3.f*u*t*t*p2.y + t*t*t*p3.y;
        pts.push_back({static_cast<int>(x + 0.5f), static_cast<int>(y + 0.5f)});
      }
    }
  }
  return pts;
}

// ── ハンドルヒットテスト ──────────────────────────────────────────────────
int RectSelectionTool::hitTestHandle(const SelectionMask& sel, const Point& pt, int tolerance) const noexcept {
  if (!sel.hasSelection()) return -1;

  const auto boundsOpt = sel.boundingRect();
  if (!boundsOpt.has_value()) return -1;

  const Rect r = boundsOpt.value();
  const int x1 = r.x, y1 = r.y;
  const int x2 = r.x + r.width - 1, y2 = r.y + r.height - 1;
  const int mx = (x1 + x2) / 2, my = (y1 + y2) / 2;

  const int dx = std::abs(pt.x);
  const int dy = std::abs(pt.y);

  // ハンドル候補: TL, TC, TR, ML, MR, BL, BC, BR
  const Point handles[8] = {
    {x1, y1}, {mx, y1}, {x2, y1},  // TL, TC, TR
    {x1, my},           {x2, my},  // ML,     MR
    {x1, y2}, {mx, y2}, {x2, y2}   // BL, BC, BR
  };

  for (int i = 0; i < 8; ++i) {
    const int hdx = pt.x - handles[i].x;
    const int hdy = pt.y - handles[i].y;
    if (hdx * hdx + hdy * hdy <= tolerance * tolerance) {
      return i;
    }
  }

  return -1;
}

// ── 矩形リサイズ（ハンドル別） ────────────────────────────────────────────
Rect RectSelectionTool::resizeRectByHandle(const Rect& origRect, int handleIdx,
                                           const Point& delta, bool constrainAspect) const noexcept {
  int x1 = origRect.x, y1 = origRect.y;
  int x2 = origRect.x + origRect.width - 1;
  int y2 = origRect.y + origRect.height - 1;

  // ハンドルインデックス: 0=TL, 1=TC, 2=TR, 3=ML, 4=MR, 5=BL, 6=BC, 7=BR
  if (handleIdx == 0 || handleIdx == 1 || handleIdx == 2) x1 += delta.x;  // Top edge
  if (handleIdx == 3 || handleIdx == 4)                    x1 += delta.x;  // Left/Right (ML/MR)
  if (handleIdx == 5 || handleIdx == 6 || handleIdx == 7) x1 += delta.x;  // Bottom edge

  if (handleIdx == 0 || handleIdx == 3 || handleIdx == 5) y1 += delta.y;  // Left edge
  if (handleIdx == 1 || handleIdx == 6)                    y1 += delta.y;  // Top/Bottom (TC/BC)
  if (handleIdx == 2 || handleIdx == 4 || handleIdx == 7) y2 += delta.y;  // Right edge

  // 矯正: x1 < x2, y1 < y2 を保持
  if (x1 > x2) std::swap(x1, x2);
  if (y1 > y2) std::swap(y1, y2);

  // 縦横比制約 (origRect の比率を保持)
  if (constrainAspect && origRect.width > 0 && origRect.height > 0) {
    const float aspect = static_cast<float>(origRect.width) / origRect.height;
    const int w = x2 - x1 + 1;
    const int h = y2 - y1 + 1;
    if (w > h * aspect) {
      x2 = x1 + static_cast<int>(h * aspect) - 1;
    } else {
      y2 = y1 + static_cast<int>(w / aspect) - 1;
    }
  }

  return Rect{x1, y1, std::max(1, x2 - x1 + 1), std::max(1, y2 - y1 + 1)};
}

// ── PointerPress ──────────────────────────────────────────────────────────
ToolResult RectSelectionTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  const SelectionOp op = opFromEvent(event);
  ToolResult result;


  // ── 自動選択 (click → 即実行) ────────────────────────────────────────────
  if (m_mode == Mode::AutoSelect) {
    result.selectionChanged = applyAutoSelect(context, event.point, op);
    if (result.selectionChanged) applyFeather(context);
    result.viewportChanged = true;
    return result;
  }

  // ── Object Select (ストローク収集 → release で確定) ──────────────────────
  if (m_mode == Mode::ObjectSelect) {
    m_selecting  = true;
    m_opAtPress  = op;
    m_strokeHint.clear();
    m_strokeHint.push_back(event.point);
    result.viewportChanged = true;
    return result;
  }

  // ── 多角形ラッソ ──────────────────────────────────────────────────────────
  if (m_mode == Mode::PolygonLasso) {
    result.viewportChanged = true;
    if (event.isDblClick) {
      // ダブルクリック: release で追加されたノードを1つ除去して確定
      if (!m_polyNodes.empty()) {
        m_polyNodes.pop_back();
      }
      if (m_polyNodes.size() >= 3) {
        result.selectionChanged = applyPolygon(context, m_opAtPress);
        if (result.selectionChanged) applyFeather(context);
      }
      cancelPolygon();
    } else {
      // 通常 press: ノード追加は release に延期してドラッグを測定
      if (!m_polyInProgress) {
        m_polyInProgress = true;
        m_polyNodes.clear();
        m_opAtPress = op;
      }
      // 最初のノードに近ければ閉じる（距離 < 8px）
      if (!m_polyNodes.empty()) {
        const FPoint& first = m_polyNodes.front().anchor;
        const float dx = event.fpoint.x - first.x;
        const float dy = event.fpoint.y - first.y;
        if (dx * dx + dy * dy <= 64.f) {
          result.selectionChanged = applyPolygon(context, m_opAtPress);
          if (result.selectionChanged) applyFeather(context);
          cancelPolygon();
          return result;
        }
      }
      m_polyPressing    = true;
      m_polyPressAnchor = event.fpoint;
      m_polyHasDrag     = false;
      m_polyDragHandle  = {0, 0};
    }

    return result;
  }

  // ── 矩形 / フリーハンドラッソ: 選択範囲移動チェック ──────────────────────
  const bool hasSelection = context.document.selection().hasSelection();
  const bool insideSel    = hasSelection && isInsideSelection(context.document.selection(), event.point);

  // ハンドルドラッグの優先度を高くする
  if (hasSelection && op == SelectionOp::New) {
    const int handleIdx = hitTestHandle(context.document.selection(), event.point, 5);
    if (handleIdx >= 0) {
      // ハンドルをドラッグしている
      m_resizingHandle = true;
      m_activeHandle = handleIdx;
      m_savedHandleRect = context.document.selection().boundingRect().value_or(Rect{});
      result.viewportChanged = true;
      return result;
    }
  }

  if (hasSelection && insideSel && op == SelectionOp::New) {
    // 選択マーキーを移動するモード
    m_movingMarquee = true;
    m_moveOrigin = event.point;
    m_moveDx = 0;
    m_moveDy = 0;
    const int w = context.document.selection().width();
    const int h = context.document.selection().height();
    m_savedMask.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        m_savedMask[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)] =
            context.document.selection().maskValue(x, y);
      }
    }
    m_savedMaskW = w;
    m_savedMaskH = h;
    result.viewportChanged = true;

    return result;
  }

  // ── 新規選択ドラッグ開始 ──────────────────────────────────────────────────
  m_selecting = true;
  m_opAtPress = op;
  m_start   = event.point;
  m_current = event.point;
  if (m_mode == Mode::Lasso) {
    m_lassoPoints.clear();
    m_lassoPoints.push_back(event.point);
  }
  result.viewportChanged = true;
  return result;
}

// ── PointerMove ──────────────────────────────────────────────────────────
ToolResult RectSelectionTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  ToolResult result;
  result.viewportChanged = true;

  // ── 選択マーキー移動 ──────────────────────────────────────────────────────
  if (m_movingMarquee) {
    const int newDx = event.point.x - m_moveOrigin.x;
    const int newDy = event.point.y - m_moveOrigin.y;
    if (newDx != m_moveDx || newDy != m_moveDy) {
      m_moveDx = newDx;
      m_moveDy = newDy;
      // savedMask を元に戻してから translate する
      SelectionMask& sel = context.document.selection();
      sel.setPixels(m_savedMask);
      sel.translate(m_moveDx, m_moveDy);
      result.selectionChanged = true;
    }

    return result;
  }

  // ── ハンドルドラッグ（選択範囲リサイズ） ───────────────────────────────────
  if (m_resizingHandle && m_activeHandle >= 0) {
    const Point delta {
      event.point.x - m_savedHandleRect.x - m_savedHandleRect.width / 2,
      event.point.y - m_savedHandleRect.y - m_savedHandleRect.height / 2
    };
    const Rect newRect = resizeRectByHandle(m_savedHandleRect, m_activeHandle, delta, event.shift);
    SelectionMask& sel = context.document.selection();
    sel.clear();
    sel.setRect(newRect);
    result.selectionChanged = true;
    return result;
  }

  // ── 多角形ラッソ: マウス追従 + ドラッグハンドル測定 ──────────────────────
  if (m_mode == Mode::PolygonLasso) {
    m_polyMouse = event.fpoint;
    if (m_polyPressing) {
      const float dx = event.fpoint.x - m_polyPressAnchor.x;
      const float dy = event.fpoint.y - m_polyPressAnchor.y;
      if (dx * dx + dy * dy >= 9.f) {  // 3px 閾値を超えたらドラッグ確定
        m_polyHasDrag    = true;
        m_polyDragHandle = {dx, dy};
      }
      result.viewportChanged = true;
    }
    return result;
  }

  // ── Object Select: ストロークに点追加 ────────────────────────────────────
  if (m_mode == Mode::ObjectSelect && m_selecting) {
    m_strokeHint.push_back(event.point);
    return result;
  }

  if (!m_selecting) return result;

  // Shift制約（正方形）
  if (event.shift && m_mode == Mode::Rectangle) {
    m_current = constrainToSquare(m_start, event.point);
  } else {
    m_current = event.point;
  }

  if (m_mode == Mode::Lasso) {
    const Point last = m_lassoPoints.empty() ? event.point : m_lassoPoints.back();
    if (std::abs(last.x - event.point.x) + std::abs(last.y - event.point.y) >= 1) {
      m_lassoPoints.push_back(event.point);
    }
  }
  return result;
}

// ── PointerRelease ────────────────────────────────────────────────────────
ToolResult RectSelectionTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  ToolResult result;

  // ── ハンドルドラッグ完了 ──────────────────────────────────────────────────
  if (m_resizingHandle) {
    m_resizingHandle = false;
    m_activeHandle = -1;
    result.selectionChanged = true;
    result.viewportChanged = true;
    return result;
  }

  // ── 選択マーキー移動完了 ──────────────────────────────────────────────────
  if (m_movingMarquee) {
    m_movingMarquee = false;
    m_savedMask.clear();
    result.selectionChanged = true;
    result.viewportChanged  = true;
    return result;
  }

  // 多角形ラッソ: release でノードを確定（コーナー or スムース）
  if (m_mode == Mode::PolygonLasso) {
    if (m_polyPressing) {
      m_polyPressing = false;
      PolyLassoNode node;
      node.anchor    = m_polyPressAnchor;
      node.handleOut = m_polyHasDrag ? m_polyDragHandle : FPoint{0, 0};
      node.smooth    = m_polyHasDrag;
      m_polyNodes.push_back(node);
      m_polyMouse  = m_polyPressAnchor;
      m_polyHasDrag = false;
      result.viewportChanged = true;
    }
    return result;
  }

  // ── Object Select: ストローク確定 → Provider 呼び出し ────────────────────
  if (m_mode == Mode::ObjectSelect && m_selecting) {
    m_selecting = false;
    m_strokeHint.push_back(event.point);
    result.selectionChanged = applyObjectSelect(context, m_opAtPress);
    if (result.selectionChanged) applyFeather(context);
    result.viewportChanged = true;
    return result;
  }

  if (!m_selecting) return result;
  m_selecting = false;

  // Shift制約最終適用
  if (event.shift && m_mode == Mode::Rectangle) {
    m_current = constrainToSquare(m_start, event.point);
  } else {
    m_current = event.point;
  }

  bool changed = false;
  if (m_mode == Mode::Lasso) {
    if (!m_lassoPoints.empty() &&
        (m_lassoPoints.back().x != event.point.x || m_lassoPoints.back().y != event.point.y)) {
      m_lassoPoints.push_back(event.point);
    }
    changed = applyLasso(context, m_opAtPress);
  } else {
    // Rectangle
    const Rect rect = normalizeRect(m_start, m_current);
    if (rect.width <= 1 && rect.height <= 1 && m_opAtPress == SelectionOp::New) {
      // 実質ゼロサイズ: 選択解除
      const bool had = context.document.selection().hasSelection();
      context.document.clearSelection();
      changed = had;
    } else {
      // NEW PATH: SelectionEngine → ClassicProvider → SelectionRefiner
      SelectionRequest request;
      request.type = SelectionRequest::Type::Rectangle;
      request.op = m_opAtPress;
      request.rect = rect;
      request.feather = m_featherRadius;
      request.antiAlias = m_antiAlias;
      request.expandPixels = m_expandPixels;
      request.gapClose = m_gapCloseRadius;
      
      if (context.selectionEngine != nullptr) {
        changed = context.selectionEngine->execute(request, context.composited);
      }
    }
  }
  if (changed && !m_featherRadius) {
    // フェザーは SelectionEngine で処理済みなので、ここではスキップ
    // applyFeather(context);
  }
  result.selectionChanged = changed;
  result.viewportChanged  = true;
  return result;
}

ToolResult RectSelectionTool::onCancel(ToolContext& context) {
  ToolResult result;
  if (m_movingMarquee) {
    // 移動キャンセル: 元のマスクを復元
    context.document.selection().setPixels(m_savedMask);
    m_movingMarquee = false;
    m_savedMask.clear();
    result.selectionChanged = true;
    result.viewportChanged  = true;
    return result;
  }
  if (m_polyInProgress) {
    cancelPolygon();
    result.viewportChanged = true;
    return result;
  }
  if (!m_selecting) return result;
  m_selecting = false;
  m_strokeHint.clear();
  result.viewportChanged = true;
  return result;
}

ToolResult RectSelectionTool::onWheel(ToolContext&, int, const ToolPointerEvent&) {
  return {};
}

// ── Overlay ───────────────────────────────────────────────────────────────
ToolOverlayState RectSelectionTool::overlay() const {
  ToolOverlayState state;

  // 多角形ラッソ進行中
  if (m_mode == Mode::PolygonLasso && m_polyInProgress) {
    state.hasPolyLasso = true;
    state.polyLassoNodes.reserve(m_polyNodes.size());
    for (const auto& n : m_polyNodes) {
      state.polyLassoNodes.push_back({n.anchor, n.smooth ? n.handleOut : FPoint{0, 0}});
    }
    state.polyLassoMouse       = m_polyMouse;
    state.polyLassoIsDragging  = m_polyPressing && m_polyHasDrag;
    state.polyLassoDragAnchor  = m_polyPressAnchor;
    state.polyLassoDragHandle  = m_polyDragHandle;
    state.cursorHint = OverlayCursorHint::Cross;
    return state;
  }

  // Object Select: ストローク中は lasso 風に表示
  if (m_mode == Mode::ObjectSelect && m_selecting && m_strokeHint.size() >= 2) {
    state.hasPolygon    = true;
    state.polygonClosed = false;
    state.polygonPoints = m_strokeHint;
    state.cursorHint    = OverlayCursorHint::Cross;
    return state;
  }

  if (!m_selecting) return state;

  if (m_mode == Mode::Lasso) {
    if (!m_lassoPoints.empty()) {
      state.hasPolygon    = true;
      state.polygonClosed = false;
      state.polygonPoints = m_lassoPoints;
      if (state.polygonPoints.back().x != m_current.x ||
          state.polygonPoints.back().y != m_current.y) {
        state.polygonPoints.push_back(m_current);
      }
    }
  } else if (m_mode == Mode::Rectangle) {
    state.hasRect = true;
    state.rect = normalizeRect(m_start, m_current);
  }

  if (m_movingMarquee) {
    state.cursorHint = OverlayCursorHint::Move;
  }
  return state;
}

// ── フェザー適用 ──────────────────────────────────────────────────────────
void RectSelectionTool::applyFeather(ToolContext& context) const {
  if (m_featherRadius > 0) {
    context.document.selection().feather(m_featherRadius);
  }
}

// ── 自動選択 / オブジェクト選択 ───────────────────────────────────────────
bool RectSelectionTool::applyAutoSelect(ToolContext& context, const Point& seed, SelectionOp op) {
  const int width  = context.composited.width();
  const int height = context.composited.height();
  if (width <= 0 || height <= 0) return false;
  if (seed.x < 0 || seed.y < 0 || seed.x >= width || seed.y >= height) return false;

  // NEW PATH: SelectionEngine → ClassicProvider → SelectionRefiner
  SelectionRequest request;
  request.type = (m_mode == Mode::AutoSelect)
      ? SelectionRequest::Type::MagicWand
      : SelectionRequest::Type::Object;

  request.op = op;
  request.seed = seed;
  request.tolerance = m_autoSelectThreshold;
  request.contiguous = (m_mode == Mode::AutoSelect) ? m_autoSelectContiguous : false;
  request.referenceMode = m_autoSelectReferAllLayers
      ? SelectionRequest::ReferenceMode::AllLayers
      : SelectionRequest::ReferenceMode::CurrentLayer;
  request.feather = m_featherRadius;
  request.antiAlias = m_antiAlias;
  request.expandPixels = m_expandPixels;
  request.gapClose = m_gapCloseRadius;
  request.edgeAware = m_edgeAware;

  if (context.selectionEngine != nullptr) {
    return context.selectionEngine->execute(request, context.composited);
  }
  return false;
}


bool RectSelectionTool::applyObjectSelect(ToolContext& context, SelectionOp op) {
  if (m_strokeHint.empty()) return false;
  const int width  = context.composited.width();
  const int height = context.composited.height();
  if (width <= 0 || height <= 0) return false;

  SelectionRequest request;
  request.type        = SelectionRequest::Type::Object;
  request.op          = op;
  request.seed        = m_strokeHint.front();
  request.strokeHint  = m_strokeHint;
  request.tolerance   = m_autoSelectThreshold;
  request.contiguous  = false;
  request.referenceMode = m_autoSelectReferAllLayers
      ? SelectionRequest::ReferenceMode::AllLayers
      : SelectionRequest::ReferenceMode::CurrentLayer;
  request.feather      = m_featherRadius;
  request.antiAlias    = m_antiAlias;
  request.expandPixels = m_expandPixels;
  request.gapClose     = m_gapCloseRadius;
  request.edgeAware    = m_edgeAware;
  request.maxCandidates = 3;

  if (context.selectionEngine != nullptr) {
    return context.selectionEngine->execute(request, context.composited);
  }
  return false;
}

bool RectSelectionTool::applyLasso(ToolContext& context, SelectionOp op) {
  const int width  = context.document.canvasSize().width;
  const int height = context.document.canvasSize().height;
  if (width <= 0 || height <= 0 || m_lassoPoints.size() < 3) return false;

  // NEW PATH: SelectionEngine → ClassicProvider → SelectionRefiner
  SelectionRequest request;
  request.type = SelectionRequest::Type::FreeLasso;
  request.op = op;
  request.points = m_lassoPoints;
  request.feather = m_featherRadius;
  request.antiAlias = m_antiAlias;
  request.expandPixels = m_expandPixels;
  request.gapClose = m_gapCloseRadius;
  
  if (context.selectionEngine != nullptr) {
    return context.selectionEngine->execute(request, context.composited);
  }
  return false;
}

bool RectSelectionTool::applyPolygon(ToolContext& context, SelectionOp op) {
  const int width  = context.document.canvasSize().width;
  const int height = context.document.canvasSize().height;
  if (width <= 0 || height <= 0 || m_polyNodes.size() < 3) return false;

  const auto flatPoints = flattenPolyNodes(m_polyNodes);
  if (flatPoints.size() < 3) return false;

  // NEW PATH: SelectionEngine → ClassicProvider → SelectionRefiner
  SelectionRequest request;
  request.type = SelectionRequest::Type::PolygonLasso;
  request.op = op;
  request.points = flatPoints;
  request.feather = m_featherRadius;
  request.antiAlias = m_antiAlias;
  request.expandPixels = m_expandPixels;
  request.gapClose = m_gapCloseRadius;
  
  if (context.selectionEngine != nullptr) {
    return context.selectionEngine->execute(request, context.composited);
  }
  return false;
}


} // namespace core
