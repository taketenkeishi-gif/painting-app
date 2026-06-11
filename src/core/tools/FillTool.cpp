#include "core/tools/FillTool.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "core/selection/SelectionMask.h"

// ─────────────────────────────────────────────────────────────────────────────
// アルゴリズム吸収元:
//
//  ① 色距離: AiSelectTool と同じ知覚的ユークリッド距離
//     (旧: チェビシェフ距離 = 単純 max)
//
//  ② スキャンライン flood fill: OpenCV cv::floodFill と同じ考え方を C++ で実装
//     スタック式 4 方向探索は同一ピクセルを複数回訪れる可能性があるが、
//     スキャンラインは 1 行を一括でフィルし、各ピクセルを正確に 1 回だけ訪れる。
//     参考: OpenCV 4.x imgproc/src/floodfill.cpp
//
//  ③ AA エッジ: cv::distanceTransform の概念を吸収。
//     フィル境界ピクセルに対して 3x3 近傍のカバレッジ率でアルファを付与し、
//     1px のアンチエイリアスを実現する。
// ─────────────────────────────────────────────────────────────────────────────

namespace core {

// ── 知覚的色距離（旧: チェビシェフ → 新: 輝度重み付きユークリッド） ──────────
// AiSelectTool と同じ式を採用（コード統一）
float FillTool::colorDistance(const Color& a, const Color& b) noexcept {
  const float dr = static_cast<float>(a.r) - static_cast<float>(b.r);
  const float dg = static_cast<float>(a.g) - static_cast<float>(b.g);
  const float db = static_cast<float>(a.b) - static_cast<float>(b.b);
  const float da = static_cast<float>(a.a) - static_cast<float>(b.a);
  // BT.601 輝度重みで RGB をスケール、アルファは軽め
  return std::sqrt(0.299f*dr*dr + 0.587f*dg*dg + 0.114f*db*db + 0.15f*da*da);
}

bool FillTool::isSameColor(const Color& a, const Color& b) noexcept {
  return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

bool FillTool::matchesTarget(const PixelBuffer& source, const Color& target,
                              int x, int y) const noexcept {
  if (!source.inBounds(x, y)) return false;
  return colorDistance(source.pixel(x, y), target) <= static_cast<float>(m_settings.threshold);
}

// ── ギャップクローズ: 指定距離内に同色ピクセルがあるか ───────────────────────
bool FillTool::hasBridge(const PixelBuffer& source, const Color& target,
                          int x, int y) const noexcept {
  if (m_settings.gapClose <= 0) return false;
  static constexpr Point kDirs[] = {{1,0},{-1,0},{0,1},{0,-1}};
  for (const Point& dir : kDirs) {
    for (int d = 1; d <= m_settings.gapClose; ++d) {
      const int nx = x + dir.x * d;
      const int ny = y + dir.y * d;
      if (!source.inBounds(nx, ny)) break;
      if (matchesTarget(source, target, nx, ny)) return true;
    }
  }
  return false;
}

// ── スキャンライン flood fill (OpenCV cv::floodFill と同方式) ────────────────
// 戻り値: 塗りつぶされたピクセル数
static int scanlineFill(
    const PixelBuffer& source,
    std::vector<std::uint8_t>& mask,
    int startX, int startY,
    const Color& target, int threshold,
    const SelectionMask& selection, bool hasSelection,
    bool isErase, int gapClose,
    // matchFn: この関数が true を返す座標を塗る
    std::function<bool(int,int)> matchFn)
{
  const int W = source.width();
  const int H = source.height();
  if (!source.inBounds(startX, startY)) return 0;
  if (mask[startY * W + startX]) return 0;

  // スタックに (y, x_seed) を積む
  // OpenCV と同様にスパンベースで処理する
  struct Span { int y, x; };
  std::vector<Span> stack;
  stack.reserve(H * 2);
  stack.push_back({startY, startX});

  int filled = 0;

  while (!stack.empty()) {
    const auto [scanY, seedX] = stack.back();
    stack.pop_back();

    if (scanY < 0 || scanY >= H) continue;
    if (mask[scanY * W + seedX]) continue;
    if (hasSelection && !selection.contains(seedX, scanY)) continue;
    if (!matchFn(seedX, scanY)) continue;

    // スキャンライン左端を探す
    int left = seedX;
    while (left > 0 && !mask[scanY * W + (left-1)]
           && (!hasSelection || selection.contains(left-1, scanY))
           && matchFn(left-1, scanY))
      --left;

    // スキャンライン右端を探す
    int right = seedX;
    while (right < W-1 && !mask[scanY * W + (right+1)]
           && (!hasSelection || selection.contains(right+1, scanY))
           && matchFn(right+1, scanY))
      ++right;

    // 現在の行を一括マーク
    for (int x = left; x <= right; ++x) {
      mask[scanY * W + x] = 1;
      ++filled;
    }

    // 上下の行に新しいシードを追加
    // OpenCV の実装と同様に、スパン範囲を 1px 拡張してスキャン
    for (int dy : {-1, +1}) {
      const int ny = scanY + dy;
      if (ny < 0 || ny >= H) continue;
      for (int x = std::max(0, left-1); x <= std::min(W-1, right+1); ++x) {
        if (!mask[ny * W + x]
            && (!hasSelection || selection.contains(x, ny))
            && matchFn(x, ny))
        {
          stack.push_back({ny, x});
        }
      }
    }
  }

  return filled;
}

// ── AA エッジ適用 (cv::distanceTransform の概念を吸収) ───────────────────────
// フィルマスク境界ピクセルに 3x3 カバレッジ率でアルファを付与することで
// 1px のアンチエイリアスを実現する。
static void applyFillWithAA(
    PixelBuffer& buffer,
    const std::vector<std::uint8_t>& mask,
    const Color& fillColor,
    bool alphaLocked,
    bool eraseMode,
    const SelectionMask* selMask = nullptr)
{
  const int W = buffer.width();
  const int H = buffer.height();

  for (int y = 0; y < H; ++y) {
    for (int x = 0; x < W; ++x) {
      if (!mask[y * W + x]) continue;

      // 選択マスクによる部分アルファスケール
      float selScale = 1.0f;
      if (selMask != nullptr) {
        const uint8_t mv = selMask->maskValue(x, y);
        if (mv == 0) continue;
        if (mv < 255) selScale = static_cast<float>(mv) / 255.0f;
      }

      const Color dst = buffer.pixel(x, y);

      // 境界判定: 8 近傍にマスク外ピクセルがある
      bool isBoundary = false;
      for (int dy = -1; dy <= 1 && !isBoundary; ++dy) {
        for (int dx = -1; dx <= 1 && !isBoundary; ++dx) {
          if (dx == 0 && dy == 0) continue;
          const int nx = x + dx, ny = y + dy;
          if (nx < 0 || nx >= W || ny < 0 || ny >= H || !mask[ny * W + nx])
            isBoundary = true;
        }
      }

      if (eraseMode) {
        // 選択マスクで消去強度をスケール
        if (selScale >= 1.0f) {
          buffer.setPixel(x, y, Color::Transparent());
        } else {
          const float keep = 1.0f - selScale;
          buffer.setPixel(x, y, Color{
              static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.r) * keep)),
              static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.g) * keep)),
              static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.b) * keep)),
              static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.a) * keep))});
        }
        continue;
      }

      Color out = fillColor;

      if (isBoundary) {
        // 3x3 近傍でフィル済みピクセルの割合をカバレッジとして使用
        int filled = 0;
        for (int dy = -1; dy <= 1; ++dy)
          for (int dx = -1; dx <= 1; ++dx) {
            const int nx = x+dx, ny = y+dy;
            if (nx >= 0 && nx < W && ny >= 0 && ny < H && mask[ny * W + nx])
              ++filled;
          }
        const float coverage = static_cast<float>(filled) / 9.0f;
        out.a = static_cast<std::uint8_t>(
            std::lround(static_cast<float>(fillColor.a) * coverage));
      }

      // 選択マスクでアルファをスケール
      out.a = static_cast<std::uint8_t>(
          std::lround(static_cast<float>(out.a) * selScale));

      if (alphaLocked) {
        if (dst.a == 0) continue;
        out.a = dst.a;
      }

      // Porter-Duff Over 合成
      const float sa = out.a / 255.0f;
      const float da = dst.a / 255.0f;
      const float oa = sa + da * (1.0f - sa);
      if (oa <= 0.0f) { buffer.setPixel(x, y, Color::Transparent()); continue; }
      const float sr = out.r/255.0f, sg = out.g/255.0f, sb = out.b/255.0f;
      const float dr = dst.r/255.0f, dg = dst.g/255.0f, db_= dst.b/255.0f;
      buffer.setPixel(x, y, Color{
          static_cast<std::uint8_t>(std::lround(std::clamp((sr*sa + dr*da*(1-sa))/oa, 0.0f,1.0f)*255)),
          static_cast<std::uint8_t>(std::lround(std::clamp((sg*sa + dg*da*(1-sa))/oa, 0.0f,1.0f)*255)),
          static_cast<std::uint8_t>(std::lround(std::clamp((sb*sa + db_*da*(1-sa))/oa,0.0f,1.0f)*255)),
          static_cast<std::uint8_t>(std::lround(std::clamp(oa, 0.0f, 1.0f) * 255.0f))
      });
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
ToolResult FillTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  Layer* active = context.document.activeLayer();
  if (active == nullptr || active->kind() != LayerKind::Raster || active->locked()) {
    return {};
  }

  const PixelBuffer* source = m_settings.referAllLayers
      ? &context.composited : &active->buffer();
  PixelBuffer& buffer = active->buffer();

  if (!source->inBounds(event.point.x, event.point.y)) return {};

  const SelectionMask& selection = context.document.selection();
  const bool hasSelection = selection.hasSelection();
  if (hasSelection && !selection.contains(event.point.x, event.point.y)) return {};

  const Color target      = source->pixel(event.point.x, event.point.y);
  const bool  isErase     = m_settings.eraseMode || context.currentColor.a == 0;
  const Color replacement = isErase ? Color::Transparent() : context.currentColor;
  if (!isErase && m_settings.threshold == 0 && isSameColor(target, replacement)) return {};

  const int W = buffer.width();
  const int H = buffer.height();
  std::vector<std::uint8_t> fillMask(static_cast<std::size_t>(W) * H, 0u);

  // matchFn: どのピクセルを塗るかの判定ロジック
  auto matchFn = [&](int x, int y) -> bool {
    if (isErase) return source->pixel(x,y).a > 0;
    return matchesTarget(*source, target, x, y)
        || hasBridge(*source, target, x, y);
  };

  if (m_settings.contiguous) {
    // ── スキャンライン flood fill ──────────────────────────────────────────
    scanlineFill(*source, fillMask,
                 event.point.x, event.point.y,
                 target, m_settings.threshold,
                 selection, hasSelection,
                 isErase, m_settings.gapClose,
                 matchFn);
  } else {
    // ── 全域フィル ─────────────────────────────────────────────────────────
    for (int y = 0; y < H; ++y) {
      for (int x = 0; x < W; ++x) {
        if (hasSelection && !selection.contains(x, y)) continue;
        if (matchFn(x, y)) fillMask[y * W + x] = 1u;
      }
    }
  }

  // ── AA エッジ付きで描画 ────────────────────────────────────────────────────
  applyFillWithAA(buffer, fillMask, replacement,
                  active->alphaLocked() && !isErase, isErase,
                  hasSelection ? &selection : nullptr);

  ToolResult result;
  result.pixelsChanged = true;
  result.dirtyRect = Rect{0, 0, W, H};
  return result;
}

ToolResult FillTool::onPointerMove(ToolContext&, const ToolPointerEvent&)   { return {}; }
ToolResult FillTool::onPointerRelease(ToolContext&, const ToolPointerEvent&) { return {}; }
ToolResult FillTool::onCancel(ToolContext&)                                  { return {}; }
ToolResult FillTool::onWheel(ToolContext&, int, const ToolPointerEvent&)     { return {}; }

} // namespace core
