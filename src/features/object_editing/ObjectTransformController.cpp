#include "features/object_editing/ObjectTransformController.h"

#include <algorithm>

namespace features::object_editing {

bool ObjectTransformController::applyDrag(ObjectModel& object, HandleHit handle, int dx, int dy) const noexcept {
  if (dx == 0 && dy == 0) {
    return false;
  }
  if (handle == HandleHit::Move) {
    object.bounds.x += dx;
    object.bounds.y += dy;
    if (auto* ruler = std::get_if<RulerPayload>(&object.payload)) {
      ruler->start.x += dx;
      ruler->start.y += dy;
      ruler->end.x += dx;
      ruler->end.y += dy;
    } else if (auto* path = std::get_if<VectorPathPayload>(&object.payload)) {
      for (auto& p : path->points) {
        p.x += dx;
        p.y += dy;
      }
    }
    return true;
  }
  if (handle == HandleHit::RulerStart || handle == HandleHit::RulerEnd) {
    auto* ruler = std::get_if<RulerPayload>(&object.payload);
    if (ruler == nullptr) {
      return false;
    }
    core::Point& p = handle == HandleHit::RulerStart ? ruler->start : ruler->end;
    p.x += dx;
    p.y += dy;
    object.bounds = boundsFromPayload(object.payload, object.bounds);
    return true;
  }
  if (handle == HandleHit::Rotate) {
    object.transform.rotationDeg += static_cast<float>(dx);
    return true;
  }

  int left = object.bounds.x;
  int top = object.bounds.y;
  int right = object.bounds.x + object.bounds.width;
  int bottom = object.bounds.y + object.bounds.height;
  if (handle == HandleHit::TopLeft) {
    left += dx;
    top += dy;
  } else if (handle == HandleHit::TopRight) {
    right += dx;
    top += dy;
  } else if (handle == HandleHit::BottomLeft) {
    left += dx;
    bottom += dy;
  } else if (handle == HandleHit::BottomRight) {
    right += dx;
    bottom += dy;
  } else {
    return false;
  }
  if (right - left < 4 || bottom - top < 4) {
    return false;
  }
  object.bounds = core::Rect {left, top, right - left, bottom - top};
  if (auto* text = std::get_if<TextPayload>(&object.payload)) {
    text->fontSize = std::max(8, object.bounds.height / 2);
  }
  return true;
}

} // namespace features::object_editing
