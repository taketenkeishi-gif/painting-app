#include "core/selection/SelectionRefiner.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "core/selection/SelectionMask.h"

namespace core {

namespace {

// ── Connected-component BFS on the raw pixel buffer ─────────────────────────
// Returns a vector where each element is the component id (0 = background).
// Pixels with value >= threshold belong to the foreground.
struct ComponentInfo {
  std::vector<int>  labels;   // per-pixel component label (0 = unvisited fg)
  std::vector<int>  sizes;    // sizes[label] = pixel count (index 0 unused)
  int               count{0}; // number of components
};

ComponentInfo labelComponents(const std::vector<std::uint8_t>& pixels,
                               int w, int h, std::uint8_t threshold) {
  ComponentInfo info;
  info.labels.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), -1);
  info.sizes.push_back(0); // index-0 placeholder

  auto idx = [w](int x, int y) { return y * w + x; };
  auto fg  = [&](int x, int y) {
    return x >= 0 && y >= 0 && x < w && y < h &&
           pixels[static_cast<std::size_t>(idx(x,y))] >= threshold;
  };

  int nextLabel = 1;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      if (!fg(x, y) || info.labels[static_cast<std::size_t>(idx(x,y))] >= 0) continue;
      // BFS
      const int label = nextLabel++;
      info.sizes.push_back(0);
      std::vector<std::pair<int,int>> queue;
      queue.reserve(256);
      queue.push_back({x, y});
      info.labels[static_cast<std::size_t>(idx(x,y))] = label;
      for (std::size_t qi = 0; qi < queue.size(); ++qi) {
        auto [cx, cy] = queue[qi];
        ++info.sizes[static_cast<std::size_t>(label)];
        const int dx[4] = {1,-1,0,0}, dy[4] = {0,0,1,-1};
        for (int d = 0; d < 4; ++d) {
          const int nx = cx + dx[d], ny = cy + dy[d];
          if (!fg(nx, ny)) continue;
          const std::size_t ni = static_cast<std::size_t>(idx(nx, ny));
          if (info.labels[ni] >= 0) continue;
          info.labels[ni] = label;
          queue.push_back({nx, ny});
        }
      }
    }
  }
  info.count = nextLabel - 1;
  return info;
}

// ── Remove selected islands smaller than minArea pixels ───────────────────
void removeIslands(SelectionMask& mask, int minArea) {
  if (minArea <= 0 || !mask.hasSelection()) return;
  const int w = mask.width(), h = mask.height();
  if (w <= 0 || h <= 0) return;

  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(w * h));
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
      pixels[static_cast<std::size_t>(y*w+x)] = mask.maskValue(x, y);

  const auto info = labelComponents(pixels, w, h, 128U);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const std::size_t i  = static_cast<std::size_t>(y*w+x);
      const int         lb = info.labels[i];
      if (lb > 0 && info.sizes[static_cast<std::size_t>(lb)] < minArea) {
        pixels[i] = 0U;
      }
    }
  }
  mask.setPixels(pixels);
}

// ── Fill enclosed unselected holes smaller than maxArea pixels ────────────
void fillHoles(SelectionMask& mask, int maxArea) {
  if (maxArea <= 0 || !mask.hasSelection()) return;
  const int w = mask.width(), h = mask.height();
  if (w <= 0 || h <= 0) return;

  // Work on the inverted mask
  std::vector<std::uint8_t> inv(static_cast<std::size_t>(w * h));
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
      inv[static_cast<std::size_t>(y*w+x)] =
          255U - mask.maskValue(x, y);

  const auto info = labelComponents(inv, w, h, 128U);
  // Components that touch the canvas border are not "holes"
  std::vector<bool> isBorder(static_cast<std::size_t>(info.count + 1), false);
  for (int x = 0; x < w; ++x) {
    const int tl = info.labels[static_cast<std::size_t>(x)];
    const int bl = info.labels[static_cast<std::size_t>((h-1)*w+x)];
    if (tl > 0) isBorder[static_cast<std::size_t>(tl)] = true;
    if (bl > 0) isBorder[static_cast<std::size_t>(bl)] = true;
  }
  for (int y = 0; y < h; ++y) {
    const int ll = info.labels[static_cast<std::size_t>(y*w)];
    const int rl = info.labels[static_cast<std::size_t>(y*w+w-1)];
    if (ll > 0) isBorder[static_cast<std::size_t>(ll)] = true;
    if (rl > 0) isBorder[static_cast<std::size_t>(rl)] = true;
  }

  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(w * h));
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const std::size_t i  = static_cast<std::size_t>(y*w+x);
      const int         lb = info.labels[i];
      const bool isHole = lb > 0 &&
                          !isBorder[static_cast<std::size_t>(lb)] &&
                          info.sizes[static_cast<std::size_t>(lb)] <= maxArea;
      pixels[i] = isHole ? 255U : (255U - inv[i]);
    }
  }
  mask.setPixels(pixels);
}

// ── 1px Gaussian AA on boundary pixels only ──────────────────────────────
void antiAliasEdge(SelectionMask& mask) {
  const int w = mask.width(), h = mask.height();
  if (w <= 0 || h <= 0) return;

  const std::size_t total = static_cast<std::size_t>(w * h);
  std::vector<float> src(total);
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x)
      src[static_cast<std::size_t>(y*w+x)] = mask.maskValue(x, y) / 255.0f;

  auto get = [&](int x, int y) -> float {
    if (x < 0 || y < 0 || x >= w || y >= h) return 0.0f;
    return src[static_cast<std::size_t>(y*w+x)];
  };

  std::vector<std::uint8_t> out(total);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const float v = get(x, y);
      // Is this a boundary pixel? (has neighbor with significantly different value)
      bool boundary = false;
      for (int dy = -1; dy <= 1 && !boundary; ++dy)
        for (int dx = -1; dx <= 1 && !boundary; ++dx)
          if ((dx || dy) && std::abs(get(x+dx, y+dy) - v) > 0.35f)
            boundary = true;

      if (boundary) {
        // 3×3 box blur
        float sum = 0.0f;
        for (int dy = -1; dy <= 1; ++dy)
          for (int dx = -1; dx <= 1; ++dx)
            sum += get(x+dx, y+dy);
        out[static_cast<std::size_t>(y*w+x)] =
            static_cast<std::uint8_t>(std::clamp(sum / 9.0f * 255.0f + 0.5f, 0.0f, 255.0f));
      } else {
        out[static_cast<std::size_t>(y*w+x)] =
            static_cast<std::uint8_t>(std::clamp(v * 255.0f + 0.5f, 0.0f, 255.0f));
      }
    }
  }
  mask.setPixels(out);
}

// ── Basic edge-snap: soften low-confidence boundary pixels ────────────────
void applyEdgeSnap(SelectionMask& mask,
                   const std::vector<std::uint8_t>& edgeMap) {
  const int w = mask.width(), h = mask.height();
  if (w <= 0 || h <= 0 || edgeMap.empty()) return;

  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(w * h));
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const std::size_t i = static_cast<std::size_t>(y*w+x);
      const std::uint8_t mv = mask.maskValue(x, y);
      if (mv == 0U || mv == 255U) { pixels[i] = mv; continue; }
      // Boundary pixel: amplify toward edges, reduce away from edges
      const float edgeStrength = edgeMap[i] / 255.0f;
      const float snapped = mv / 255.0f * (0.6f + 0.4f * edgeStrength);
      pixels[i] = static_cast<std::uint8_t>(std::clamp(snapped * 255.0f, 0.0f, 255.0f));
    }
  }
  mask.setPixels(pixels);
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// SelectionRefiner::apply
// ─────────────────────────────────────────────────────────────────────────────
void SelectionRefiner::apply(SelectionResult& result,
                              const RefinementOptions& opts) {
  SelectionMask& mask = result.mask;

  // 1. ギャップ補完 (形態学的クローズ: expand → contract)
  if (opts.gapCloseRadius > 0) {
    mask.expand(opts.gapCloseRadius);
    mask.contract(opts.gapCloseRadius);
  }

  // 2. 孤立島除去
  if (opts.removeIslandsMinArea > 0) {
    removeIslands(mask, opts.removeIslandsMinArea);
  }

  // 3. 穴埋め
  if (opts.fillHolesMaxArea > 0) {
    fillHoles(mask, opts.fillHolesMaxArea);
  }

  // 4. 拡張 → 収縮 → フェザー → スムージング
  if (opts.expandRadius > 0)   mask.expand(opts.expandRadius);
  if (opts.contractRadius > 0) mask.contract(opts.contractRadius);
  if (opts.featherRadius > 0)  mask.feather(opts.featherRadius);
  if (opts.smoothRadius > 0)   mask.smooth(opts.smoothRadius);

  // 5. エッジスナップ (edgeMap が必要)
  if (opts.edgeSnap && result.hasEdgeMap()) {
    applyEdgeSnap(mask, result.edgeMap);
  }

  // 6. 境界アンチエイリアス
  if (opts.antiAliasEdge) {
    antiAliasEdge(mask);
  }
}

} // namespace core
