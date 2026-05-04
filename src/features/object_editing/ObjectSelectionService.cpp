#include "features/object_editing/ObjectSelectionService.h"

#include <cmath>

namespace features::object_editing {

namespace {

bool inRect(const core::Rect& r, core::Point p, int pad = 0) noexcept {
  return p.x >= r.x - pad && p.y >= r.y - pad &&
      p.x <= r.x + r.width + pad && p.y <= r.y + r.height + pad;
}

bool nearPoint(core::Point a, core::Point b, int radius = 6) noexcept {
  const int dx = a.x - b.x;
  const int dy = a.y - b.y;
  return dx * dx + dy * dy <= radius * radius;
}

} // namespace

SelectionResult ObjectSelectionService::hitTest(const std::vector<ObjectLayerModel>& layers, core::Point canvasPoint) const noexcept {
  for (std::size_t li = layers.size(); li > 0; --li) {
    const ObjectLayerModel& layer = layers[li - 1];
    if (!layer.visible || layer.locked) {
      continue;
    }
    for (std::size_t oi = layer.objects.size(); oi > 0; --oi) {
      const ObjectModel& obj = layer.objects[oi - 1];
      if (!obj.visible || obj.locked || !inRect(obj.bounds, canvasPoint, 8)) {
        continue;
      }
      SelectionResult out;
      out.selectedObjectId = obj.id;
      out.bounds = obj.bounds;
      out.editableKind = obj.kind;
      out.hasSelection = true;
      if (obj.kind == ObjectKind::Ruler) {
        if (const auto* ruler = std::get_if<RulerPayload>(&obj.payload)) {
          if (nearPoint(canvasPoint, ruler->start)) out.handleHit = HandleHit::RulerStart;
          else if (nearPoint(canvasPoint, ruler->end)) out.handleHit = HandleHit::RulerEnd;
          else out.handleHit = HandleHit::Move;
        } else {
          out.handleHit = HandleHit::Move;
        }
      } else {
        const core::Point rot {obj.bounds.x + obj.bounds.width / 2, obj.bounds.y - 18};
        if (nearPoint(canvasPoint, rot)) {
          out.handleHit = HandleHit::Rotate;
          return out;
        }
        const core::Point tl {obj.bounds.x, obj.bounds.y};
        const core::Point tr {obj.bounds.x + obj.bounds.width, obj.bounds.y};
        const core::Point bl {obj.bounds.x, obj.bounds.y + obj.bounds.height};
        const core::Point br {obj.bounds.x + obj.bounds.width, obj.bounds.y + obj.bounds.height};
        if (nearPoint(canvasPoint, tl)) out.handleHit = HandleHit::TopLeft;
        else if (nearPoint(canvasPoint, tr)) out.handleHit = HandleHit::TopRight;
        else if (nearPoint(canvasPoint, bl)) out.handleHit = HandleHit::BottomLeft;
        else if (nearPoint(canvasPoint, br)) out.handleHit = HandleHit::BottomRight;
        else out.handleHit = HandleHit::Move;
      }
      return out;
    }
  }
  return {};
}

} // namespace features::object_editing
