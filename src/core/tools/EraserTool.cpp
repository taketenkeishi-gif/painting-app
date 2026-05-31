#include "core/tools/EraserTool.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace core {

namespace {

Rect strokeDirtyRect(const Point& from, const Point& to, int size) {
  const int radius = std::max(1, size) / 2 + 2;
  const int minX = std::min(from.x, to.x) - radius;
  const int minY = std::min(from.y, to.y) - radius;
  const int maxX = std::max(from.x, to.x) + radius;
  const int maxY = std::max(from.y, to.y) + radius;
  return Rect {minX, minY, maxX - minX + 1, maxY - minY + 1};
}

Rect fullLayerRect(const Layer& layer) {
  return Rect {0, 0, layer.buffer().width(), layer.buffer().height()};
}

FPoint interpolatePoint(const FPoint& a, const FPoint& b, float t) {
  return FPoint {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

FPoint samplePathPoint(const VectorPath& path, float position) {
  if (path.points.empty()) {
    return FPoint {0.0F, 0.0F};
  }
  if (path.points.size() == 1) {
    return path.points.front();
  }
  const float clamped = std::clamp(position, 0.0F, static_cast<float>(path.points.size() - 1));
  const int index = static_cast<int>(std::floor(clamped));
  const int next = std::min(index + 1, static_cast<int>(path.points.size() - 1));
  const float t = clamped - static_cast<float>(index);
  return interpolatePoint(path.points[static_cast<std::size_t>(index)], path.points[static_cast<std::size_t>(next)], t);
}

std::vector<FPoint> slicePath(const VectorPath& path, float startPos, float endPos) {
  std::vector<FPoint> points;
  if (path.points.size() < 2) {
    return points;
  }
  if (endPos < startPos) {
    std::swap(startPos, endPos);
  }
  const float maxPos = static_cast<float>(path.points.size() - 1);
  startPos = std::clamp(startPos, 0.0F, maxPos);
  endPos = std::clamp(endPos, 0.0F, maxPos);
  points.push_back(samplePathPoint(path, startPos));
  const int first = static_cast<int>(std::ceil(startPos));
  const int last = static_cast<int>(std::floor(endPos));
  for (int i = first; i <= last; ++i) {
    if (i <= 0 || i >= static_cast<int>(path.points.size() - 1)) {
      continue;
    }
    points.push_back(path.points[static_cast<std::size_t>(i)]);
  }
  points.push_back(samplePathPoint(path, endPos));
  if (points.size() >= 2 &&
      points.front().x == points.back().x &&
      points.front().y == points.back().y) {
    points.pop_back();
  }
  return points;
}

bool segmentIntersection(
    const FPoint& a0,
    const FPoint& a1,
    const FPoint& b0,
    const FPoint& b1,
    float& outT,
    float& outU) {
  const float ax = a0.x;
  const float ay = a0.y;
  const float bx = a1.x;
  const float by = a1.y;
  const float cx = b0.x;
  const float cy = b0.y;
  const float dx = b1.x;
  const float dy = b1.y;

  const float rX = bx - ax;
  const float rY = by - ay;
  const float sX = dx - cx;
  const float sY = dy - cy;
  const float denom = rX * sY - rY * sX;
  if (std::abs(denom) < 0.0001F) {
    return false;
  }
  const float qpx = cx - ax;
  const float qpy = cy - ay;
  const float t = (qpx * sY - qpy * sX) / denom;
  const float u = (qpx * rY - qpy * rX) / denom;
  if (t < 0.0F || t > 1.0F || u < 0.0F || u > 1.0F) {
    return false;
  }
  outT = t;
  outU = u;
  return true;
}

std::vector<float> collectIntersectionPositions(
    const VectorPath& target,
    const std::vector<VectorPath>& allPaths,
    std::size_t targetIndex) {
  std::vector<float> intersections;
  if (target.points.size() < 2) {
    return intersections;
  }
  for (std::size_t i = 1; i < target.points.size(); ++i) {
    const FPoint& a0 = target.points[i - 1];
    const FPoint& a1 = target.points[i];
    for (std::size_t p = 0; p < allPaths.size(); ++p) {
      if (p == targetIndex) {
        continue;
      }
      const VectorPath& other = allPaths[p];
      if (other.points.size() < 2) {
        continue;
      }
      for (std::size_t j = 1; j < other.points.size(); ++j) {
        float t = 0.0F;
        float u = 0.0F;
        if (!segmentIntersection(a0, a1, other.points[j - 1], other.points[j], t, u)) {
          continue;
        }
        intersections.push_back(static_cast<float>(i - 1) + t);
      }
    }
  }
  std::sort(intersections.begin(), intersections.end());
  intersections.erase(
      std::unique(intersections.begin(), intersections.end(), [](float lhs, float rhs) {
        return std::abs(lhs - rhs) < 0.01F;
      }),
      intersections.end());
  return intersections;
}

std::optional<float> nearestLower(const std::vector<float>& values, float pivot) {
  std::optional<float> out;
  for (float value : values) {
    if (value <= pivot) {
      out = value;
    } else {
      break;
    }
  }
  return out;
}

std::optional<float> nearestUpper(const std::vector<float>& values, float pivot) {
  for (float value : values) {
    if (value >= pivot) {
      return value;
    }
  }
  return std::nullopt;
}

} // namespace

ToolResult EraserTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  Layer* active = context.document.activeLayer();
  if (active == nullptr || active->kind() == LayerKind::Folder || active->locked()) {
    return {};
  }
  if (active->kind() == LayerKind::Raster && active->alphaLocked()) {
    return {};
  }

  m_erasing = true;
  m_lastPoint = event.point;
  if (active->kind() == LayerKind::Vector) {
    eraseVectorStroke(*active, m_lastPoint, m_lastPoint);
  } else {
    eraseStroke(*active, m_lastPoint, m_lastPoint);
  }
  ToolResult result;
  result.pixelsChanged = true;
  result.dirtyRect = active->kind() == LayerKind::Vector ? fullLayerRect(*active)
                                                          : strokeDirtyRect(m_lastPoint, m_lastPoint, m_size);
  return result;
}

ToolResult EraserTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  if (!m_erasing) {
    return {};
  }

  Layer* active = context.document.activeLayer();
  if (active == nullptr || active->kind() == LayerKind::Folder || active->locked()) {
    m_erasing = false;
    return {};
  }
  if (active->kind() == LayerKind::Raster && active->alphaLocked()) {
    m_erasing = false;
    return {};
  }

  const Point previous = m_lastPoint;
  const Point stabilizedPoint = applyStabilization(m_lastPoint, event.point);
  if (active->kind() == LayerKind::Vector) {
    eraseVectorStroke(*active, previous, stabilizedPoint);
  } else {
    eraseStroke(*active, previous, stabilizedPoint);
  }
  m_lastPoint = stabilizedPoint;
  ToolResult result;
  result.pixelsChanged = true;
  result.dirtyRect = active->kind() == LayerKind::Vector ? fullLayerRect(*active)
                                                          : strokeDirtyRect(previous, stabilizedPoint, m_size);
  return result;
}

ToolResult EraserTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  if (!m_erasing) {
    return {};
  }

  m_erasing = false;
  Layer* active = context.document.activeLayer();
  if (active == nullptr || active->kind() == LayerKind::Folder || active->locked()) {
    return {};
  }
  if (active->kind() == LayerKind::Raster && active->alphaLocked()) {
    return {};
  }

  const Point stabilizedPoint = applyStabilization(m_lastPoint, event.point);
  if (m_lastPoint.x == stabilizedPoint.x && m_lastPoint.y == stabilizedPoint.y &&
      (!m_postCorrection ||
       (stabilizedPoint.x == event.point.x && stabilizedPoint.y == event.point.y))) {
    return {};
  }

  if (active->kind() == LayerKind::Vector) {
    eraseVectorStroke(*active, m_lastPoint, stabilizedPoint);
  } else {
    eraseStroke(*active, m_lastPoint, stabilizedPoint);
  }
  Rect dirty = active->kind() == LayerKind::Vector ? fullLayerRect(*active)
                                                    : strokeDirtyRect(m_lastPoint, stabilizedPoint, m_size);
  if (m_postCorrection &&
      (stabilizedPoint.x != event.point.x || stabilizedPoint.y != event.point.y)) {
    if (active->kind() == LayerKind::Vector) {
      eraseVectorStroke(*active, stabilizedPoint, event.point);
      dirty = fullLayerRect(*active);
    } else {
      eraseStroke(*active, stabilizedPoint, event.point);
      const Rect correctionRect = strokeDirtyRect(stabilizedPoint, event.point, m_size);
      const int minX = std::min(dirty.x, correctionRect.x);
      const int minY = std::min(dirty.y, correctionRect.y);
      const int maxX = std::max(dirty.x + dirty.width, correctionRect.x + correctionRect.width);
      const int maxY = std::max(dirty.y + dirty.height, correctionRect.y + correctionRect.height);
      dirty = Rect {minX, minY, maxX - minX, maxY - minY};
    }
    m_lastPoint = event.point;
  } else {
    m_lastPoint = stabilizedPoint;
  }
  ToolResult result;
  result.pixelsChanged = true;
  result.dirtyRect = dirty;
  return result;
}

ToolResult EraserTool::onCancel(ToolContext& context) {
  static_cast<void>(context);
  m_erasing = false;
  return {};
}

ToolResult EraserTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(deltaSteps);
  static_cast<void>(event);
  return {};
}

Point EraserTool::applyStabilization(const Point& from, const Point& to) const {
  const float stabilization = std::clamp(m_stabilization, 0.0F, 1.0F);
  if (stabilization <= 0.001F) {
    return to;
  }

  float response = 1.0F - stabilization * 0.85F;
  if (m_velocityBasedCorrection) {
    const float distance = std::hypot(static_cast<float>(to.x - from.x), static_cast<float>(to.y - from.y));
    const float velocityFactor = std::clamp(1.0F - distance / 80.0F, 0.25F, 1.0F);
    response *= velocityFactor;
  }
  response = std::clamp(response, 0.05F, 1.0F);
  return Point {
      static_cast<int>(std::lround(static_cast<float>(from.x) + static_cast<float>(to.x - from.x) * response)),
      static_cast<int>(std::lround(static_cast<float>(from.y) + static_cast<float>(to.y - from.y) * response))};
}

void EraserTool::eraseStroke(Layer& layer, const Point& from, const Point& to) const {
  PixelBuffer& buffer = layer.buffer();
  const int radius = std::max(1, m_size) / 2;
  const float fRadius = static_cast<float>(m_size) * 0.5f;
  const int dx = to.x - from.x;
  const int dy = to.y - from.y;
  const float distance = std::hypot(static_cast<float>(dx), static_cast<float>(dy));
  const float spacingPixels = std::max(1.0F, m_spacing * fRadius * 2.0F);
  const int steps = std::max(1, static_cast<int>(std::ceil(distance / spacingPixels)));

  if (distance <= 0.001F) {
    if (m_shapeType == BrushShapeType::Square) {
      eraseSquare(buffer, from, radius);
    } else {
      eraseCircle(buffer, from, radius);
    }
    return;
  }

  for (int i = 0; i <= steps; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(steps);
    const Point p {
        static_cast<int>(std::lround(static_cast<float>(from.x) + static_cast<float>(dx) * t)),
        static_cast<int>(std::lround(static_cast<float>(from.y) + static_cast<float>(dy) * t))};
    if (m_shapeType == BrushShapeType::Square) {
      eraseSquare(buffer, p, radius);
    } else {
      eraseCircle(buffer, p, radius);
    }
  }
}

void EraserTool::eraseVectorStroke(Layer& layer, const Point& from, const Point& to) const {
  std::vector<VectorPath>& editablePaths = layer.vectorPaths();
  if (editablePaths.empty()) {
    return;
  }
  const std::vector<VectorPath> originalPaths = editablePaths;

  const int radius = std::max(1, m_size) / 2;
  const float fRadius2 = static_cast<float>(m_size) * 0.5f;
  const int dx = to.x - from.x;
  const int dy = to.y - from.y;
  const float distance = std::hypot(static_cast<float>(dx), static_cast<float>(dy));
  const float spacingPixels = std::max(1.0F, m_spacing * fRadius2 * 2.0F);
  const int steps = std::max(1, static_cast<int>(std::ceil(distance / spacingPixels)));

  std::vector<Point> stamps;
  stamps.reserve(static_cast<std::size_t>(steps) + 1);
  for (int i = 0; i <= steps; ++i) {
    const float t = steps == 0 ? 0.0F : static_cast<float>(i) / static_cast<float>(steps);
    stamps.push_back(Point {
        static_cast<int>(std::lround(static_cast<float>(from.x) + static_cast<float>(dx) * t)),
        static_cast<int>(std::lround(static_cast<float>(from.y) + static_cast<float>(dy) * t))});
  }

  const float radiusF = static_cast<float>(radius);
  const auto collectHitPositions = [&](const VectorPath& path) {
    std::vector<float> hits;
    if (path.points.empty()) {
      return hits;
    }
    const float widthRadius = static_cast<float>(std::max(1, path.width)) * 0.5F;
    for (const Point& stamp : stamps) {
      const FPoint stampF {static_cast<float>(stamp.x), static_cast<float>(stamp.y)};
      for (const FPoint& pt : path.points) {
        const float px = pt.x - stampF.x;
        const float py = pt.y - stampF.y;
        const float allowance = radiusF + widthRadius;
        if ((px * px + py * py) <= (allowance * allowance)) {
          hits.push_back(0.0F);
        }
      }
      for (std::size_t i = 1; i < path.points.size(); ++i) {
        const float allowance = radiusF + widthRadius;
        if (distancePointToSegment(stampF, path.points[i - 1], path.points[i]) > allowance) {
          continue;
        }
        const float ax = path.points[i - 1].x;
        const float ay = path.points[i - 1].y;
        const float bx = path.points[i].x;
        const float by = path.points[i].y;
        const float px = stampF.x;
        const float py = stampF.y;
        const float vx = bx - ax;
        const float vy = by - ay;
        const float lenSq = vx * vx + vy * vy;
        float t = 0.0F;
        if (lenSq > 0.0001F) {
          t = std::clamp(((px - ax) * vx + (py - ay) * vy) / lenSq, 0.0F, 1.0F);
        }
        hits.push_back(static_cast<float>(i - 1) + t);
      }
    }
    std::sort(hits.begin(), hits.end());
    return hits;
  };

  std::vector<VectorPath> result;
  result.reserve(originalPaths.size());

  for (std::size_t pathIndex = 0; pathIndex < originalPaths.size(); ++pathIndex) {
    const VectorPath& path = originalPaths[pathIndex];
    const std::vector<float> hitPositions = collectHitPositions(path);
    if (hitPositions.empty()) {
      result.push_back(path);
      continue;
    }
    if (path.points.size() < 2) {
      continue;
    }

    const float lastPos = static_cast<float>(path.points.size() - 1);
    const float hitPos = hitPositions[hitPositions.size() / 2];
    const std::vector<float> intersections = collectIntersectionPositions(path, originalPaths, pathIndex);

    auto appendSlice = [&](float startPos, float endPos) {
      std::vector<FPoint> points = slicePath(path, startPos, endPos);
      if (points.size() < 2) {
        return;
      }
      VectorPath kept = path;
      kept.points = std::move(points);
      result.push_back(std::move(kept));
    };

    if (m_vectorEraseMode == VectorEraseMode::TouchedOnly) {
      continue;
    }

    if (m_vectorEraseMode == VectorEraseMode::ToIntersection) {
      const std::optional<float> lower = nearestLower(intersections, hitPos);
      const std::optional<float> upper = nearestUpper(intersections, hitPos);
      if (lower.has_value() && upper.has_value() && std::abs(*upper - *lower) > 0.01F) {
        appendSlice(0.0F, *lower);
        appendSlice(*upper, lastPos);
        continue;
      }
      if (lower.has_value()) {
        appendSlice(0.0F, *lower);
        continue;
      }
      if (upper.has_value()) {
        appendSlice(*upper, lastPos);
        continue;
      }
      const float removeStart = std::clamp(hitPos - 0.55F, 0.0F, lastPos);
      const float removeEnd = std::clamp(hitPos + 0.55F, 0.0F, lastPos);
      appendSlice(0.0F, removeStart);
      appendSlice(removeEnd, lastPos);
      continue;
    }

    if (m_vectorEraseMode == VectorEraseMode::TrimOutside) {
      if (!m_vectorTrimOutside) {
        continue;
      }
      const float distanceToStart = std::abs(hitPos - 0.0F);
      const float distanceToEnd = std::abs(lastPos - hitPos);
      if (distanceToStart <= distanceToEnd) {
        const std::optional<float> lower = nearestLower(intersections, hitPos);
        const float cut = lower.value_or(hitPos);
        appendSlice(cut, lastPos);
      } else {
        const std::optional<float> upper = nearestUpper(intersections, hitPos);
        const float cut = upper.value_or(hitPos);
        appendSlice(0.0F, cut);
      }
      continue;
    }
  }

  editablePaths = std::move(result);
}

float EraserTool::distancePointToSegment(FPoint p, FPoint a, FPoint b) noexcept {
  const float ax = a.x;
  const float ay = a.y;
  const float bx = b.x;
  const float by = b.y;
  const float px = p.x;
  const float py = p.y;

  const float vx = bx - ax;
  const float vy = by - ay;
  const float wx = px - ax;
  const float wy = py - ay;
  const float lenSq = vx * vx + vy * vy;
  if (lenSq <= 0.0001F) {
    const float dx = px - ax;
    const float dy = py - ay;
    return std::sqrt(dx * dx + dy * dy);
  }

  const float t = std::clamp((wx * vx + wy * vy) / lenSq, 0.0F, 1.0F);
  const float cx = ax + vx * t;
  const float cy = ay + vy * t;
  const float dx = px - cx;
  const float dy = py - cy;
  return std::sqrt(dx * dx + dy * dy);
}

void EraserTool::eraseCircle(PixelBuffer& buffer, const Point& center, int radius) const {
  const int r2 = radius * radius;
  const float radiusF = static_cast<float>(std::max(1, radius));
  const float hardEdge = std::clamp(m_hardness, 0.0F, 1.0F);
  for (int y = center.y - radius; y <= center.y + radius; ++y) {
    for (int x = center.x - radius; x <= center.x + radius; ++x) {
      const int dx = x - center.x;
      const int dy = y - center.y;
      if ((dx * dx + dy * dy) <= r2) {
        const float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy)) / radiusF;
        if (distance > 1.0F) {
          continue;
        }
        float strength = 1.0F;
        if (m_antiAlias) {
          if (hardEdge < 0.999F && distance > hardEdge) {
            strength = 1.0F - (distance - hardEdge) / (1.0F - hardEdge);
          }
        } else {
          strength = distance <= hardEdge ? 1.0F : 0.0F;
        }
        strength *= std::clamp(m_opacity * m_flow, 0.0F, 1.0F);
        if (strength > 0.001F) {
          erasePixel(buffer, x, y, strength);
        }
      }
    }
  }
}

void EraserTool::eraseSquare(PixelBuffer& buffer, const Point& center, int radius) const {
  const float radiusF = static_cast<float>(std::max(1, radius));
  const float hardEdge = std::clamp(m_hardness, 0.0F, 1.0F);
  for (int y = center.y - radius; y <= center.y + radius; ++y) {
    for (int x = center.x - radius; x <= center.x + radius; ++x) {
      const int dx = std::abs(x - center.x);
      const int dy = std::abs(y - center.y);
      const float distance = static_cast<float>(std::max(dx, dy)) / radiusF;
      if (distance > 1.0F) {
        continue;
      }
      float strength = 1.0F;
      if (m_antiAlias) {
        if (hardEdge < 0.999F && distance > hardEdge) {
          strength = 1.0F - (distance - hardEdge) / (1.0F - hardEdge);
        }
      } else {
        strength = distance <= hardEdge ? 1.0F : 0.0F;
      }
      strength *= std::clamp(m_opacity * m_flow, 0.0F, 1.0F);
      if (strength > 0.001F) {
        erasePixel(buffer, x, y, strength);
      }
    }
  }
}

void EraserTool::erasePixel(PixelBuffer& buffer, int x, int y, float strength) const {
  if (!buffer.inBounds(x, y)) {
    return;
  }

  const float s = std::clamp(strength, 0.0F, 1.0F);
  const Color dst = buffer.pixel(x, y);
  const float keep = 1.0F - s;
  buffer.setPixel(
      x,
      y,
      Color {
          static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.r) * keep)),
          static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.g) * keep)),
          static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.b) * keep)),
          static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.a) * keep))});
}

} // namespace core
