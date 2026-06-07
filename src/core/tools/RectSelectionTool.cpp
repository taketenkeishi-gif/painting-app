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
  m_polyInProgress = false;
  m_polyPoints.clear();
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
      // ダブルクリック: 最後に追加されたクリック頂点を除去し確定
      if (!m_polyPoints.empty()) {
        m_polyPoints.pop_back();  // 通常 press で追加された分を除去
      }
      if (m_polyPoints.size() >= 3) {
        result.selectionChanged = applyPolygon(context, op);
        if (result.selectionChanged) applyFeather(context);

      }
      cancelPolygon();
    } else {
      // 通常クリック: 頂点追加
      if (!m_polyInProgress) {
        m_polyInProgress = true;
        m_polyPoints.clear();
        m_opAtPress = op;
      }
      // 最初の頂点に近ければ閉じる（距離 < 8px）
      if (m_polyPoints.size() >= 3) {
        const Point& first = m_polyPoints.front();
        const int dx = event.point.x - first.x;
        const int dy = event.point.y - first.y;
        if (dx * dx + dy * dy <= 64) {
          result.selectionChanged = applyPolygon(context, m_opAtPress);
          if (result.selectionChanged) applyFeather(context);
  
          cancelPolygon();
          return result;
        }
      }
      m_polyPoints.push_back(event.point);
      m_polyMouse = event.point;
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

  // ── 多角形ラッソ: マウス追従 ──────────────────────────────────────────────
  if (m_mode == Mode::PolygonLasso) {
    m_polyMouse = event.point;
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

  // 多角形ラッソ中は release では何もしない（click/dblclick で処理）
  if (m_mode == Mode::PolygonLasso) {
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
    state.polyLassoVertices = m_polyPoints;
    state.polyLassoMouse = m_polyMouse;
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
  if (width <= 0 || height <= 0 || m_polyPoints.size() < 3) return false;

  // NEW PATH: SelectionEngine → ClassicProvider → SelectionRefiner
  SelectionRequest request;
  request.type = SelectionRequest::Type::PolygonLasso;
  request.op = op;
  request.points = m_polyPoints;
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
