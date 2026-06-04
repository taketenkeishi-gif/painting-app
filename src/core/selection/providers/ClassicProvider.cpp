#include "core/selection/providers/ClassicProvider.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "core/buffer/PixelBuffer.h"
#include "core/color/Color.h"
#include "core/common/Point.h"
#include "core/common/Rect.h"
#include "core/selection/SelectionMask.h"
#include "core/selection/SelectionRequest.h"
#include "core/selection/SelectionResult.h"

namespace core {

// ─────────────────────────────────────────────────────────────────────────────
// canHandle
// ─────────────────────────────────────────────────────────────────────────────
bool ClassicProvider::canHandle(SelectionRequest::Type type) const noexcept {
  switch (type) {
    case SelectionRequest::Type::Rectangle:
    case SelectionRequest::Type::FreeLasso:
    case SelectionRequest::Type::PolygonLasso:
    case SelectionRequest::Type::MagicWand:
    case SelectionRequest::Type::Object:
      return true;
    default:
      return false;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// execute
// ─────────────────────────────────────────────────────────────────────────────
SelectionResult ClassicProvider::execute(const SelectionRequest& request,
                                          const PixelBuffer&      reference,
                                          int canvasW,
                                          int canvasH) const {
  SelectionResult result;
  result.providerName = "Classic";

  switch (request.type) {

    case SelectionRequest::Type::Rectangle:
      result.mask = rasterizeRect(request.rect, canvasW, canvasH);
      break;

    case SelectionRequest::Type::FreeLasso:
    case SelectionRequest::Type::PolygonLasso:
      result.mask = rasterizePolygon(request.points, canvasW, canvasH,
                                     request.antiAlias);
      break;

    case SelectionRequest::Type::MagicWand:
    case SelectionRequest::Type::Object: {
      const bool contiguous = (request.type == SelectionRequest::Type::MagicWand)
                                  ? request.contiguous
                                  : false;
      result = floodFillSelect(reference, request.seed,
                               request.tolerance, contiguous,
                               request.antiAlias, request.edgeAware,
                               canvasW, canvasH);
      break;
    }

    default:
      // 非対応タイプ: 空マスクを返す
      result.mask = SelectionMask(canvasW, canvasH);
      break;
  }

  return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// rasterizeRect
// ─────────────────────────────────────────────────────────────────────────────
SelectionMask ClassicProvider::rasterizeRect(const Rect& rect, int w, int h) {
  SelectionMask mask(w, h);
  if (w <= 0 || h <= 0) return mask;

  const std::size_t total = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
  std::vector<std::uint8_t> pixels(total, 0U);

  const int x0 = std::clamp(rect.x,                   0, w - 1);
  const int y0 = std::clamp(rect.y,                   0, h - 1);
  const int x1 = std::clamp(rect.x + rect.width  - 1, 0, w - 1);
  const int y1 = std::clamp(rect.y + rect.height - 1, 0, h - 1);

  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
             static_cast<std::size_t>(x)] = 255U;
    }
  }

  mask.setPixels(pixels);
  return mask;
}

// ─────────────────────────────────────────────────────────────────────────────
// rasterizePolygon
// ─────────────────────────────────────────────────────────────────────────────
SelectionMask ClassicProvider::rasterizePolygon(const std::vector<Point>& pts,
                                                 int w, int h,
                                                 bool antiAlias) {
  SelectionMask mask(w, h);
  if (pts.size() < 3 || w <= 0 || h <= 0) return mask;

  auto pixels = antiAlias ? antiAliasedFillPolygon(pts, w, h)
                           : scanFillPolygon(pts, w, h);
  mask.setPixels(pixels);
  return mask;
}

// ─────────────────────────────────────────────────────────────────────────────
// scanFillPolygon  — スキャンライン塗りつぶし (0/255)
// ─────────────────────────────────────────────────────────────────────────────
std::vector<std::uint8_t> ClassicProvider::scanFillPolygon(
    const std::vector<Point>& poly, int width, int height) {
  std::vector<std::uint8_t> mask(
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0U);
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
        mask[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
             static_cast<std::size_t>(x)] = 255U;
      }
    }
  }
  return mask;
}

// ─────────────────────────────────────────────────────────────────────────────
// antiAliasedFillPolygon  — スキャンライン + 1px ボックスフィルタ AA
// ─────────────────────────────────────────────────────────────────────────────
std::vector<std::uint8_t> ClassicProvider::antiAliasedFillPolygon(
    const std::vector<Point>& poly, int width, int height) {
  auto mask = scanFillPolygon(poly, width, height);
  if (width <= 0 || height <= 0) return mask;

  std::vector<float> buf(mask.size());
  for (std::size_t i = 0; i < mask.size(); ++i) {
    buf[i] = static_cast<float>(mask[i]) / 255.0f;
  }

  auto widx = [width](int x, int y) {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
           static_cast<std::size_t>(x);
  };

  std::vector<float> tmp(mask.size());

  // 横方向
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

  // 縦方向
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
    mask[i] = static_cast<std::uint8_t>(
        std::clamp(static_cast<int>(buf[i] * 255.0f + 0.5f), 0, 255));
  }
  return mask;
}

// ─────────────────────────────────────────────────────────────────────────────
// Lab color helpers  (anonymous namespace — internal only)
// ─────────────────────────────────────────────────────────────────────────────
namespace {

struct Lab { float L, a, b; };

// 256-entry LUT: sRGB byte → linear light (computed once)
const float* linearLut() noexcept {
  static float lut[256];
  static bool  ready = false;
  if (!ready) {
    for (int i = 0; i < 256; ++i) {
      const float c = static_cast<float>(i) / 255.0f;
      lut[i] = (c <= 0.04045f) ? c / 12.92f
                                : std::pow((c + 0.055f) / 1.055f, 2.4f);
    }
    ready = true;
  }
  return lut;
}

Lab rgbToLab(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept {
  const float* L = linearLut();
  const float  lr = L[r], lg = L[g], lb = L[b];

  // sRGB linear → XYZ D65
  const float X = 0.4124564f * lr + 0.3575761f * lg + 0.1804375f * lb;
  const float Y = 0.2126729f * lr + 0.7151522f * lg + 0.0721750f * lb;
  const float Z = 0.0193339f * lr + 0.1191920f * lg + 0.9503041f * lb;

  // XYZ → Lab (D65 white: Xn=0.95047, Yn=1.0, Zn=1.08883)
  auto f = [](float t) -> float {
    return t > 0.008856f ? std::cbrt(t) : 7.787037f * t + 16.0f / 116.0f;
  };
  const float fx = f(X / 0.95047f), fy = f(Y), fz = f(Z / 1.08883f);
  return { 116.0f * fy - 16.0f, 500.0f * (fx - fy), 200.0f * (fy - fz) };
}

float labDist(const Lab& a, const Lab& b) noexcept {
  const float dL = a.L - b.L, da = a.a - b.a, db = a.b - b.b;
  return std::sqrt(dL * dL + da * da + db * db);
}

// Perceptual distance including alpha awareness.
// Returns a value in the same ΔE scale as labDist.
// Fully transparent pixels form their own equivalence class.
float pixelDist(const Color& sc, const Lab& sl, std::uint8_t sa,
                const Color& rc, const Lab& rl) noexcept {
  const std::uint8_t ra = rc.a;
  if (sa < 10 && ra < 10) return 0.0f;   // both transparent → match
  if (sa < 10 || ra < 10) return 9999.0f; // opacity mismatch → no match
  // Both opaque: Lab + small alpha penalty
  const float alphaPenalty =
      std::abs(static_cast<float>(sa) - static_cast<float>(ra)) * (50.0f / 255.0f);
  return labDist(sl, rl) + alphaPenalty * 0.15f;
}

// Mask value from distance and tolerance (soft edge in outer 20% of range)
std::uint8_t distToMask(float dist, float labTol) noexcept {
  if (dist >= labTol) return 0U;
  const float softZone = labTol * 0.20f;
  const float hardEdge = labTol - softZone;
  if (dist <= hardEdge) return 255U;
  const float t = (labTol - dist) / softZone;  // 1.0 at hardEdge → 0.0 at labTol
  return static_cast<std::uint8_t>(std::clamp(t * 255.0f + 0.5f, 0.0f, 255.0f));
}

// Fast luma for Sobel edge map
float luma(const Color& c) noexcept {
  return 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// floodFillSelect  — Lab色差エッジアウェア洪水塗り (MagicWand / Object)
// ─────────────────────────────────────────────────────────────────────────────
// edge-aware BFS helper: Sobel magnitude at (x,y) from reference, normalized 0-1
static float sobelMag(const PixelBuffer& ref, int x, int y, int w, int h) noexcept {
  if (x < 1 || y < 1 || x >= w - 1 || y >= h - 1) return 0.0f;
  const float gx =
      luma(ref.pixel(x+1,y-1)) + 2.0f*luma(ref.pixel(x+1,y)) + luma(ref.pixel(x+1,y+1)) -
      luma(ref.pixel(x-1,y-1)) - 2.0f*luma(ref.pixel(x-1,y)) - luma(ref.pixel(x-1,y+1));
  const float gy =
      luma(ref.pixel(x-1,y+1)) + 2.0f*luma(ref.pixel(x,y+1)) + luma(ref.pixel(x+1,y+1)) -
      luma(ref.pixel(x-1,y-1)) - 2.0f*luma(ref.pixel(x,y-1)) - luma(ref.pixel(x+1,y-1));
  return std::sqrt(gx*gx + gy*gy) / 1440.0f;  // max Sobel ≈ 1440
}

SelectionResult ClassicProvider::floodFillSelect(const PixelBuffer& reference,
                                                  const Point&       seed,
                                                  int                tolerance,
                                                  bool               contiguous,
                                                  bool               antiAlias,
                                                  bool               edgeAware,
                                                  int w, int h) {
  SelectionResult out;
  out.mask = SelectionMask(w, h);
  if (w <= 0 || h <= 0) return out;
  if (seed.x < 0 || seed.y < 0 || seed.x >= w || seed.y >= h) return out;

  // Tolerance: user 0-255 → ΔE 0-100
  const float labTol = static_cast<float>(tolerance) * (100.0f / 255.0f) + 0.5f;

  const Color        seedColor = reference.pixel(seed.x, seed.y);
  const Lab          seedLab   = rgbToLab(seedColor.r, seedColor.g, seedColor.b);
  const std::uint8_t seedAlpha = seedColor.a;

  const std::size_t total = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
  std::vector<std::uint8_t> maskPixels(total, 0U);
  std::vector<float>        confidence(total, 0.0f);

  auto idx = [w](int x, int y) -> std::size_t {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
           static_cast<std::size_t>(x);
  };

  // ΔE-scale distance for propagation check
  auto dist = [&](int x, int y) -> float {
    const Color rc = reference.pixel(x, y);
    const Lab   rl = rgbToLab(rc.r, rc.g, rc.b);
    return pixelDist(seedColor, seedLab, seedAlpha, rc, rl);
  };

  // Edge-aware threshold: BFS stops when Sobel mag > kEdgeStop.
  // Tuned for line art (ink on white): strong edges are ink lines.
  constexpr float kEdgeStop = 0.12f;  // ~170/1440 — catches medium-contrast lines

  auto matches = [&](int x, int y) -> bool {
    if (dist(x, y) > labTol) return false;
    if (edgeAware && sobelMag(reference, x, y, w, h) > kEdgeStop) return false;
    return true;
  };

  auto setPixel = [&](int x, int y) {
    const std::size_t i  = idx(x, y);
    const Color       rc = reference.pixel(x, y);
    const Lab         rl = rgbToLab(rc.r, rc.g, rc.b);
    const float       d  = pixelDist(seedColor, seedLab, seedAlpha, rc, rl);
    const std::uint8_t v = antiAlias ? distToMask(d, labTol) : 255U;
    maskPixels[i]        = v;
    confidence[i]        = static_cast<float>(v) / 255.0f;
  };

  if (contiguous) {
    // Span-fill BFS — propagate only through hard-matching pixels
    struct Span { int y, x0, x1; };
    std::vector<Span> stack;
    stack.reserve(512);

    auto pushSpan = [&](int y, int x0, int x1) {
      if (y >= 0 && y < h) stack.push_back({y, x0, x1});
    };

    if (matches(seed.x, seed.y)) {
      int l = seed.x, r = seed.x;
      while (l > 0     && matches(l - 1, seed.y)) --l;
      while (r < w - 1 && matches(r + 1, seed.y)) ++r;
      for (int x = l; x <= r; ++x) setPixel(x, seed.y);
      pushSpan(seed.y - 1, l, r);
      pushSpan(seed.y + 1, l, r);
    }

    while (!stack.empty()) {
      const auto [sy, sx0, sx1] = stack.back();
      stack.pop_back();

      for (int x = sx0; x <= sx1; ) {
        if (maskPixels[idx(x, sy)] != 0U || !matches(x, sy)) { ++x; continue; }
        int l = x;
        while (l > 0     && maskPixels[idx(l-1,sy)] == 0U && matches(l-1, sy)) --l;
        int r = x;
        while (r < w - 1 && maskPixels[idx(r+1,sy)] == 0U && matches(r+1, sy)) ++r;
        for (int px = l; px <= r; ++px) setPixel(px, sy);
        pushSpan(sy - 1, l, r);
        pushSpan(sy + 1, l, r);
        x = r + 1;
      }
    }
  } else {
    // Non-contiguous: scan entire canvas
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        if (matches(x, y)) setPixel(x, y);
      }
    }
  }

  out.mask.setPixels(maskPixels);
  out.confidenceMap = std::move(confidence);

  // ── Edge map: Sobel on luma, computed in bounding rect of result ─────────
  {
    const auto bounds = out.mask.boundingRect();
    if (bounds.has_value()) {
      const int ex0 = std::max(1,     bounds->x);
      const int ey0 = std::max(1,     bounds->y);
      const int ex1 = std::min(w - 2, bounds->x + bounds->width  - 1);
      const int ey1 = std::min(h - 2, bounds->y + bounds->height - 1);

      std::vector<std::uint8_t> edgeMap(total, 0U);
      for (int y = ey0; y <= ey1; ++y) {
        for (int x = ex0; x <= ex1; ++x) {
          const float gx =
              luma(reference.pixel(x+1,y-1)) + 2.0f*luma(reference.pixel(x+1,y)) +
              luma(reference.pixel(x+1,y+1)) -
              luma(reference.pixel(x-1,y-1)) - 2.0f*luma(reference.pixel(x-1,y)) -
              luma(reference.pixel(x-1,y+1));
          const float gy =
              luma(reference.pixel(x-1,y+1)) + 2.0f*luma(reference.pixel(x,y+1)) +
              luma(reference.pixel(x+1,y+1)) -
              luma(reference.pixel(x-1,y-1)) - 2.0f*luma(reference.pixel(x,y-1)) -
              luma(reference.pixel(x+1,y-1));
          const float mag = std::sqrt(gx*gx + gy*gy) / 1440.0f * 255.0f;
          edgeMap[idx(x,y)] = static_cast<std::uint8_t>(std::clamp(mag, 0.0f, 255.0f));
        }
      }
      out.edgeMap = std::move(edgeMap);
    }
  }

  return out;
}

} // namespace core
