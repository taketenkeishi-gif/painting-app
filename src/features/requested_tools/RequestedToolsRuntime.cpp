#include "features/requested_tools/RequestedToolsRuntime.h"

#include <algorithm>
#include <cmath>
#include <string_view>

#include "core/layer/Layer.h"

namespace features::requested_tools {

namespace {

bool isRasterLayer(const core::Layer* layer) {
  return layer != nullptr && layer->kind() == core::LayerKind::Raster && !layer->locked();
}

core::Color withOpacity(core::Color color, int opacityPercent) {
  const float factor = std::clamp(static_cast<float>(opacityPercent) / 100.0F, 0.0F, 1.0F);
  color.a = static_cast<std::uint8_t>(std::lround(static_cast<float>(color.a) * factor));
  return color;
}

void blendPixel(core::PixelBuffer& buffer, int x, int y, core::Color src, float strength) {
  if (!buffer.inBounds(x, y) || strength <= 0.0F) {
    return;
  }
  const core::Color dst = buffer.pixel(x, y);
  const float s = std::clamp(strength, 0.0F, 1.0F) * (static_cast<float>(src.a) / 255.0F);
  const float inv = 1.0F - s;
  core::Color out;
  out.r = static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.r) * inv + static_cast<float>(src.r) * s));
  out.g = static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.g) * inv + static_cast<float>(src.g) * s));
  out.b = static_cast<std::uint8_t>(std::lround(static_cast<float>(dst.b) * inv + static_cast<float>(src.b) * s));
  out.a = static_cast<std::uint8_t>(std::max(dst.a, src.a));
  buffer.setPixel(x, y, out);
}

void stampDisk(core::PixelBuffer& buffer, const core::Point& center, int radius, core::Color color, float strength) {
  const int r = std::max(1, radius);
  const int r2 = r * r;
  for (int y = center.y - r; y <= center.y + r; ++y) {
    for (int x = center.x - r; x <= center.x + r; ++x) {
      const int dx = x - center.x;
      const int dy = y - center.y;
      if (dx * dx + dy * dy > r2) {
        continue;
      }
      const float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy)) / static_cast<float>(r);
      const float falloff = std::clamp(1.0F - dist, 0.0F, 1.0F);
      blendPixel(buffer, x, y, color, strength * falloff);
    }
  }
}

void drawLine(core::PixelBuffer& buffer, core::Point from, core::Point to, int radius, core::Color color, float strength) {
  const int dx = to.x - from.x;
  const int dy = to.y - from.y;
  const int steps = std::max(std::abs(dx), std::abs(dy));
  if (steps <= 0) {
    stampDisk(buffer, from, radius, color, strength);
    return;
  }
  for (int i = 0; i <= steps; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(steps);
    core::Point p {
        static_cast<int>(std::lround(static_cast<float>(from.x) + static_cast<float>(dx) * t)),
        static_cast<int>(std::lround(static_cast<float>(from.y) + static_cast<float>(dy) * t))};
    stampDisk(buffer, p, radius, color, strength);
  }
}

} // namespace

bool RequestedToolsRuntime::handles(std::string_view subToolId) const noexcept {
  return subToolId == "gradient_linear" || subToolId == "comic_panel" || subToolId == "text_basic" ||
      subToolId == "ruler_straight" || subToolId == "line_correction_smooth" || subToolId == "clone_stamp_basic" ||
      subToolId == "color_mix_blend" || subToolId == "liquify_push" || subToolId == "operation_object";
}

core::ToolResult RequestedToolsRuntime::onPointerPress(
    std::string_view subToolId,
    core::ToolContext& context,
    const core::ToolPointerEvent& event,
    const core::Color& color,
    int size,
    int opacityPercent) {
  if (subToolId == "gradient_linear") {
    return pressGradient(context, event);
  }
  if (subToolId == "text_basic") {
    return pressText(context, event, color, size, opacityPercent);
  }
  if (subToolId == "clone_stamp_basic") {
    return pressCloneStamp(context, event, size, opacityPercent);
  }
  if (subToolId == "color_mix_blend") {
    return pressBlend(context, event);
  }
  if (subToolId == "liquify_push") {
    return pressLiquify(context, event);
  }
  if (subToolId == "operation_object") {
    return pressOperation(context, event);
  }
  m_dragStart = event.point;
  m_lastPoint = event.point;
  m_dragging = true;
  return {};
}

core::ToolResult RequestedToolsRuntime::onPointerMove(
    std::string_view subToolId,
    core::ToolContext& context,
    const core::ToolPointerEvent& event,
    const core::Color& color,
    int size,
    int opacityPercent) {
  static_cast<void>(color);
  if (subToolId == "gradient_linear") {
    return moveGradient(context, event);
  }
  if (subToolId == "clone_stamp_basic") {
    return moveCloneStamp(context, event, size, opacityPercent);
  }
  if (subToolId == "color_mix_blend") {
    return moveBlend(context, event, size, opacityPercent);
  }
  if (subToolId == "liquify_push") {
    return moveLiquify(context, event, size);
  }
  if (subToolId == "operation_object") {
    return moveOperation(context, event);
  }
  m_lastPoint = event.point;
  core::ToolResult result;
  result.viewportChanged = true;
  return result;
}

core::ToolResult RequestedToolsRuntime::onPointerRelease(
    std::string_view subToolId,
    core::ToolContext& context,
    const core::ToolPointerEvent& event,
    const core::Color& color,
    int size,
    int opacityPercent) {
  if (subToolId == "gradient_linear") {
    return releaseGradient(context, event, color, opacityPercent);
  }
  if (subToolId == "comic_panel") {
    return releaseComic(context, event, color, size, opacityPercent);
  }
  if (subToolId == "ruler_straight") {
    return releaseRuler(context, event, color, size, opacityPercent);
  }
  if (subToolId == "line_correction_smooth") {
    return releaseLineCorrection(context);
  }
  if (subToolId == "clone_stamp_basic") {
    return releaseCloneStamp(context);
  }
  if (subToolId == "operation_object") {
    return releaseOperation(context);
  }
  m_dragging = false;
  return {};
}

void RequestedToolsRuntime::cancel() noexcept {
  m_dragging = false;
  m_cloneStrokeOrigin.reset();
  m_cloneSnapshot.reset();
  m_cloneWorkingSnapshot.reset();
  m_blendSnapshot.reset();
  m_liquifySnapshot.reset();
  m_operationSelectedPathIndex.reset();
}

core::ToolResult RequestedToolsRuntime::pressGradient(core::ToolContext& context, const core::ToolPointerEvent& event) {
  static_cast<void>(context);
  m_dragging = true;
  m_dragStart = event.point;
  m_lastPoint = event.point;
  core::ToolResult result;
  result.viewportChanged = true;
  return result;
}

core::ToolResult RequestedToolsRuntime::moveGradient(core::ToolContext& context, const core::ToolPointerEvent& event) {
  static_cast<void>(context);
  if (!m_dragging) {
    return {};
  }
  m_lastPoint = event.point;
  core::ToolResult result;
  result.viewportChanged = true;
  return result;
}

core::ToolResult RequestedToolsRuntime::releaseGradient(
    core::ToolContext& context,
    const core::ToolPointerEvent& event,
    const core::Color& color,
    int opacityPercent) {
  m_dragging = false;
  core::Layer* active = context.document.activeLayer();
  if (!isRasterLayer(active)) {
    return {};
  }
  core::PixelBuffer& buffer = active->buffer();
  const core::Point end = event.point;
  const int dx = end.x - m_dragStart.x;
  const int dy = end.y - m_dragStart.y;
  const float len2 = static_cast<float>(dx * dx + dy * dy);
  if (len2 < 1.0F) {
    return {};
  }
  const core::Color src = withOpacity(color, opacityPercent);
  for (int y = 0; y < buffer.height(); ++y) {
    for (int x = 0; x < buffer.width(); ++x) {
      const float t = std::clamp(((x - m_dragStart.x) * dx + (y - m_dragStart.y) * dy) / len2, 0.0F, 1.0F);
      core::Color grad = src;
      grad.a = static_cast<std::uint8_t>(std::lround(static_cast<float>(src.a) * t));
      blendPixel(buffer, x, y, grad, t);
    }
  }
  core::ToolResult result;
  result.pixelsChanged = true;
  result.dirtyRect = core::Rect {0, 0, buffer.width(), buffer.height()};
  return result;
}

core::ToolResult RequestedToolsRuntime::releaseComic(
    core::ToolContext& context,
    const core::ToolPointerEvent& event,
    const core::Color& color,
    int size,
    int opacityPercent) {
  core::Layer* active = context.document.activeLayer();
  if (active == nullptr || active->locked()) {
    return {};
  }
  const core::Point end = event.point;
  const int minX = std::min(m_dragStart.x, end.x);
  const int minY = std::min(m_dragStart.y, end.y);
  const int maxX = std::max(m_dragStart.x, end.x);
  const int maxY = std::max(m_dragStart.y, end.y);
  const core::Color c = withOpacity(color, opacityPercent);
  if (active->kind() == core::LayerKind::Vector) {
    core::VectorPath path;
    path.color = c;
    path.width = std::max(1, size);
    path.points = {{minX, minY}, {maxX, minY}, {maxX, maxY}, {minX, maxY}, {minX, minY}};
    active->addVectorPath(std::move(path));
    core::ToolResult result;
    result.pixelsChanged = true;
    result.dirtyRect = core::Rect {0, 0, active->buffer().width(), active->buffer().height()};
    return result;
  }
  if (!isRasterLayer(active)) {
    return {};
  }
  core::PixelBuffer& buffer = active->buffer();
  drawLine(buffer, {minX, minY}, {maxX, minY}, std::max(1, size / 2), c, 1.0F);
  drawLine(buffer, {maxX, minY}, {maxX, maxY}, std::max(1, size / 2), c, 1.0F);
  drawLine(buffer, {maxX, maxY}, {minX, maxY}, std::max(1, size / 2), c, 1.0F);
  drawLine(buffer, {minX, maxY}, {minX, minY}, std::max(1, size / 2), c, 1.0F);
  core::ToolResult result;
  result.pixelsChanged = true;
  result.dirtyRect = core::Rect {minX, minY, maxX - minX + 1, maxY - minY + 1};
  return result;
}

core::ToolResult RequestedToolsRuntime::pressText(
    core::ToolContext& context,
    const core::ToolPointerEvent& event,
    const core::Color& color,
    int size,
    int opacityPercent) {
  core::Layer* active = context.document.activeLayer();
  if (active == nullptr || active->locked()) {
    return {};
  }
  const int w = std::max(20, size * 5);
  const int h = std::max(12, size * 2);
  const core::Color c = withOpacity(color, opacityPercent);
  if (active->kind() == core::LayerKind::Vector) {
    core::VectorPath path;
    path.color = c;
    path.width = std::max(1, size / 3);
    path.points = {
        {event.point.x, event.point.y},
        {event.point.x + w, event.point.y},
        {event.point.x + w, event.point.y + h},
        {event.point.x, event.point.y + h},
        {event.point.x, event.point.y}};
    active->addVectorPath(std::move(path));
    core::ToolResult result;
    result.pixelsChanged = true;
    result.dirtyRect = core::Rect {0, 0, active->buffer().width(), active->buffer().height()};
    return result;
  }
  if (!isRasterLayer(active)) {
    return {};
  }
  core::PixelBuffer& buffer = active->buffer();
  for (int y = event.point.y; y < event.point.y + h; ++y) {
    for (int x = event.point.x; x < event.point.x + w; ++x) {
      if (!buffer.inBounds(x, y)) {
        continue;
      }
      const bool border = (x == event.point.x) || (x == event.point.x + w - 1) || (y == event.point.y) ||
          (y == event.point.y + h - 1);
      if (border || ((x - event.point.x) % std::max(2, size) == 0)) {
        blendPixel(buffer, x, y, c, border ? 1.0F : 0.35F);
      }
    }
  }
  core::ToolResult result;
  result.pixelsChanged = true;
  result.dirtyRect = core::Rect {event.point.x, event.point.y, w, h};
  return result;
}

core::ToolResult RequestedToolsRuntime::releaseRuler(
    core::ToolContext& context,
    const core::ToolPointerEvent& event,
    const core::Color& color,
    int size,
    int opacityPercent) {
  core::Layer* active = context.document.activeLayer();
  if (active == nullptr || active->locked()) {
    return {};
  }
  if (active->kind() == core::LayerKind::Raster) {
    const std::size_t guideLayer = context.document.addVectorLayer("Ruler Guides");
    active = &context.document.layerAt(guideLayer);
  }
  const core::Color c = withOpacity(color, opacityPercent);
  if (active->kind() == core::LayerKind::Vector) {
    core::VectorPath path;
    path.kind = core::VectorPath::Kind::Ruler;
    path.color = c;
    path.width = std::max(1, size / 3);
    path.points = {m_dragStart, event.point};
    active->addVectorPath(std::move(path));
    core::ToolResult result;
    result.pixelsChanged = true;
    result.viewportChanged = true;
    result.dirtyRect = core::Rect {0, 0, active->buffer().width(), active->buffer().height()};
    return result;
  }
  return {};
}

core::ToolResult RequestedToolsRuntime::releaseLineCorrection(core::ToolContext& context) {
  core::Layer* active = context.document.activeLayer();
  if (active == nullptr || active->kind() != core::LayerKind::Vector || active->locked()) {
    return {};
  }
  auto& paths = active->vectorPaths();
  if (paths.empty()) {
    return {};
  }
  core::VectorPath& path = paths.back();
  if (path.points.size() < 3) {
    return {};
  }
  std::vector<core::Point> smoothed = path.points;
  for (std::size_t i = 1; i + 1 < path.points.size(); ++i) {
    smoothed[i].x = (path.points[i - 1].x + path.points[i].x + path.points[i + 1].x) / 3;
    smoothed[i].y = (path.points[i - 1].y + path.points[i].y + path.points[i + 1].y) / 3;
  }
  path.points = std::move(smoothed);
  core::ToolResult result;
  result.pixelsChanged = true;
  result.dirtyRect = core::Rect {0, 0, active->buffer().width(), active->buffer().height()};
  return result;
}

core::ToolResult RequestedToolsRuntime::pressCloneStamp(
    core::ToolContext& context,
    const core::ToolPointerEvent& event,
    int size,
    int opacityPercent) {
  core::Layer* active = context.document.activeLayer();
  if (!isRasterLayer(active)) {
    return {};
  }
  if (event.alt) {
    m_cloneSamplePoint = event.point;
    m_cloneHasSample = true;
    m_cloneStrokeOrigin.reset();
    m_cloneSnapshot.reset();
    m_cloneWorkingSnapshot.reset();
    return {};
  }
  if (!m_cloneHasSample) {
    m_cloneSamplePoint = event.point;
    m_cloneHasSample = true;
    return {};
  }
  m_cloneStrokeOrigin = event.point;
  m_cloneSnapshot = active->buffer();
  m_cloneWorkingSnapshot = active->buffer();
  m_lastPoint = event.point;
  m_dragging = true;
  return moveCloneStamp(context, event, size, opacityPercent);
}

core::ToolResult RequestedToolsRuntime::moveCloneStamp(
    core::ToolContext& context,
    const core::ToolPointerEvent& event,
    int size,
    int opacityPercent) {
  core::Layer* active = context.document.activeLayer();
  if (!isRasterLayer(active) || !m_cloneStrokeOrigin.has_value() || !m_cloneSnapshot.has_value() ||
      !m_cloneWorkingSnapshot.has_value()) {
    return {};
  }
  core::PixelBuffer& dst = active->buffer();
  core::PixelBuffer& work = *m_cloneWorkingSnapshot;
  const core::PixelBuffer& src = *m_cloneSnapshot;
  const int radius = std::max(1, size / 2);
  const float strength = std::clamp(static_cast<float>(opacityPercent) / 100.0F, 0.05F, 1.0F);
  for (int y = event.point.y - radius; y <= event.point.y + radius; ++y) {
    for (int x = event.point.x - radius; x <= event.point.x + radius; ++x) {
      if (!dst.inBounds(x, y)) {
        continue;
      }
      const int sx = m_cloneSamplePoint.x + (x - m_cloneStrokeOrigin->x);
      const int sy = m_cloneSamplePoint.y + (y - m_cloneStrokeOrigin->y);
      if (!src.inBounds(sx, sy)) {
        continue;
      }
      core::Color sample = src.pixel(sx, sy);
      blendPixel(work, x, y, sample, strength);
    }
  }
  dst = work;
  m_lastPoint = event.point;
  core::ToolResult result;
  result.pixelsChanged = true;
  result.dirtyRect = core::Rect {0, 0, dst.width(), dst.height()};
  return result;
}

core::ToolResult RequestedToolsRuntime::releaseCloneStamp(core::ToolContext& context) {
  static_cast<void>(context);
  m_dragging = false;
  m_cloneStrokeOrigin.reset();
  m_cloneSnapshot.reset();
  m_cloneWorkingSnapshot.reset();
  return {};
}

core::ToolResult RequestedToolsRuntime::pressBlend(core::ToolContext& context, const core::ToolPointerEvent& event) {
  core::Layer* active = context.document.activeLayer();
  if (!isRasterLayer(active)) {
    return {};
  }
  m_blendSnapshot = active->buffer();
  m_lastPoint = event.point;
  m_dragging = true;
  return {};
}

core::ToolResult RequestedToolsRuntime::moveBlend(
    core::ToolContext& context,
    const core::ToolPointerEvent& event,
    int size,
    int opacityPercent) {
  core::Layer* active = context.document.activeLayer();
  if (!isRasterLayer(active) || !m_blendSnapshot.has_value()) {
    return {};
  }
  core::PixelBuffer& dst = active->buffer();
  const core::PixelBuffer src = *m_blendSnapshot;
  const int radius = std::max(2, size / 2);
  const float mix = std::clamp(static_cast<float>(opacityPercent) / 120.0F, 0.08F, 0.9F);
  for (int y = event.point.y - radius; y <= event.point.y + radius; ++y) {
    for (int x = event.point.x - radius; x <= event.point.x + radius; ++x) {
      if (!dst.inBounds(x, y)) {
        continue;
      }
      const int rx = x - event.point.x;
      const int ry = y - event.point.y;
      const float dist = std::sqrt(static_cast<float>(rx * rx + ry * ry));
      if (dist > static_cast<float>(radius)) {
        continue;
      }
      const float local = mix * (1.0F - (dist / static_cast<float>(radius)));
      if (local <= 0.0F) {
        continue;
      }
      const core::Color base = src.pixel(x, y);
      float rr = static_cast<float>(base.r) * 2.0F;
      float gg = static_cast<float>(base.g) * 2.0F;
      float bb = static_cast<float>(base.b) * 2.0F;
      float aa = static_cast<float>(base.a) * 2.0F;
      float weightSum = 2.0F;
      for (int ny = -2; ny <= 2; ++ny) {
        for (int nx = -2; nx <= 2; ++nx) {
          if (nx == 0 && ny == 0) {
            continue;
          }
          const int px = x + nx;
          const int py = y + ny;
          if (!src.inBounds(px, py)) {
            continue;
          }
          const float d = std::sqrt(static_cast<float>(nx * nx + ny * ny));
          const float w = std::max(0.0F, 1.0F - (d / 3.0F));
          if (w <= 0.0F) {
            continue;
          }
          const core::Color s = src.pixel(px, py);
          rr += static_cast<float>(s.r) * w;
          gg += static_cast<float>(s.g) * w;
          bb += static_cast<float>(s.b) * w;
          aa += static_cast<float>(s.a) * w;
          weightSum += w;
        }
      }
      core::Color avg {
          static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(rr / weightSum)), 0, 255)),
          static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(gg / weightSum)), 0, 255)),
          static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(bb / weightSum)), 0, 255)),
          static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(aa / weightSum)), 0, 255))};
      blendPixel(dst, x, y, avg, local);
    }
  }
  m_blendSnapshot = dst;
  m_lastPoint = event.point;
  core::ToolResult result;
  result.pixelsChanged = true;
  result.dirtyRect = core::Rect {0, 0, dst.width(), dst.height()};
  return result;
}

core::ToolResult RequestedToolsRuntime::pressLiquify(core::ToolContext& context, const core::ToolPointerEvent& event) {
  core::Layer* active = context.document.activeLayer();
  if (!isRasterLayer(active)) {
    return {};
  }
  m_liquifySnapshot = active->buffer();
  m_lastPoint = event.point;
  m_dragging = true;
  return {};
}

core::ToolResult RequestedToolsRuntime::moveLiquify(core::ToolContext& context, const core::ToolPointerEvent& event, int size) {
  core::Layer* active = context.document.activeLayer();
  if (!isRasterLayer(active) || !m_liquifySnapshot.has_value()) {
    return {};
  }
  core::PixelBuffer src = *m_liquifySnapshot;
  core::PixelBuffer& dst = active->buffer();
  const int radius = std::max(4, size);
  const int dx = event.point.x - m_lastPoint.x;
  const int dy = event.point.y - m_lastPoint.y;
  for (int y = event.point.y - radius; y <= event.point.y + radius; ++y) {
    for (int x = event.point.x - radius; x <= event.point.x + radius; ++x) {
      if (!dst.inBounds(x, y)) {
        continue;
      }
      const int ox = x - event.point.x;
      const int oy = y - event.point.y;
      const float dist = std::sqrt(static_cast<float>(ox * ox + oy * oy));
      if (dist > static_cast<float>(radius)) {
        continue;
      }
      const float weight = 1.0F - (dist / static_cast<float>(radius));
      const int sx = x - static_cast<int>(std::lround(static_cast<float>(dx) * weight));
      const int sy = y - static_cast<int>(std::lround(static_cast<float>(dy) * weight));
      if (!src.inBounds(sx, sy)) {
        continue;
      }
      dst.setPixel(x, y, src.pixel(sx, sy));
    }
  }
  m_liquifySnapshot = dst;
  m_lastPoint = event.point;
  core::ToolResult result;
  result.pixelsChanged = true;
  result.dirtyRect = core::Rect {0, 0, dst.width(), dst.height()};
  return result;
}

core::ToolResult RequestedToolsRuntime::pressOperation(core::ToolContext& context, const core::ToolPointerEvent& event) {
  core::Layer* active = context.document.activeLayer();
  if (active == nullptr || active->locked()) {
    return {};
  }
  m_dragging = true;
  m_lastPoint = event.point;
  m_operationSelectedPathIndex.reset();
  if (active->kind() != core::LayerKind::Vector) {
    return {};
  }
  auto& paths = active->vectorPaths();
  float bestDist2 = 1.0e12F;
  for (std::size_t i = 0; i < paths.size(); ++i) {
    for (const core::Point& p : paths[i].points) {
      const float dx = static_cast<float>(p.x - event.point.x);
      const float dy = static_cast<float>(p.y - event.point.y);
      const float d2 = dx * dx + dy * dy;
      if (d2 < bestDist2) {
        bestDist2 = d2;
        m_operationSelectedPathIndex = i;
      }
    }
  }
  if (bestDist2 > 400.0F) {
    m_operationSelectedPathIndex.reset();
  }
  core::ToolResult result;
  result.viewportChanged = true;
  return result;
}

core::ToolResult RequestedToolsRuntime::moveOperation(core::ToolContext& context, const core::ToolPointerEvent& event) {
  if (!m_dragging) {
    return {};
  }
  core::Layer* active = context.document.activeLayer();
  if (active == nullptr || active->locked()) {
    return {};
  }
  const int dx = event.point.x - m_lastPoint.x;
  const int dy = event.point.y - m_lastPoint.y;
  m_lastPoint = event.point;
  if (dx == 0 && dy == 0) {
    return {};
  }
  if (active->kind() == core::LayerKind::Vector && m_operationSelectedPathIndex.has_value() &&
      *m_operationSelectedPathIndex < active->vectorPaths().size()) {
    auto& points = active->vectorPaths()[*m_operationSelectedPathIndex].points;
    for (core::Point& p : points) {
      p.x += dx;
      p.y += dy;
    }
    core::ToolResult result;
    result.pixelsChanged = true;
    result.viewportChanged = true;
    result.dirtyRect = core::Rect {0, 0, active->buffer().width(), active->buffer().height()};
    return result;
  }
  return {};
}

core::ToolResult RequestedToolsRuntime::releaseOperation(core::ToolContext& context) {
  static_cast<void>(context);
  m_dragging = false;
  core::ToolResult result;
  result.viewportChanged = true;
  return result;
}

} // namespace features::requested_tools
