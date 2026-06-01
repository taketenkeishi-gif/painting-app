#include "core/selection/SelectionEngine.h"

#include <algorithm>
#include <queue>

#include "core/document/Document.h"

namespace core {

// ── PrivateアクセサHelper ──────────────────────────────────────────────────
const SelectionMask* SelectionEngine::mask() const noexcept {
  if (m_doc == nullptr) return nullptr;
  return &m_doc->selection();
}

SelectionMask* SelectionEngine::mutableMask() noexcept {
  if (m_doc == nullptr) return nullptr;
  return &m_doc->selection();
}

// ── 選択適用 ──────────────────────────────────────────────────────────────
bool SelectionEngine::applyRect(SelectionOp op, const Rect& rect) {
  auto* m = mutableMask();
  if (m == nullptr) return false;
  return m->applyRect(op, rect);
}

bool SelectionEngine::applyPixels(
    SelectionOp op,
    const std::vector<std::uint8_t>& pixels,
    SelectionSource /*source*/) {
  auto* m = mutableMask();
  if (m == nullptr) return false;
  return m->applyPixels(op, pixels);
}

bool SelectionEngine::floodFill(
    const PixelBuffer& reference, int sx, int sy,
    int threshold, SelectionOp op) {
  if (m_doc == nullptr) return false;
  const int w = reference.width();
  const int h = reference.height();
  if (sx < 0 || sy < 0 || sx >= w || sy >= h) return false;

  const Color seed = reference.pixel(sx, sy);
  auto colorDiff = [&](const Color& a, const Color& b) {
    const int dr = static_cast<int>(a.r) - static_cast<int>(b.r);
    const int dg = static_cast<int>(a.g) - static_cast<int>(b.g);
    const int db = static_cast<int>(a.b) - static_cast<int>(b.b);
    return dr * dr + dg * dg + db * db;
  };
  const int thresh2 = threshold * threshold * 3;

  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(w * h), 0U);
  std::vector<bool> visited(static_cast<std::size_t>(w * h), false);
  std::queue<int> queue;
  const int startIdx = sy * w + sx;
  queue.push(startIdx);
  visited[static_cast<std::size_t>(startIdx)] = true;

  while (!queue.empty()) {
    const int idx = queue.front();
    queue.pop();
    const int cx = idx % w;
    const int cy = idx / w;
    pixels[static_cast<std::size_t>(idx)] = 255U;
    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, 1, -1};
    for (int d = 0; d < 4; ++d) {
      const int nx = cx + dx[d];
      const int ny = cy + dy[d];
      if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
      const int nIdx = ny * w + nx;
      if (visited[static_cast<std::size_t>(nIdx)]) continue;
      visited[static_cast<std::size_t>(nIdx)] = true;
      if (colorDiff(reference.pixel(nx, ny), seed) <= thresh2) {
        queue.push(nIdx);
      }
    }
  }
  return applyPixels(op, pixels, SelectionSource::ColorBased);
}

// ── 後処理 ────────────────────────────────────────────────────────────────
bool SelectionEngine::applyRefinement(const RefinementOptions& opts) {
  bool changed = false;
  if (opts.expandPixels   > 0) changed |= expand(opts.expandPixels);
  if (opts.contractPixels > 0) changed |= contract(opts.contractPixels);
  if (opts.smoothRadius   > 0) changed |= smooth(opts.smoothRadius);
  if (opts.featherRadius  > 0) changed |= feather(opts.featherRadius);
  return changed;
}

bool SelectionEngine::expand(int radius) {
  auto* m = mutableMask();
  return m != nullptr && m->expand(radius);
}

bool SelectionEngine::contract(int radius) {
  auto* m = mutableMask();
  return m != nullptr && m->contract(radius);
}

bool SelectionEngine::smooth(int radius) {
  auto* m = mutableMask();
  return m != nullptr && m->smooth(radius);
}

bool SelectionEngine::feather(int radius) {
  auto* m = mutableMask();
  return m != nullptr && m->feather(radius);
}

// ── 全体操作 ──────────────────────────────────────────────────────────────
bool SelectionEngine::selectAll() {
  if (m_doc == nullptr) return false;
  const Size sz = m_doc->canvasSize();
  return applyRect(SelectionOp::New, Rect {0, 0, sz.width, sz.height});
}

bool SelectionEngine::deselect() {
  auto* m = mutableMask();
  if (m == nullptr) return false;
  m->clear();
  return true;
}

bool SelectionEngine::invert() {
  auto* m = mutableMask();
  return m != nullptr && m->invert();
}

} // namespace core
