#include "core/tools/MoveLayerTool.h"

#include "core/selection/SelectionMask.h"

namespace core {

ToolResult MoveLayerTool::onPointerPress(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  m_dragging = true;
  m_start = event.point;
  m_current = event.point;
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult MoveLayerTool::onPointerMove(ToolContext& context, const ToolPointerEvent& event) {
  static_cast<void>(context);
  if (!m_dragging) {
    return {};
  }
  m_current = event.point;
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult MoveLayerTool::onPointerRelease(ToolContext& context, const ToolPointerEvent& event) {
  if (!m_dragging) {
    return {};
  }

  m_dragging = false;
  m_current = event.point;

  Layer* active = context.document.activeLayer();
  if (active == nullptr) {
    return {};
  }

  const int dx = m_current.x - m_start.x;
  const int dy = m_current.y - m_start.y;
  if (dx == 0 && dy == 0) {
    ToolResult result;
    result.viewportChanged = true;
    return result;
  }

  if (active->kind() == LayerKind::Vector) {
    active->moveVectorPathsBy(dx, dy);
    ToolResult result;
    result.pixelsChanged = true;
    result.viewportChanged = true;
    return result;
  }
  if (active->kind() != LayerKind::Raster) {
    ToolResult result;
    result.viewportChanged = true;
    return result;
  }

  PixelBuffer& source = active->buffer();
  const SelectionMask& selection = context.document.selection();
  const bool hasSelection = selection.hasSelection();

  PixelBuffer moved;
  if (!hasSelection) {
    moved.resize(source.width(), source.height(), Color::Transparent());
    for (int y = 0; y < source.height(); ++y) {
      for (int x = 0; x < source.width(); ++x) {
        const int nx = x + dx;
        const int ny = y + dy;
        if (!moved.inBounds(nx, ny)) {
          continue;
        }
        moved.setPixel(nx, ny, source.pixel(x, y));
      }
    }
  } else {
    moved = source;
    const PixelBuffer snapshot = source;
    for (int y = 0; y < source.height(); ++y) {
      for (int x = 0; x < source.width(); ++x) {
        if (!selection.contains(x, y)) {
          continue;
        }
        moved.setPixel(x, y, Color::Transparent());
      }
    }
    for (int y = 0; y < source.height(); ++y) {
      for (int x = 0; x < source.width(); ++x) {
        if (!selection.contains(x, y)) {
          continue;
        }
        const int nx = x + dx;
        const int ny = y + dy;
        if (!moved.inBounds(nx, ny)) {
          continue;
        }
        moved.setPixel(nx, ny, snapshot.pixel(x, y));
      }
    }
  }

  source = moved;
  ToolResult result;
  result.pixelsChanged = true;
  result.viewportChanged = true;
  return result;
}

ToolResult MoveLayerTool::onCancel(ToolContext& context) {
  static_cast<void>(context);
  if (!m_dragging) {
    return {};
  }
  m_dragging = false;
  ToolResult result;
  result.viewportChanged = true;
  return result;
}

ToolResult MoveLayerTool::onWheel(ToolContext& context, int deltaSteps, const ToolPointerEvent& event) {
  static_cast<void>(context);
  static_cast<void>(deltaSteps);
  static_cast<void>(event);
  return {};
}

ToolOverlayState MoveLayerTool::overlay() const {
  ToolOverlayState state;
  if (!m_dragging) {
    return state;
  }
  state.hasLine = true;
  state.lineStart = m_start;
  state.lineEnd = m_current;
  return state;
}

} // namespace core
