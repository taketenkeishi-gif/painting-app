#include "core/tools/CurveTool.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace core {

namespace {

Rect curveDirtyRect(const std::vector<FPoint>& points, int size) {
  if (points.empty()) {
    return Rect {0, 0, 0, 0};
  }
  const int radius = std::max(1, size) / 2 + 2;
  int minX = static_cast<int>(points[0].x);
  int maxX = minX;
  int minY = static_cast<int>(points[0].y);
  int maxY = minY;
  for (const auto& pt : points) {
    minX = std::min(minX, static_cast<int>(pt.x));
    maxX = std::max(maxX, static_cast<int>(pt.x));
    minY = std::min(minY, static_cast<int>(pt.y));
    maxY = std::max(maxY, static_cast<int>(pt.y));
  }
  return Rect {minX - radius, minY - radius, maxX - minX + 1 + 2 * radius, maxY - minY + 1 + 2 * radius};
}

Rect fullLayerRect(const Layer& layer) {
  return Rect {0, 0, layer.buffer().width(), layer.buffer().height()};
}

} // namespace

ToolResult CurveTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  m_drawing = true;
  m_start = event.point;
  m_current = event.point;
  m_handle1 = FPoint {0.0f, 0.0f};
  m_handle2 = FPoint {0.0f, 0.0f};
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult CurveTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  if (!m_drawing) {
    return {};
  }

  Point snapped = snappedPoint(m_start, event.point, event.shift);
  m_current = snapped;

  const float dx = static_cast<float>(snapped.x - m_start.x);
  const float dy = static_cast<float>(snapped.y - m_start.y);
  const float distance = std::hypot(dx, dy);

  if (distance > 0.001f) {
    const float ux = dx / distance;
    const float uy = dy / distance;
    const float handleDist = distance * 0.333f;
    m_handle1 = FPoint {static_cast<float>(m_start.x) + ux * handleDist, static_cast<float>(m_start.y) + uy * handleDist};
    m_handle2 = FPoint {static_cast<float>(snapped.x) - ux * handleDist, static_cast<float>(snapped.y) - uy * handleDist};
  }

  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult CurveTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  if (!m_drawing) {
    return {};
  }
  m_drawing = false;

  Point snapped = snappedPoint(m_start, event.point, event.shift);
  m_current = snapped;

  Layer* active = context.document.activeLayer();
  if (active == nullptr) {
    return {};
  }
  if (active->locked()) {
    return {};
  }

  if (active->kind() != LayerKind::Vector) {
    return {};
  }

  const auto curvePoints = generateBezierPoints(m_start, m_handle1, m_handle2, m_current);
  addVectorBezierCurve(*active, curvePoints, context.currentColor, context.brushSize);

  ToolResult result;
  result.pixelsChanged = true;
  result.viewportChanged = true;
  result.dirtyRect = fullLayerRect(*active);
  return result;
}

Point CurveTool::snappedPoint(const Point& start, const Point& rawEnd, bool shiftConstraint) const {
  const int snapAngle = shiftConstraint ? 45 : m_snapAngleDegrees;
  if (snapAngle <= 0 || snapAngle >= 180) {
    return rawEnd;
  }
  const float dx = static_cast<float>(rawEnd.x - start.x);
  const float dy = static_cast<float>(rawEnd.y - start.y);
  if (std::abs(dx) < 0.001F && std::abs(dy) < 0.001F) {
    return rawEnd;
  }
  const float distance = std::hypot(dx, dy);
  const float angle = std::atan2(dy, dx);
  constexpr float kPi = 3.14159265358979323846F;
  const float step = static_cast<float>(snapAngle) * kPi / 180.0F;
  const float snapped = std::round(angle / step) * step;
  return Point {
      static_cast<int>(std::lround(static_cast<float>(start.x) + std::cos(snapped) * distance)),
      static_cast<int>(std::lround(static_cast<float>(start.y) + std::sin(snapped) * distance))};
}

ToolResult CurveTool::onCancel(ToolContext& context) {
  static_cast<void>(context);
  if (!m_drawing) {
    return {};
  }
  m_drawing = false;
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult CurveTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(deltaSteps);
  static_cast<void>(event);
  return {};
}

ToolOverlayState CurveTool::overlay() const {
  ToolOverlayState state;
  if (!m_drawing) {
    return state;
  }
  state.hasLine = true;
  state.lineStart = m_start;
  state.lineEnd = m_current;
  return state;
}

std::vector<FPoint> CurveTool::generateBezierPoints(
    const Point& p0,
    const FPoint& p1,
    const FPoint& p2,
    const Point& p3) const {
  std::vector<FPoint> result;

  const float fp0x = static_cast<float>(p0.x);
  const float fp0y = static_cast<float>(p0.y);
  const float fp3x = static_cast<float>(p3.x);
  const float fp3y = static_cast<float>(p3.y);

  constexpr int kSteps = 64;
  result.reserve(kSteps + 1);

  for (int i = 0; i <= kSteps; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(kSteps);
    const float t1 = 1.0f - t;

    const float t1_3 = t1 * t1 * t1;
    const float t1_2_t = 3.0f * t1 * t1 * t;
    const float t1_t_2 = 3.0f * t1 * t * t;
    const float t_3 = t * t * t;

    const float x = t1_3 * fp0x + t1_2_t * p1.x + t1_t_2 * p2.x + t_3 * fp3x;
    const float y = t1_3 * fp0y + t1_2_t * p1.y + t1_t_2 * p2.y + t_3 * fp3y;

    result.push_back(FPoint {x, y});
  }

  return result;
}

void CurveTool::addVectorBezierCurve(
    Layer& layer,
    const std::vector<FPoint>& curvePoints,
    const Color& color,
    int size) const {
  if (curvePoints.empty()) {
    return;
  }

  VectorPath path;
  path.points = curvePoints;
  path.color = color;
  path.width = std::max(1, size);
  path.opacity = static_cast<float>(color.a) / 255.0F;
  layer.addVectorPath(std::move(path));
}

} // namespace core
