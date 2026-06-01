#include "core/tools/FillTool.h"

#include <algorithm>
#include <vector>

#include "core/selection/SelectionMask.h"

namespace core {

bool FillTool::isSameColor(const Color& a, const Color& b) noexcept {
  return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
int FillTool::colorDistance(const Color& a, const Color& b) noexcept {
  const int dr = std::abs(static_cast<int>(a.r) - static_cast<int>(b.r));
  const int dg = std::abs(static_cast<int>(a.g) - static_cast<int>(b.g));
  const int db = std::abs(static_cast<int>(a.b) - static_cast<int>(b.b));
  const int da = std::abs(static_cast<int>(a.a) - static_cast<int>(b.a));
  return std::max({dr, dg, db, da});
}

bool FillTool::matchesTarget(const PixelBuffer& source, const Color& target, int x, int y) const noexcept {
  if (!source.inBounds(x, y)) {
    return false;
  }
  return colorDistance(source.pixel(x, y), target) <= m_settings.threshold;
}

bool FillTool::hasBridge(const PixelBuffer& source, const Color& target, int x, int y) const noexcept {
  if (m_settings.gapClose <= 0) {
    return false;
  }
  static constexpr Point kDirs[] = {
      Point {1, 0}, Point {-1, 0}, Point {0, 1}, Point {0, -1}};
  for (const Point& dir : kDirs) {
    for (int d = 1; d <= m_settings.gapClose; ++d) {
      const int nx = x + dir.x * d;
      const int ny = y + dir.y * d;
      if (!source.inBounds(nx, ny)) {
        break;
      }
      if (matchesTarget(source, target, nx, ny)) {
        return true;
      }
    }
  }
  return false;
}

ToolResult FillTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  Layer* active = context.document.activeLayer();
  if (active == nullptr || active->kind() != LayerKind::Raster || active->locked()) {
    return {};
  }

  const PixelBuffer* source = &active->buffer();
  if (m_settings.referAllLayers) {
    source = &context.composited;
  }

  PixelBuffer& buffer = active->buffer();
  if (!source->inBounds(event.point.x, event.point.y)) {
    return {};
  }
  const SelectionMask& selection = context.document.selection();
  const bool hasSelection = selection.hasSelection();
  if (hasSelection && !selection.contains(event.point.x, event.point.y)) {
    return {};
  }

  const Color target = source->pixel(event.point.x, event.point.y);
  // eraseMode または描画色が完全透明なら消去塗りつぶし
  const bool isErase = m_settings.eraseMode || context.currentColor.a == 0;
  const Color replacement = isErase ? Color::Transparent() : context.currentColor;
  if (!isErase && m_settings.threshold == 0 && isSameColor(target, replacement)) {
    return {};
  }

  const int width = buffer.width();
  const int height = buffer.height();
  std::vector<std::uint8_t> fillMask(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0U);
  auto idx = [width](int x, int y) -> std::size_t {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
  };

  if (m_settings.contiguous) {
    std::vector<Point> stack;
    stack.reserve(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) / 8 + 1);
    stack.push_back(event.point);

    while (!stack.empty()) {
      const Point p = stack.back();
      stack.pop_back();

      if (!source->inBounds(p.x, p.y)) {
        continue;
      }
      const std::size_t i = idx(p.x, p.y);
      if (fillMask[i] != 0U) {
        continue;
      }
      if (hasSelection && !selection.contains(p.x, p.y)) {
        continue;
      }
      // 消去モード: 有色ピクセル（alpha>0）を隣接フラッドフィル
      if (isErase) {
        if (source->pixel(p.x, p.y).a == 0) {
          continue;
        }
      } else if (!matchesTarget(*source, target, p.x, p.y) && !hasBridge(*source, target, p.x, p.y)) {
        continue;
      }

      fillMask[i] = 1U;
      stack.push_back(Point {p.x + 1, p.y});
      stack.push_back(Point {p.x - 1, p.y});
      stack.push_back(Point {p.x, p.y + 1});
      stack.push_back(Point {p.x, p.y - 1});
    }
  } else {
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        if (hasSelection && !selection.contains(x, y)) {
          continue;
        }
        // 消去モード非隣接: 選択範囲内のすべての有色ピクセルを対象
        if (isErase) {
          if (source->pixel(x, y).a > 0) {
            fillMask[idx(x, y)] = 1U;
          }
        } else if (matchesTarget(*source, target, x, y)) {
          fillMask[idx(x, y)] = 1U;
        }
      }
    }
  }

  bool changed = false;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (fillMask[idx(x, y)] == 0U) {
        continue;
      }
      const Color before = buffer.pixel(x, y);
      Color output = replacement;
      if (isErase) {
        // 消去モード: alphaLock を無視して強制透明化
        output = Color::Transparent();
      } else {
        if (active->alphaLocked() && before.a == 0) {
          continue;
        }
        if (active->alphaLocked()) {
          output.a = before.a;
        }
      }
      if (!isSameColor(before, output)) {
        buffer.setPixel(x, y, output);
        changed = true;
      }
    }
  }

  if (!changed) {
    return {};
  }
  ToolResult result;
  result.pixelsChanged = true;
  result.dirtyRect = Rect {0, 0, buffer.width(), buffer.height()};
  return result;
}

ToolResult FillTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(event);
  return {};
}

ToolResult FillTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(event);
  return {};
}

ToolResult FillTool::onCancel(ToolContext& context) {
  static_cast<void>(context);
  return {};
}

ToolResult FillTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(deltaSteps);
  static_cast<void>(event);
  return {};
}

} // namespace core
