#include "core/tools/GradientTool.h"

#include <algorithm>
#include <cmath>

#include "core/buffer/PixelBuffer.h"
#include "core/document/Document.h"
#include "core/layer/Layer.h"

namespace core {

namespace {

// ── 共通ヘルパー ───────────────────────────────────────────────────────────

inline float toF(uint8_t v) noexcept { return static_cast<float>(v) / 255.0f; }
inline uint8_t fromF(float v) noexcept {
  return static_cast<uint8_t>(std::clamp(v * 255.0f + 0.5f, 0.0f, 255.0f));
}
inline float clamp01(float v) noexcept { return std::clamp(v, 0.0f, 1.0f); }
inline uint8_t clamp8(float v) noexcept {
  return static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f));
}

/// 線形補間（ピクセル値）
inline uint8_t lerp8(uint8_t a, uint8_t b, float t) noexcept {
  return clamp8(static_cast<float>(a) + t * (static_cast<float>(b) - static_cast<float>(a)));
}

// ── HSL ヘルパー（BrushTool と同じ実装）──────────────────────────────────

struct RGB3 { float r, g, b; };

inline float hslLuminance(float r, float g, float b) noexcept {
  return 0.299f * r + 0.587f * g + 0.114f * b;
}
inline float hslSaturation(float r, float g, float b) noexcept {
  return std::max({r, g, b}) - std::min({r, g, b});
}
RGB3 clipColor(RGB3 c) noexcept {
  const float lum = hslLuminance(c.r, c.g, c.b);
  const float cmin = std::min({c.r, c.g, c.b});
  const float cmax = std::max({c.r, c.g, c.b});
  if (cmin < 0.0f) {
    c.r = lum + (c.r - lum) * lum / (lum - cmin);
    c.g = lum + (c.g - lum) * lum / (lum - cmin);
    c.b = lum + (c.b - lum) * lum / (lum - cmin);
  }
  if (cmax > 1.0f) {
    c.r = lum + (c.r - lum) * (1.0f - lum) / (cmax - lum);
    c.g = lum + (c.g - lum) * (1.0f - lum) / (cmax - lum);
    c.b = lum + (c.b - lum) * (1.0f - lum) / (cmax - lum);
  }
  return c;
}
RGB3 setLuminance(RGB3 c, float lum) noexcept {
  const float d = lum - hslLuminance(c.r, c.g, c.b);
  return clipColor({c.r + d, c.g + d, c.b + d});
}
RGB3 setSaturation(RGB3 c, float sat) noexcept {
  float cmin = std::min({c.r, c.g, c.b});
  float cmax = std::max({c.r, c.g, c.b});
  const float cmid_v = c.r + c.g + c.b - cmin - cmax;
  if (cmax <= cmin) {
    return {0.0f, 0.0f, 0.0f};
  }
  // Map cmin→0, cmax→sat
  auto mapCh = [&](float ch) {
    if (ch == cmin) return 0.0f;
    if (ch == cmax) return sat;
    return (ch - cmin) * sat / (cmax - cmin);
  };
  const float r2 = mapCh(c.r), g2 = mapCh(c.g), b2 = mapCh(c.b);
  static_cast<void>(cmid_v);
  return {r2, g2, b2};
}

// ── ポーター・ダフ over 合成 ──────────────────────────────────────────────

/// src over dst (アルファ合成) — 基本合成演算子
Color porterDuffOver(const Color& dst, const Color& src) noexcept {
  const float sA = toF(src.a);
  const float dA = toF(dst.a);
  if (sA <= 0.0f) {
    return dst;
  }
  const float outA = sA + dA * (1.0f - sA);
  if (outA <= 0.0f) {
    return Color::Transparent();
  }
  auto ch = [&](uint8_t sc, uint8_t dc) -> uint8_t {
    return fromF((toF(sc) * sA + toF(dc) * dA * (1.0f - sA)) / outA);
  };
  return Color {ch(src.r, dst.r), ch(src.g, dst.g), ch(src.b, dst.b), fromF(outA)};
}

/// src に混合演算子 f(s,d) を適用し、アルファ合成を行う汎用関数
Color applyBlend(const Color& dst, const Color& src,
                 float fr, float fg, float fb) noexcept {
  const float sA = toF(src.a);
  const float dA = toF(dst.a);
  if (sA <= 0.0f) {
    return dst;
  }
  const float outA = sA + dA * (1.0f - sA);
  if (outA <= 0.0f) {
    return Color::Transparent();
  }
  // Porter-Duff: blended = (1-dA)*srcC + (1-sA)*dstC + dA*sA * f(s,d)
  auto ch = [&](float fs, float fd, float blended) -> uint8_t {
    return fromF(((1.0f - dA) * fs * sA + (1.0f - sA) * fd * dA + dA * sA * blended) / outA);
  };
  return Color {
      ch(toF(src.r), toF(dst.r), fr),
      ch(toF(src.g), toF(dst.g), fg),
      ch(toF(src.b), toF(dst.b), fb),
      fromF(outA)};
}

/// Dissolve — アルファに応じたランダム散布（hash ベース）
Color blendDissolve(const Color& dst, const Color& src, int x, int y) noexcept {
  const uint32_t hash = static_cast<uint32_t>(x * 1234567 + y * 7654321 + x * y * 31337);
  const float rand01 = (hash & 0xFFFFu) / 65535.0f;
  if (rand01 > toF(src.a)) {
    return dst;
  }
  Color opaque = src;
  opaque.a = dst.a;
  return opaque;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────

ToolResult GradientTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  m_drawing = true;
  m_start   = event.fpoint;
  m_current = event.fpoint;
  return {};
}

ToolResult GradientTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  if (!m_drawing) {
    return {};
  }
  m_current = event.fpoint;
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult GradientTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  if (!m_drawing) {
    return {};
  }
  m_drawing = false;
  m_current = event.fpoint;

  Layer* layer = context.document.activeLayer();
  if (layer == nullptr || layer->locked()) {
    return {};
  }

  const float dx = m_current.x - m_start.x;
  const float dy = m_current.y - m_start.y;
  // 最低でも 1 px の距離が必要
  if (std::sqrt(dx * dx + dy * dy) < 1.0f) {
    return {};
  }

  applyGradient(context, m_start, m_current);

  ToolResult result;
  result.pixelsChanged = true;
  result.dirtyRect = core::Rect {
      0, 0, context.document.canvasSize().width, context.document.canvasSize().height};
  return result;
}

ToolResult GradientTool::onCancel(ToolContext& context) {
  static_cast<void>(context);
  m_drawing = false;
  return {};
}

ToolResult GradientTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(deltaSteps);
  static_cast<void>(event);
  return {};
}

ToolOverlayState GradientTool::overlay() const {
  if (!m_drawing) {
    return {};
  }
  ToolOverlayState state;
  state.hasLine   = true;
  state.lineStart = Point {static_cast<int>(m_start.x),   static_cast<int>(m_start.y)};
  state.lineEnd   = Point {static_cast<int>(m_current.x), static_cast<int>(m_current.y)};
  return state;
}

// ── プライベートメソッド ───────────────────────────────────────────────────

Color GradientTool::sampleGradient(float t, const Color& fgColor, const Color& bgColor) const {
  t = std::clamp(t, 0.0f, 1.0f);
  switch (m_fill) {
    case GradientFill::ForegroundToBackground: {
      Color c;
      c.r = lerp8(fgColor.r, bgColor.r, t);
      c.g = lerp8(fgColor.g, bgColor.g, t);
      c.b = lerp8(fgColor.b, bgColor.b, t);
      c.a = lerp8(fgColor.a, bgColor.a, t);
      // ユーザーの不透明度を掛ける
      c.a = clamp8(static_cast<float>(c.a) * m_opacity);
      return c;
    }
    case GradientFill::ForegroundToTransparent: {
      Color c = fgColor;
      c.a = clamp8(static_cast<float>(fgColor.a) * (1.0f - t) * m_opacity);
      return c;
    }
  }
  return fgColor;
}

Color GradientTool::blendWithDst(const Color& dst, const Color& src, int px, int py) const {
  if (m_eraseMode) {
    // 消去モード: src のアルファ分だけ dst を透明化
    const float eraseAlpha = toF(src.a);
    const float newA       = toF(dst.a) * (1.0f - eraseAlpha);
    return Color {dst.r, dst.g, dst.b, fromF(newA)};
  }

  const float sr = toF(src.r), sg = toF(src.g), sb = toF(src.b);
  const float dr = toF(dst.r), dg = toF(dst.g), db = toF(dst.b);

  switch (m_blendMode) {
    case BlendMode::Normal:
      return porterDuffOver(dst, src);
    case BlendMode::Dissolve:
      return blendDissolve(dst, src, px, py);

    // ── 暗くする ─────────────────────────────────────────────────────────
    case BlendMode::Darken:
      return applyBlend(dst, src, std::min(sr,dr), std::min(sg,dg), std::min(sb,db));
    case BlendMode::Multiply:
      return applyBlend(dst, src, sr*dr, sg*dg, sb*db);
    case BlendMode::ColorBurn:
      return applyBlend(dst, src,
          sr>0.0f ? clamp01(1.0f-(1.0f-dr)/sr) : 0.0f,
          sg>0.0f ? clamp01(1.0f-(1.0f-dg)/sg) : 0.0f,
          sb>0.0f ? clamp01(1.0f-(1.0f-db)/sb) : 0.0f);
    case BlendMode::LinearBurn:
      return applyBlend(dst, src, clamp01(sr+dr-1.0f), clamp01(sg+dg-1.0f), clamp01(sb+db-1.0f));
    case BlendMode::DarkerColor: {
      const float lumS = hslLuminance(sr,sg,sb);
      const float lumD = hslLuminance(dr,dg,db);
      return (lumS <= lumD) ? porterDuffOver(dst, src) : dst;
    }

    // ── 明るくする ────────────────────────────────────────────────────────
    case BlendMode::Lighten:
      return applyBlend(dst, src, std::max(sr,dr), std::max(sg,dg), std::max(sb,db));
    case BlendMode::Screen:
      return applyBlend(dst, src, 1.0f-(1.0f-sr)*(1.0f-dr), 1.0f-(1.0f-sg)*(1.0f-dg), 1.0f-(1.0f-sb)*(1.0f-db));
    case BlendMode::ColorDodge:
      return applyBlend(dst, src,
          sr<1.0f ? clamp01(dr/(1.0f-sr)) : 1.0f,
          sg<1.0f ? clamp01(dg/(1.0f-sg)) : 1.0f,
          sb<1.0f ? clamp01(db/(1.0f-sb)) : 1.0f);
    case BlendMode::LinearDodge:
      return applyBlend(dst, src, clamp01(sr+dr), clamp01(sg+dg), clamp01(sb+db));
    case BlendMode::LighterColor: {
      const float lumS = hslLuminance(sr,sg,sb);
      const float lumD = hslLuminance(dr,dg,db);
      return (lumS >= lumD) ? porterDuffOver(dst, src) : dst;
    }

    // ── コントラスト ──────────────────────────────────────────────────────
    case BlendMode::Overlay:
      return applyBlend(dst, src,
          dr<0.5f ? 2.0f*sr*dr : 1.0f-2.0f*(1.0f-sr)*(1.0f-dr),
          dg<0.5f ? 2.0f*sg*dg : 1.0f-2.0f*(1.0f-sg)*(1.0f-dg),
          db<0.5f ? 2.0f*sb*db : 1.0f-2.0f*(1.0f-sb)*(1.0f-db));
    case BlendMode::SoftLight: {
      auto sl = [](float s, float d) {
        if (s <= 0.5f) return d - (1.0f-2.0f*s)*d*(1.0f-d);
        const float g = d <= 0.25f ? ((16.0f*d-12.0f)*d+4.0f)*d : std::sqrt(d);
        return d + (2.0f*s-1.0f)*(g-d);
      };
      return applyBlend(dst, src, sl(sr,dr), sl(sg,dg), sl(sb,db));
    }
    case BlendMode::HardLight:
      return applyBlend(dst, src,
          sr<0.5f ? 2.0f*sr*dr : 1.0f-2.0f*(1.0f-sr)*(1.0f-dr),
          sg<0.5f ? 2.0f*sg*dg : 1.0f-2.0f*(1.0f-sg)*(1.0f-dg),
          sb<0.5f ? 2.0f*sb*db : 1.0f-2.0f*(1.0f-sb)*(1.0f-db));
    case BlendMode::VividLight:
      return applyBlend(dst, src,
          sr<=0.5f ? (sr>0.0f ? clamp01(1.0f-(1.0f-dr)/(2.0f*sr)) : 0.0f)
                   : (sr<1.0f ? clamp01(dr/(2.0f*(1.0f-sr))) : 1.0f),
          sg<=0.5f ? (sg>0.0f ? clamp01(1.0f-(1.0f-dg)/(2.0f*sg)) : 0.0f)
                   : (sg<1.0f ? clamp01(dg/(2.0f*(1.0f-sg))) : 1.0f),
          sb<=0.5f ? (sb>0.0f ? clamp01(1.0f-(1.0f-db)/(2.0f*sb)) : 0.0f)
                   : (sb<1.0f ? clamp01(db/(2.0f*(1.0f-sb))) : 1.0f));
    case BlendMode::LinearLight:
      return applyBlend(dst, src, clamp01(2.0f*sr+dr-1.0f), clamp01(2.0f*sg+dg-1.0f), clamp01(2.0f*sb+db-1.0f));
    case BlendMode::PinLight:
      return applyBlend(dst, src,
          sr<0.5f ? std::min(dr,2.0f*sr) : std::max(dr,2.0f*sr-1.0f),
          sg<0.5f ? std::min(dg,2.0f*sg) : std::max(dg,2.0f*sg-1.0f),
          sb<0.5f ? std::min(db,2.0f*sb) : std::max(db,2.0f*sb-1.0f));
    case BlendMode::HardMix:
      return applyBlend(dst, src,
          (sr+dr)>=1.0f ? 1.0f : 0.0f,
          (sg+dg)>=1.0f ? 1.0f : 0.0f,
          (sb+db)>=1.0f ? 1.0f : 0.0f);

    // ── 比較 ──────────────────────────────────────────────────────────────
    case BlendMode::Difference:
      return applyBlend(dst, src, std::abs(sr-dr), std::abs(sg-dg), std::abs(sb-db));
    case BlendMode::Exclusion:
      return applyBlend(dst, src, sr+dr-2.0f*sr*dr, sg+dg-2.0f*sg*dg, sb+db-2.0f*sb*db);
    case BlendMode::Subtract:
      return applyBlend(dst, src, clamp01(dr-sr), clamp01(dg-sg), clamp01(db-sb));
    case BlendMode::Divide:
      return applyBlend(dst, src,
          sr>0.0f ? clamp01(dr/sr) : 1.0f,
          sg>0.0f ? clamp01(dg/sg) : 1.0f,
          sb>0.0f ? clamp01(db/sb) : 1.0f);

    // ── HSL ──────────────────────────────────────────────────────────────
    case BlendMode::Hue: {
      const RGB3 res = setLuminance(setSaturation({sr,sg,sb}, hslSaturation(dr,dg,db)),
                                    hslLuminance(dr,dg,db));
      return applyBlend(dst, src, res.r, res.g, res.b);
    }
    case BlendMode::HslSat: {
      const RGB3 res = setLuminance(setSaturation({dr,dg,db}, hslSaturation(sr,sg,sb)),
                                    hslLuminance(dr,dg,db));
      return applyBlend(dst, src, res.r, res.g, res.b);
    }
    case BlendMode::HslColor: {
      const RGB3 res = setLuminance({sr,sg,sb}, hslLuminance(dr,dg,db));
      return applyBlend(dst, src, res.r, res.g, res.b);
    }
    case BlendMode::Luminosity: {
      const RGB3 res = setLuminance({dr,dg,db}, hslLuminance(sr,sg,sb));
      return applyBlend(dst, src, res.r, res.g, res.b);
    }
  }
  return porterDuffOver(dst, src);
}

void GradientTool::applyGradient(ToolContext& context, const FPoint& start, const FPoint& end) const {
  Layer* layer = context.document.activeLayer();
  if (layer == nullptr) {
    return;
  }
  PixelBuffer& buf = layer->buffer();
  const int W = buf.width();
  const int H = buf.height();
  if (W == 0 || H == 0) {
    return;
  }

  const Color fgColor = context.currentColor;
  const Color bgColor = context.secondaryColor;

  const float vx = end.x - start.x;
  const float vy = end.y - start.y;

  if (m_type == GradientType::Linear) {
    // 直線: t = dot(pos - start, dir) / |dir|^2
    const float lenSq = vx * vx + vy * vy;
    for (int y = 0; y < H; ++y) {
      for (int x = 0; x < W; ++x) {
        const float px = static_cast<float>(x) - start.x;
        const float py = static_cast<float>(y) - start.y;
        const float t  = (px * vx + py * vy) / lenSq;
        const Color src = sampleGradient(t, fgColor, bgColor);
        const Color dst = buf.pixel(x, y);
        buf.setPixel(x, y, blendWithDst(dst, src, x, y));
      }
    }
  } else {
    // 放射: t = distance(pos, start) / distance(end, start)
    const float maxDist = std::sqrt(vx * vx + vy * vy);
    if (maxDist < 0.001f) {
      return;
    }
    for (int y = 0; y < H; ++y) {
      for (int x = 0; x < W; ++x) {
        const float px   = static_cast<float>(x) - start.x;
        const float py   = static_cast<float>(y) - start.y;
        const float dist = std::sqrt(px * px + py * py);
        const float t    = dist / maxDist;
        const Color src  = sampleGradient(t, fgColor, bgColor);
        const Color dst  = buf.pixel(x, y);
        buf.setPixel(x, y, blendWithDst(dst, src, x, y));
      }
    }
  }
}

} // namespace core
