#include "core/tools/RectSelectionTool.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace core {

ToolResult RectSelectionTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  if (m_mode == Mode::AutoSelect) {
    ToolResult result;
    result.selectionChanged = applyAutoSelect(context, event.point);
    result.viewportChanged = true;
    return result;
  }

  m_selecting = true;
  m_start = event.point;
  m_current = event.point;
  if (m_mode == Mode::Lasso) {
    m_lassoPoints.clear();
    m_lassoPoints.push_back(event.point);
  }
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult RectSelectionTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  if (!m_selecting) {
    return {};
  }
  m_current = event.point;
  if (m_mode == Mode::Lasso) {
    if (m_lassoPoints.empty()) {
      m_lassoPoints.push_back(event.point);
    } else {
      const Point last = m_lassoPoints.back();
      if (std::abs(last.x - event.point.x) + std::abs(last.y - event.point.y) >= 1) {
        m_lassoPoints.push_back(event.point);
      }
    }
  }
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult RectSelectionTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  if (!m_selecting) {
    return {};
  }
  m_selecting = false;
  m_current = event.point;
  bool changed = false;
  if (m_mode == Mode::Lasso) {
    if (m_lassoPoints.empty() || (m_lassoPoints.back().x != event.point.x || m_lassoPoints.back().y != event.point.y)) {
      m_lassoPoints.push_back(event.point);
    }
    changed = applyLassoSelection(context);
  } else {
    const Rect rect = normalizeRect(m_start, m_current);
    changed = context.document.selection().setRect(rect);
  }
  ToolResult result;
  result.selectionChanged = changed;
  result.viewportChanged = true;
  return result;
}

ToolResult RectSelectionTool::onCancel(ToolContext& context) {
  static_cast<void>(context);
  if (!m_selecting) {
    return {};
  }
  m_selecting = false;
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult RectSelectionTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(deltaSteps);
  static_cast<void>(event);
  return {};
}

ToolOverlayState RectSelectionTool::overlay() const {
  ToolOverlayState state;
  if (!m_selecting) {
    return state;
  }
  if (m_mode == Mode::Lasso) {
    state.hasLine = true;
    state.lineStart = m_start;
    state.lineEnd = m_current;
  } else if (m_mode == Mode::Rectangle) {
    state.hasRect = true;
    state.rect = normalizeRect(m_start, m_current);
  }
  return state;
}

Rect RectSelectionTool::normalizeRect(const Point& a, const Point& b) {
  const int left = std::min(a.x, b.x);
  const int top = std::min(a.y, b.y);
  const int right = std::max(a.x, b.x);
  const int bottom = std::max(a.y, b.y);
  return Rect {left, top, right - left + 1, bottom - top + 1};
}

int RectSelectionTool::colorDistance(const Color& a, const Color& b) noexcept {
  const int dr = std::abs(static_cast<int>(a.r) - static_cast<int>(b.r));
  const int dg = std::abs(static_cast<int>(a.g) - static_cast<int>(b.g));
  const int db = std::abs(static_cast<int>(a.b) - static_cast<int>(b.b));
  const int da = std::abs(static_cast<int>(a.a) - static_cast<int>(b.a));
  return std::max({dr, dg, db, da});
}

bool RectSelectionTool::applyAutoSelect(ToolContext& context, const Point& seed) {
  const int width = context.composited.width();
  const int height = context.composited.height();
  if (width <= 0 || height <= 0) {
    return false;
  }
  if (seed.x < 0 || seed.y < 0 || seed.x >= width || seed.y >= height) {
    return false;
  }

  const Layer* active = context.document.activeLayer();
  const PixelBuffer* source = &context.composited;
  if (!m_autoSelectReferAllLayers && active != nullptr && active->kind() == LayerKind::Raster) {
    source = &active->buffer();
  }
  const Color seedColor = source->pixel(seed.x, seed.y);
  const std::size_t total = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  std::vector<std::uint8_t> mask(total, 0U);

  auto idx = [width](int x, int y) -> std::size_t {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
  };
  auto inBounds = [width, height](int x, int y) -> bool {
    return x >= 0 && y >= 0 && x < width && y < height;
  };
  auto matches = [&](int x, int y) -> bool {
    return colorDistance(source->pixel(x, y), seedColor) <= m_autoSelectThreshold;
  };

  if (m_autoSelectContiguous) {
    std::vector<Point> stack;
    stack.reserve(total / 8 + 1);
    stack.push_back(seed);
    while (!stack.empty()) {
      const Point p = stack.back();
      stack.pop_back();
      if (!inBounds(p.x, p.y)) {
        continue;
      }
      const std::size_t i = idx(p.x, p.y);
      if (mask[i] != 0U) {
        continue;
      }
      if (!matches(p.x, p.y)) {
        continue;
      }
      mask[i] = 1U;
      stack.push_back(Point {p.x + 1, p.y});
      stack.push_back(Point {p.x - 1, p.y});
      stack.push_back(Point {p.x, p.y + 1});
      stack.push_back(Point {p.x, p.y - 1});
    }
  } else {
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        if (matches(x, y)) {
          mask[idx(x, y)] = 1U;
        }
      }
    }
  }

  return context.document.selection().setPixels(mask);
}

bool RectSelectionTool::pointInPolygon(const std::vector<Point>& polygon, int x, int y) {
  bool inside = false;
  for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
    const Point& a = polygon[i];
    const Point& b = polygon[j];
    const bool intersects = ((a.y > y) != (b.y > y)) &&
                            (x < (b.x - a.x) * static_cast<double>(y - a.y) / static_cast<double>(b.y - a.y) + a.x);
    if (intersects) {
      inside = !inside;
    }
  }
  return inside;
}

bool RectSelectionTool::applyLassoSelection(ToolContext& context) {
  const int width = context.document.canvasSize().width;
  const int height = context.document.canvasSize().height;
  if (width <= 0 || height <= 0 || m_lassoPoints.size() < 3) {
    return false;
  }

  Rect bounds = normalizeRect(m_lassoPoints.front(), m_lassoPoints.front());
  for (const Point& p : m_lassoPoints) {
    const Rect candidate = normalizeRect(Point {bounds.x, bounds.y}, p);
    const int minX = std::min(bounds.x, candidate.x);
    const int minY = std::min(bounds.y, candidate.y);
    const int maxX = std::max(bounds.x + bounds.width - 1, candidate.x + candidate.width - 1);
    const int maxY = std::max(bounds.y + bounds.height - 1, candidate.y + candidate.height - 1);
    bounds = Rect {minX, minY, maxX - minX + 1, maxY - minY + 1};
  }
  bounds.x = std::clamp(bounds.x, 0, width - 1);
  bounds.y = std::clamp(bounds.y, 0, height - 1);
  bounds.width = std::clamp(bounds.width, 1, width - bounds.x);
  bounds.height = std::clamp(bounds.height, 1, height - bounds.y);

  std::vector<std::uint8_t> mask(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0U);
  for (int y = bounds.y; y < bounds.y + bounds.height; ++y) {
    for (int x = bounds.x; x < bounds.x + bounds.width; ++x) {
      if (pointInPolygon(m_lassoPoints, x, y)) {
        mask[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)] = 1U;
      }
    }
  }
  return context.document.selection().setPixels(mask);
}

} // namespace core
