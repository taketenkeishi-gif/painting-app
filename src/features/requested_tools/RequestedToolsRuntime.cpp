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

core::Rect pointsBounds(const std::vector<core::Point>& points) {
  if (points.empty()) {
    return core::Rect {0, 0, 0, 0};
  }
  int minX = points.front().x;
  int minY = points.front().y;
  int maxX = points.front().x;
  int maxY = points.front().y;
  for (const core::Point& p : points) {
    minX = std::min(minX, p.x);
    minY = std::min(minY, p.y);
    maxX = std::max(maxX, p.x);
    maxY = std::max(maxY, p.y);
  }
  return core::Rect {minX, minY, std::max(1, maxX - minX + 1), std::max(1, maxY - minY + 1)};
}

core::Rect textBounds(const core::VectorPath& path) {
  if (path.points.empty()) {
    return core::Rect {0, 0, 0, 0};
  }
  const core::Point p = path.points.front();
  const int charW = std::max(4, path.width);
  auto utf8GlyphCount = [](std::string_view s) {
    int count = 0;
    for (unsigned char c : s) {
      if ((c & 0xC0) != 0x80) {
        ++count;
      }
    }
    return std::max(1, count);
  };
  const int glyphCount = utf8GlyphCount(path.text);
  const int w = std::max(12, glyphCount * (charW + 2));
  const int h = std::max(8, path.width * 2);
  return core::Rect {p.x, p.y, w, h};
}

bool inRect(const core::Rect& rect, core::Point p, int padding = 0) {
  return p.x >= rect.x - padding && p.y >= rect.y - padding &&
      p.x <= rect.x + rect.width + padding && p.y <= rect.y + rect.height + padding;
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
  m_selectionState = SelectionState {};
  m_operationHandle = OperationHandle::None;
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
  if (active->kind() == core::LayerKind::Raster) {
    const std::size_t guideLayer = context.document.addVectorLayer("Comic Objects");
    active = &context.document.layerAt(guideLayer);
  }
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
  return {};
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
  static_cast<void>(context);
  static_cast<void>(event);
  static_cast<void>(color);
  static_cast<void>(size);
  static_cast<void>(opacityPercent);
  return {};
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
  m_selectionState = SelectionState {};
  m_dragging = true;
  m_lastPoint = event.point;
  m_operationHandle = OperationHandle::None;

  for (std::size_t layerRev = context.document.layerCount(); layerRev > 0; --layerRev) {
    const std::size_t layerIndex = layerRev - 1;
    core::Layer& layer = context.document.layerAt(layerIndex);
    if (!layer.visible() || layer.locked() || layer.kind() != core::LayerKind::Vector) {
      continue;
    }
    auto& paths = layer.vectorPaths();
    for (std::size_t pathRev = paths.size(); pathRev > 0; --pathRev) {
      const std::size_t i = pathRev - 1;
      const core::VectorPath& path = paths[i];
      core::Rect bounds = path.kind == core::VectorPath::Kind::Text ? textBounds(path) : pointsBounds(path.points);
      if (bounds.width <= 0 || bounds.height <= 0 || !inRect(bounds, event.point, 8)) {
        continue;
      }
      m_selectionState.layerIndex = layerIndex;
      m_selectionState.pathIndex = i;
      m_selectionState.pathKind = path.kind;
      m_selectionState.bounds = bounds;
      m_selectionState.valid = true;

      if (path.kind == core::VectorPath::Kind::Ruler && path.points.size() >= 2) {
        const core::Point p0 = path.points[0];
        const core::Point p1 = path.points[1];
        const float d0 = static_cast<float>((p0.x - event.point.x) * (p0.x - event.point.x) + (p0.y - event.point.y) * (p0.y - event.point.y));
        const float d1 = static_cast<float>((p1.x - event.point.x) * (p1.x - event.point.x) + (p1.y - event.point.y) * (p1.y - event.point.y));
        if (d0 <= 100.0F) {
          m_operationHandle = OperationHandle::RulerStart;
        } else if (d1 <= 100.0F) {
          m_operationHandle = OperationHandle::RulerEnd;
        } else {
          m_operationHandle = OperationHandle::Move;
        }
      } else {
        const core::Point tl {bounds.x, bounds.y};
        const core::Point tr {bounds.x + bounds.width, bounds.y};
        const core::Point bl {bounds.x, bounds.y + bounds.height};
        const core::Point br {bounds.x + bounds.width, bounds.y + bounds.height};
        auto nearHandle = [&](core::Point hp) {
          const int hx = hp.x - event.point.x;
          const int hy = hp.y - event.point.y;
          return hx * hx + hy * hy <= 144;
        };
        if (nearHandle(tl)) m_operationHandle = OperationHandle::RectTopLeft;
        else if (nearHandle(tr)) m_operationHandle = OperationHandle::RectTopRight;
        else if (nearHandle(bl)) m_operationHandle = OperationHandle::RectBottomLeft;
        else if (nearHandle(br)) m_operationHandle = OperationHandle::RectBottomRight;
        else m_operationHandle = OperationHandle::Move;
      }
      goto selected_done;
    }
  }
selected_done:
  core::ToolResult result;
  result.viewportChanged = true;
  return result;
}

core::ToolResult RequestedToolsRuntime::moveOperation(core::ToolContext& context, const core::ToolPointerEvent& event) {
  if (!m_dragging) {
    return {};
  }
  core::Layer* active = context.document.activeLayer();
  if (!m_selectionState.valid || m_selectionState.layerIndex >= context.document.layerCount()) {
    return {};
  }
  core::Layer& layer = context.document.layerAt(m_selectionState.layerIndex);
  if (layer.locked() || layer.kind() != core::LayerKind::Vector || m_selectionState.pathIndex >= layer.vectorPaths().size()) {
    return {};
  }
  const int dx = event.point.x - m_lastPoint.x;
  const int dy = event.point.y - m_lastPoint.y;
  m_lastPoint = event.point;
  if (dx == 0 && dy == 0) {
    return {};
  }
  core::VectorPath& path = layer.vectorPaths()[m_selectionState.pathIndex];
  if (m_operationHandle == OperationHandle::Move) {
    for (core::Point& p : path.points) {
      p.x += dx;
      p.y += dy;
    }
  } else if (path.kind == core::VectorPath::Kind::Ruler && path.points.size() >= 2) {
    if (m_operationHandle == OperationHandle::RulerStart) {
      path.points[0].x += dx;
      path.points[0].y += dy;
    } else if (m_operationHandle == OperationHandle::RulerEnd) {
      path.points[1].x += dx;
      path.points[1].y += dy;
    }
  } else {
    core::Rect b = path.kind == core::VectorPath::Kind::Text ? textBounds(path) : pointsBounds(path.points);
    int left = b.x;
    int top = b.y;
    int right = b.x + b.width;
    int bottom = b.y + b.height;
    if (m_operationHandle == OperationHandle::RectTopLeft) {
      left += dx;
      top += dy;
    } else if (m_operationHandle == OperationHandle::RectTopRight) {
      right += dx;
      top += dy;
    } else if (m_operationHandle == OperationHandle::RectBottomLeft) {
      left += dx;
      bottom += dy;
    } else if (m_operationHandle == OperationHandle::RectBottomRight) {
      right += dx;
      bottom += dy;
    }
    if (right - left < 4 || bottom - top < 4) {
      return {};
    }
    if (path.kind == core::VectorPath::Kind::Text) {
      if (!path.points.empty()) {
        path.points[0] = core::Point {left, top};
      }
      path.width = std::max(4, (bottom - top) / 2);
    } else if (path.points.size() >= 4) {
      path.points = {{left, top}, {right, top}, {right, bottom}, {left, bottom}, {left, top}};
    }
  }
  m_selectionState.bounds = path.kind == core::VectorPath::Kind::Text ? textBounds(path) : pointsBounds(path.points);
  core::ToolResult result;
  result.pixelsChanged = true;
  result.viewportChanged = true;
  result.dirtyRect = core::Rect {0, 0, layer.buffer().width(), layer.buffer().height()};
  return result;
}

core::ToolResult RequestedToolsRuntime::releaseOperation(core::ToolContext& context) {
  static_cast<void>(context);
  m_dragging = false;
  core::ToolResult result;
  result.viewportChanged = true;
  return result;
}

} // namespace features::requested_tools
