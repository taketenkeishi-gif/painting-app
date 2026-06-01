#include "core/tools/RectSelectionTool.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace core {

// ── 修飾キーからオペレーション決定 ───────────────────────────────────────
SelectionOp RectSelectionTool::opFromEvent(const ToolPointerEvent& e) noexcept {
  if (e.shift && e.alt) return SelectionOp::Intersect;
  if (e.shift)          return SelectionOp::Add;
  if (e.alt)            return SelectionOp::Subtract;
  return SelectionOp::New;
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

// ── PointerPress ──────────────────────────────────────────────────────────
ToolResult RectSelectionTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  const SelectionOp op = opFromEvent(event);
  ToolResult result;

  // ── 自動選択 / オブジェクト選択 ──────────────────────────────────────────
  if (m_mode == Mode::AutoSelect || m_mode == Mode::ObjectSelect) {
    result.selectionChanged = applyAutoSelect(context, event.point, op);
    if (result.selectionChanged) applyFeather(context);
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
        m_committedLassoPoints = m_polyPoints;
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
          m_committedLassoPoints = m_polyPoints;
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

  // ── 多角形ラッソ: マウス追従 ──────────────────────────────────────────────
  if (m_mode == Mode::PolygonLasso) {
    m_polyMouse = event.point;
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
    if (changed) m_committedLassoPoints = m_lassoPoints;
  } else {
    // Rectangle
    const Rect rect = normalizeRect(m_start, m_current);
    if (rect.width <= 1 && rect.height <= 1 && m_opAtPress == SelectionOp::New) {
      // 実質ゼロサイズ: 選択解除
      const bool had = context.document.selection().hasSelection();
      context.document.clearSelection();
      changed = had;
    } else {
      changed = context.document.selection().applyRect(m_opAtPress, rect);
    }
  }
  if (changed) applyFeather(context);
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

  // 確定済みラッソ/多角形のアウトライン表示
  if (!m_selecting && !m_committedLassoPoints.empty()) {
    state.hasPolygon    = true;
    state.polygonClosed = true;
    state.polygonPoints = m_committedLassoPoints;
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

  const bool contiguous = (m_mode == Mode::AutoSelect) ? m_autoSelectContiguous : false;
  const bool allLayers  = (m_mode == Mode::ObjectSelect) ? true : m_autoSelectReferAllLayers;

  const Layer* active = context.document.activeLayer();
  const PixelBuffer* source = &context.composited;
  if (!allLayers && active != nullptr && active->kind() == LayerKind::Raster) {
    source = &active->buffer();
  }

  const Color seedColor = source->pixel(seed.x, seed.y);
  const std::size_t total = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  std::vector<std::uint8_t> mask(total, 0U);

  auto idx = [width](int x, int y) -> std::size_t {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
  };
  auto matches = [&](int x, int y) -> bool {
    return colorDistance(source->pixel(x, y), seedColor) <= m_autoSelectThreshold;
  };

  if (contiguous) {
    struct Span { int y, x0, x1; };
    std::vector<Span> stack;
    stack.reserve(512);

    auto pushSpan = [&](int y, int x0, int x1) {
      if (y < 0 || y >= height) return;
      stack.push_back({y, x0, x1});
    };

    if (matches(seed.x, seed.y)) {
      int l = seed.x, r = seed.x;
      while (l > 0 && matches(l - 1, seed.y)) --l;
      while (r < width - 1 && matches(r + 1, seed.y)) ++r;
      for (int x = l; x <= r; ++x) mask[idx(x, seed.y)] = 255U;
      pushSpan(seed.y - 1, l, r);
      pushSpan(seed.y + 1, l, r);
    }

    while (!stack.empty()) {
      const auto [y, sx0, sx1] = stack.back();
      stack.pop_back();

      int x = sx0;
      while (x <= sx1) {
        if (mask[idx(x, y)] != 0U || !matches(x, y)) { ++x; continue; }
        int l = x;
        while (l > 0 && mask[idx(l-1,y)] == 0U && matches(l - 1, y)) --l;
        int r = x;
        while (r < width - 1 && mask[idx(r+1,y)] == 0U && matches(r + 1, y)) ++r;
        for (int px = l; px <= r; ++px) mask[idx(px, y)] = 255U;
        pushSpan(y - 1, l, r);
        pushSpan(y + 1, l, r);
        x = r + 1;
      }
    }
  } else {
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        if (matches(x, y)) mask[idx(x, y)] = 255U;
      }
    }
  }

  return context.document.selection().applyPixels(op, mask);
}

// ── スキャンライン塗り (0/1 マスク) ──────────────────────────────────────
std::vector<std::uint8_t> RectSelectionTool::scanFillPolygon(
    const std::vector<Point>& poly, int width, int height) {
  std::vector<std::uint8_t> mask(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0U);
  if (poly.size() < 3) return mask;

  const int n = static_cast<int>(poly.size());
  int yMin = height, yMax = -1;
  for (const auto& p : poly) {
    yMin = std::min(yMin, std::clamp(p.y, 0, height - 1));
    yMax = std::max(yMax, std::clamp(p.y, 0, height - 1));
  }

  for (int y = yMin; y <= yMax; ++y) {
    std::vector<int> xs;
    for (int i = 0, j = n - 1; i < n; j = i++) {
      const int ay = poly[i].y, by = poly[j].y;
      if ((ay <= y && by > y) || (by <= y && ay > y)) {
        const int ax = poly[i].x, bx = poly[j].x;
        xs.push_back(ax + (y - ay) * (bx - ax) / (by - ay));
      }
    }
    std::sort(xs.begin(), xs.end());
    for (std::size_t k = 0; k + 1 < xs.size(); k += 2) {
      const int x0 = std::clamp(xs[k],     0, width - 1);
      const int x1 = std::clamp(xs[k + 1], 0, width - 1);
      for (int x = x0; x <= x1; ++x) {
        mask[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)] = 255U;
      }
    }
  }
  return mask;
}

// ── アンチエイリアス付きポリゴン塗り ─────────────────────────────────────
// スキャンラインで 0/255 のマスクを作り、1px ガウスで AA を近似する
std::vector<std::uint8_t> RectSelectionTool::antiAliasedFillPolygon(
    const std::vector<Point>& poly, int width, int height) {
  auto mask = scanFillPolygon(poly, width, height);
  if (width <= 0 || height <= 0) return mask;

  // 1px 分離型ボックスフィルタ → AA
  std::vector<float> buf(mask.size());
  for (std::size_t i = 0; i < mask.size(); ++i) {
    buf[i] = static_cast<float>(mask[i]) / 255.0f;
  }
  std::vector<float> tmp(mask.size());
  auto widx = [width](int x, int y) { return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x); };
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float sum = 0.0f; int cnt = 0;
      for (int dx = -1; dx <= 1; ++dx) {
        const int nx = x + dx;
        if (nx >= 0 && nx < width) { sum += buf[widx(nx, y)]; ++cnt; }
      }
      tmp[widx(x, y)] = cnt > 0 ? sum / static_cast<float>(cnt) : 0.0f;
    }
  }
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float sum = 0.0f; int cnt = 0;
      for (int dy = -1; dy <= 1; ++dy) {
        const int ny = y + dy;
        if (ny >= 0 && ny < height) { sum += tmp[widx(x, ny)]; ++cnt; }
      }
      buf[widx(x, y)] = cnt > 0 ? sum / static_cast<float>(cnt) : 0.0f;
    }
  }
  for (std::size_t i = 0; i < mask.size(); ++i) {
    mask[i] = static_cast<std::uint8_t>(std::clamp(static_cast<int>(buf[i] * 255.0f + 0.5f), 0, 255));
  }
  return mask;
}

bool RectSelectionTool::applyLasso(ToolContext& context, SelectionOp op) {
  const int width  = context.document.canvasSize().width;
  const int height = context.document.canvasSize().height;
  if (width <= 0 || height <= 0 || m_lassoPoints.size() < 3) return false;

  auto mask = m_antiAlias
      ? antiAliasedFillPolygon(m_lassoPoints, width, height)
      : scanFillPolygon(m_lassoPoints, width, height);
  return context.document.selection().applyPixels(op, mask);
}

bool RectSelectionTool::applyPolygon(ToolContext& context, SelectionOp op) {
  const int width  = context.document.canvasSize().width;
  const int height = context.document.canvasSize().height;
  if (width <= 0 || height <= 0 || m_polyPoints.size() < 3) return false;

  auto mask = m_antiAlias
      ? antiAliasedFillPolygon(m_polyPoints, width, height)
      : scanFillPolygon(m_polyPoints, width, height);
  return context.document.selection().applyPixels(op, mask);
}

int RectSelectionTool::colorDistance(const Color& a, const Color& b) noexcept {
  const int dr = std::abs(static_cast<int>(a.r) - static_cast<int>(b.r));
  const int dg = std::abs(static_cast<int>(a.g) - static_cast<int>(b.g));
  const int db = std::abs(static_cast<int>(a.b) - static_cast<int>(b.b));
  return std::max({dr, dg, db});
}

} // namespace core
