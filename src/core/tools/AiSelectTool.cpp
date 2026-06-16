#include "core/tools/AiSelectTool.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <unordered_set>
#include <vector>

#include "core/buffer/PixelBuffer.h"
#include "core/document/Document.h"
#include "core/selection/SelectionMask.h"
#include "core/tools/ToolContext.h"

namespace core {

// ─────────────────────────────────────────────────────────────────────────────
// helpers
// ─────────────────────────────────────────────────────────────────────────────
float AiSelectTool::colorDist(const Color& a, const Color& b) noexcept {
  const float dr = static_cast<float>(a.r) - static_cast<float>(b.r);
  const float dg = static_cast<float>(a.g) - static_cast<float>(b.g);
  const float db = static_cast<float>(a.b) - static_cast<float>(b.b);
  const float da = static_cast<float>(a.a) - static_cast<float>(b.a);
  return std::sqrt(0.299f*dr*dr + 0.587f*dg*dg + 0.114f*db*db + 0.15f*da*da);
}

float AiSelectTool::sobelMagnitude(const PixelBuffer& buf, int x, int y) noexcept {
  if (x <= 0 || y <= 0 || x >= buf.width()-1 || y >= buf.height()-1) return 0.f;
  auto luma = [&](int px, int py) -> float {
    const Color c = buf.pixel(px, py);
    return 0.299f*c.r + 0.587f*c.g + 0.114f*c.b;
  };
  const float gx =
      -luma(x-1,y-1) + luma(x+1,y-1)
      -2.f*luma(x-1,y) + 2.f*luma(x+1,y)
      -luma(x-1,y+1) + luma(x+1,y+1);
  const float gy =
       luma(x-1,y-1) + 2.f*luma(x,y-1) + luma(x+1,y-1)
      -luma(x-1,y+1) - 2.f*luma(x,y+1) - luma(x+1,y+1);
  return std::sqrt(gx*gx + gy*gy);
}

// ─────────────────────────────────────────────────────────────────────────────
// Stub segmentation  — エッジ重み付き適応フラッドフィル
// ─────────────────────────────────────────────────────────────────────────────
SelectionMask AiSelectTool::runStubSegmentation(
    const PixelBuffer&         source,
    const SelectionMask&       currentSelection,
    const std::vector<Point>&  positivePoints,
    const std::vector<Point>&  negativePoints,
    int                        thresholdOverride) const
{
  const int W = source.width();
  const int H = source.height();
  SelectionMask empty(W, H);
  if (W <= 0 || H <= 0 || positivePoints.empty()) return empty;

  // ── ステップ1: 種点周辺サンプリング → 色統計 ────────────────────────────
  constexpr int kSampleRadius = 4;
  std::vector<float> rS, gS, bS;
  rS.reserve(positivePoints.size() * (2*kSampleRadius+1) * (2*kSampleRadius+1));
  gS.reserve(rS.capacity());
  bS.reserve(rS.capacity());
  for (const Point& p : positivePoints) {
    for (int dy = -kSampleRadius; dy <= kSampleRadius; ++dy) {
      for (int dx = -kSampleRadius; dx <= kSampleRadius; ++dx) {
        const int sx = p.x+dx, sy = p.y+dy;
        if (sx<0||sy<0||sx>=W||sy>=H) continue;
        const Color c = source.pixel(sx, sy);
        rS.push_back(c.r); gS.push_back(c.g); bS.push_back(c.b);
      }
    }
  }
  auto mean = [](const std::vector<float>& v, float def) {
    return v.empty() ? def : std::accumulate(v.begin(),v.end(),0.f)/v.size();
  };
  const float rM = mean(rS, 128.f), gM = mean(gS, 128.f), bM = mean(bS, 128.f);
  auto var = [](const std::vector<float>& v, float m) {
    float s = 0; for (auto x : v) s += (x-m)*(x-m); return v.empty() ? 0.f : s/v.size();
  };
  const float variance = (var(rS,rM)+var(gS,gM)+var(bS,bM)) / 3.f;
  const Color seedColor{ static_cast<std::uint8_t>(rM),
                          static_cast<std::uint8_t>(gM),
                          static_cast<std::uint8_t>(bM), 255 };
  const int baseThresh  = (thresholdOverride >= 0) ? thresholdOverride : m_settings.threshold;
  const float dynThresh = static_cast<float>(baseThresh)
                          + std::min(std::sqrt(variance)*0.5f, 40.f);

  // ── ステップ2: ネガティブポイントの色収集 ────────────────────────────────
  std::vector<Color> negColors;
  negColors.reserve(negativePoints.size());
  for (const Point& p : negativePoints) {
    if (p.x>=0&&p.y>=0&&p.x<W&&p.y<H) negColors.push_back(source.pixel(p.x,p.y));
  }

  // ── ステップ3: ソーベルエッジマップ ──────────────────────────────────────
  const std::size_t SZ = static_cast<std::size_t>(W)*static_cast<std::size_t>(H);
  std::vector<float> edgeMap(SZ, 0.f);
  float edgeMax = 1.f;
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x) {
      const float m = sobelMagnitude(source, x, y);
      edgeMap[static_cast<std::size_t>(y)*W+x] = m;
      edgeMax = std::max(edgeMax, m);
    }
  for (auto& v : edgeMap) v /= edgeMax;

  // ── ステップ4: コストベース BFS ──────────────────────────────────────────
  std::vector<bool>  visited(SZ, false);
  std::vector<float> weight(SZ, 0.f);
  std::vector<Point> queue;
  queue.reserve(SZ / 4);

  for (const Point& p : positivePoints) {
    if (p.x<0||p.y<0||p.x>=W||p.y>=H) continue;
    const std::size_t idx = static_cast<std::size_t>(p.y)*W+p.x;
    if (!visited[idx]) { visited[idx]=true; weight[idx]=1.f; queue.push_back(p); }
  }

  static constexpr int  kDx[] = {1,-1,0,0, 1,-1, 1,-1};
  static constexpr int  kDy[] = {0,0,1,-1, 1, 1,-1,-1};
  static constexpr float kDiag = 0.707f;

  for (std::size_t qi = 0; qi < queue.size(); ++qi) {
    const Point cur = queue[qi];
    const float pw  = weight[static_cast<std::size_t>(cur.y)*W+cur.x];
    for (int d = 0; d < 8; ++d) {
      const int nx = cur.x+kDx[d], ny = cur.y+kDy[d];
      if (nx<0||ny<0||nx>=W||ny>=H) continue;
      const std::size_t nidx = static_cast<std::size_t>(ny)*W+nx;
      if (visited[nidx]) continue;
      visited[nidx] = true;

      const Color nc = source.pixel(nx, ny);
      bool negHit = false;
      for (const Color& neg : negColors)
        if (colorDist(neg, nc) < dynThresh*0.5f) { negHit=true; break; }
      if (negHit) continue;

      const float edge    = edgeMap[nidx];
      const float cost    = colorDist(seedColor, nc) + edge*dynThresh*1.5f
                            + (d >= 4 ? colorDist(seedColor,nc)*(1.f-kDiag) : 0.f);
      if (cost > dynThresh) continue;
      const float w = pw * (1.f - cost/(dynThresh+1.f));
      if (w < 0.05f) continue;
      weight[nidx] = w;
      queue.push_back(Point{nx, ny});
    }
  }

  // ── ステップ5: pixel マスクを生成 → SelectionMask::setPixels ─────────────
  std::vector<std::uint8_t> pixels(SZ, 0);
  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      const float w = weight[static_cast<std::size_t>(y)*W+x];
      if (w <= 0.f) continue;
      bool sel;
      if (m_settings.antiAlias) {
        const float clamped = std::clamp(w*1.5f, 0.f, 1.f);
        sel = (clamped >= 0.5f);
      } else {
        sel = (w >= 0.5f);
      }
      if (sel) pixels[static_cast<std::size_t>(y)*W+x] = 255;
    }
  }

  // ── 既存選択との合成 ─────────────────────────────────────────────────────
  if (m_settings.addMode && currentSelection.hasSelection()) {
    for (int y = 0; y < H; ++y)
      for (int x = 0; x < W; ++x)
        if (currentSelection.contains(x,y))
          pixels[static_cast<std::size_t>(y)*W+x] = 255;
  } else if (m_settings.subtractMode && currentSelection.hasSelection()) {
    for (int y = 0; y < H; ++y)
      for (int x = 0; x < W; ++x)
        if (!currentSelection.contains(x,y))
          pixels[static_cast<std::size_t>(y)*W+x] = 0;
  }

  SelectionMask result(W, H);
  result.setPixels(pixels);
  return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Morphological close で選択マスク境界を平滑化
// ─────────────────────────────────────────────────────────────────────────────
SelectionMask AiSelectTool::smoothMask(const SelectionMask& src, int radius) {
  if (radius <= 0) return src;
  const int W = src.width(), H = src.height();
  if (W <= 0 || H <= 0) return src;
  const int SZ = W * H;
  std::vector<std::uint8_t> buf(static_cast<std::size_t>(SZ), 0);
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x)
      buf[static_cast<std::size_t>(y)*W+x] = src.contains(x,y) ? 255 : 0;

  // dilate
  std::vector<std::uint8_t> tmp(static_cast<std::size_t>(SZ), 0);
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x) {
      bool hit = false;
      for (int dy = -radius; dy <= radius && !hit; ++dy)
        for (int dx = -radius; dx <= radius && !hit; ++dx) {
          const int nx = x+dx, ny = y+dy;
          if (nx>=0&&ny>=0&&nx<W&&ny<H && buf[static_cast<std::size_t>(ny)*W+nx]) hit=true;
        }
      tmp[static_cast<std::size_t>(y)*W+x] = hit ? 255 : 0;
    }

  // erode
  std::vector<std::uint8_t> out(static_cast<std::size_t>(SZ), 0);
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x) {
      bool all = true;
      for (int dy = -radius; dy <= radius && all; ++dy)
        for (int dx = -radius; dx <= radius && all; ++dx) {
          const int nx = x+dx, ny = y+dy;
          if (nx<0||ny<0||nx>=W||ny>=H||!tmp[static_cast<std::size_t>(ny)*W+nx]) all=false;
        }
      out[static_cast<std::size_t>(y)*W+x] = all ? 255 : 0;
    }

  SelectionMask result(W, H);
  result.setPixels(out);
  return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Morphological dilation/erosion でマスクを拡張または縮小
// ─────────────────────────────────────────────────────────────────────────────
SelectionMask AiSelectTool::expandMask(const SelectionMask& src, int pixels) {
  if (pixels == 0) return src;
  const int W = src.width(), H = src.height();
  if (W <= 0 || H <= 0) return src;
  const int SZ = W * H;
  std::vector<std::uint8_t> buf(static_cast<std::size_t>(SZ), 0);
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x)
      buf[static_cast<std::size_t>(y)*W+x] = src.contains(x,y) ? 255 : 0;

  std::vector<std::uint8_t> out(static_cast<std::size_t>(SZ), 0);
  const int r = std::abs(pixels);
  if (pixels > 0) {
    // dilation: 近傍に 255 があれば 255
    for (int y = 0; y < H; ++y)
      for (int x = 0; x < W; ++x) {
        bool hit = false;
        for (int dy = -r; dy <= r && !hit; ++dy)
          for (int dx = -r; dx <= r && !hit; ++dx) {
            const int nx = x+dx, ny = y+dy;
            if (nx>=0&&ny>=0&&nx<W&&ny<H && buf[static_cast<std::size_t>(ny)*W+nx]) hit=true;
          }
        out[static_cast<std::size_t>(y)*W+x] = hit ? 255 : 0;
      }
  } else {
    // erosion: 近傍が全て 255 のとき 255
    for (int y = 0; y < H; ++y)
      for (int x = 0; x < W; ++x) {
        bool all = true;
        for (int dy = -r; dy <= r && all; ++dy)
          for (int dx = -r; dx <= r && all; ++dx) {
            const int nx = x+dx, ny = y+dy;
            if (nx<0||ny<0||nx>=W||ny>=H||!buf[static_cast<std::size_t>(ny)*W+nx]) all=false;
          }
        out[static_cast<std::size_t>(y)*W+x] = all ? 255 : 0;
      }
  }

  SelectionMask result(W, H);
  result.setPixels(out);
  return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// RotoBrush helpers
// ─────────────────────────────────────────────────────────────────────────────
void AiSelectTool::clearRotoStrokes() noexcept {
  m_rotoStrokes.clear();
  m_activeStroke.clear();
  m_positivePoints.clear();
  m_negativePoints.clear();
  m_strokeActive = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// ITool implementation
// ─────────────────────────────────────────────────────────────────────────────
ToolResult AiSelectTool::onPointerPress(ToolContext& ctx, const ToolPointerEvent& e) {
  // ── RotoBrush モード ─────────────────────────────────────────────────────
  if (m_inputMode == InputMode::RotoBrush) {
    if (e.ctrl) {
      clearRotoStrokes();
      ctx.document.selection().clear();
      ToolResult r; r.selectionChanged = true; return r;
    }
    // Subtract モードなら赤（BG）、それ以外（Add / New）は緑（FG）がデフォルト
    m_paintFg = !m_settings.subtractMode;
    m_activeStroke.clear();
    m_activeStroke.push_back(e.fpoint);
    m_strokeActive = true;
    ToolResult r; r.viewportChanged = true; return r;
  }

  // ── Click モード (元の実装) ───────────────────────────────────────────────
  const PixelBuffer& source = ctx.composited;

  const bool isNegative = e.shift;
  const bool isClear    = e.ctrl;

  if (isClear) {
    m_positivePoints.clear();
    m_negativePoints.clear();
    ctx.document.selection().clear();
    ToolResult r; r.selectionChanged = true; return r;
  }

  if (isNegative) {
    m_negativePoints.push_back(e.point);
  } else {
    if (!m_settings.addMode && !m_settings.subtractMode) {
      m_positivePoints.clear();
      m_negativePoints.clear();
    }
    m_positivePoints.push_back(e.point);
  }

  const SelectionMask& currentSel = ctx.document.selection();
  SelectionMask newMask = runStubSegmentation(source, currentSel,
                                              m_positivePoints, m_negativePoints);
  ctx.document.selection() = std::move(newMask);

  if (m_inferenceCallback) {
    m_inferenceCallback(ctx.composited, m_positivePoints, m_negativePoints);
  }

  ToolResult result; result.selectionChanged = true; return result;
}

ToolResult AiSelectTool::onPointerMove(ToolContext& ctx, const ToolPointerEvent& e) {
  static_cast<void>(ctx);
  if (m_inputMode == InputMode::RotoBrush && m_strokeActive) {
    m_activeStroke.push_back(e.fpoint);
    ToolResult r; r.viewportChanged = true; return r;
  }
  return {};
}

ToolResult AiSelectTool::onPointerRelease(ToolContext& ctx, const ToolPointerEvent& e) {
  static_cast<void>(e);
  if (m_inputMode == InputMode::RotoBrush && m_strokeActive) {
    m_strokeActive = false;
    if (m_activeStroke.empty()) return {};

    // ── ストロークを確定 ─────────────────────────────────────────────────
    ToolOverlayState::RotoStroke finished;
    finished.isForeground = m_paintFg;
    finished.points       = std::move(m_activeStroke);
    m_activeStroke.clear();
    m_rotoStrokes.push_back(std::move(finished));

    // ── 全確定ストロークからプロンプト点をサンプリング ────────────────────
    // 後から描いたストロークが先のストロークを上書き（双方向）:
    // m_rotoStrokes の末尾ほど新しいので、逆順に処理して
    // ブラシ半径サイズのグリッドセル単位で「先取り」する。
    m_positivePoints.clear();
    m_negativePoints.clear();
    {
      const int cellSize = std::max(1, static_cast<int>(m_brushRadius));
      std::unordered_set<std::uint64_t> claimed;

      auto cellKey = [&](int x, int y) -> std::uint64_t {
        const std::uint32_t cx = static_cast<std::uint32_t>(x / cellSize + 32768);
        const std::uint32_t cy = static_cast<std::uint32_t>(y / cellSize + 32768);
        return (static_cast<std::uint64_t>(cx) << 32) | static_cast<std::uint64_t>(cy);
      };

      for (auto it = m_rotoStrokes.rbegin(); it != m_rotoStrokes.rend(); ++it) {
        const auto& stroke = *it;
        auto addPoint = [&](const FPoint& fp) {
          Point p {static_cast<int>(fp.x), static_cast<int>(fp.y)};
          const auto key = cellKey(p.x, p.y);
          if (claimed.count(key)) return;
          claimed.insert(key);
          (stroke.isForeground ? m_positivePoints : m_negativePoints).push_back(p);
        };
        for (std::size_t i = 0; i < stroke.points.size(); i += 8)
          addPoint(stroke.points[i]);
        if (!stroke.points.empty())
          addPoint(stroke.points.back());
      }
    }

    if (m_positivePoints.empty()) return {};

    // ONNX 推論（非同期）— ONNX がある場合はそちらが高精度マスクを返す
    if (m_inferenceCallback) {
      m_inferenceCallback(ctx.composited, m_positivePoints, m_negativePoints);
    } else {
      // スタブフォールバック: ストロークを間引きして分散シードで BFS
      // aiThreshold（デフォルト60）を使うことで多点シードでも孤立島を作らず
      // ひとつの大きな領域に統合される
      constexpr int kStride = 8;  // 8px ごとに1シードを残す
      std::vector<Point> sampledPos, sampledNeg;
      for (std::size_t i = 0; i < m_positivePoints.size(); i += static_cast<std::size_t>(kStride))
        sampledPos.push_back(m_positivePoints[i]);
      if (sampledPos.empty()) sampledPos = m_positivePoints;

      for (std::size_t i = 0; i < m_negativePoints.size(); i += static_cast<std::size_t>(kStride))
        sampledNeg.push_back(m_negativePoints[i]);
      if (m_negativePoints.size() > 0 && sampledNeg.empty()) sampledNeg = m_negativePoints;

      const SelectionMask& currentSel = ctx.document.selection();
      SelectionMask newMask = runStubSegmentation(
          ctx.composited, currentSel, sampledPos, sampledNeg,
          m_settings.aiThreshold);
      if (m_settings.vectorApprox > 0)
        newMask = smoothMask(newMask, m_settings.vectorApprox);
      if (m_settings.expandPixels != 0)
        newMask = expandMask(newMask, m_settings.expandPixels);
      if (m_settings.addMode || m_settings.subtractMode) {
        const int W = newMask.width(), H = newMask.height();
        std::vector<std::uint8_t> px(static_cast<std::size_t>(W * H), 0);
        for (int y = 0; y < H; ++y)
          for (int x = 0; x < W; ++x)
            px[static_cast<std::size_t>(y * W + x)] = newMask.maskValue(x, y);
        const SelectionOp op = m_settings.addMode ? SelectionOp::Add : SelectionOp::Subtract;
        if (ctx.document.selection().width() != W || ctx.document.selection().height() != H) {
          ctx.document.selection() = std::move(newMask);
        } else {
          ctx.document.selection().applyPixels(op, px);
        }
      } else {
        ctx.document.selection() = std::move(newMask);
      }
    }

    ToolResult r; r.selectionChanged = true; return r;
  }
  return {};
}

ToolResult AiSelectTool::onCancel(ToolContext& ctx) {
  static_cast<void>(ctx);
  m_positivePoints.clear();
  m_negativePoints.clear();
  if (m_inputMode == InputMode::RotoBrush) {
    clearRotoStrokes();
  }
  return {};
}

ToolResult AiSelectTool::onWheel(ToolContext& ctx, int deltaSteps, const ToolPointerEvent& e) {
  static_cast<void>(ctx); static_cast<void>(deltaSteps); static_cast<void>(e); return {};
}

ToolOverlayState AiSelectTool::overlay() const {
  ToolOverlayState state;
  if (m_inputMode == InputMode::RotoBrush) {
    if (!m_rotoStrokes.empty() || m_strokeActive) {
      state.hasRotoStrokes   = true;
      state.rotoStrokes      = m_rotoStrokes;
      state.rotoActiveStroke = m_activeStroke;
      state.rotoActiveFg     = m_paintFg;
      state.rotoBrushRadius  = m_brushRadius;
    }
  } else {
    if (!m_positivePoints.empty()) {
      state.hasPolygon    = true;
      state.polygonClosed = false;
      state.polygonPoints = m_positivePoints;
    }
  }
  return state;
}

} // namespace core
