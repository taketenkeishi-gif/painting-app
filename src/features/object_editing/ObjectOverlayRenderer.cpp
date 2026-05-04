#include "features/object_editing/ObjectOverlayRenderer.h"

namespace features::object_editing {

ObjectOverlayModel ObjectOverlayRenderer::buildSelectionOverlay(const SelectionResult& selection) const {
  ObjectOverlayModel out;
  if (!selection.hasSelection) {
    return out;
  }
  OverlayPrimitive box;
  box.kind = OverlayPrimitive::Kind::Rect;
  box.rect = selection.bounds;
  out.primitives.push_back(box);

  const core::Point tl {selection.bounds.x, selection.bounds.y};
  const core::Point tr {selection.bounds.x + selection.bounds.width, selection.bounds.y};
  const core::Point bl {selection.bounds.x, selection.bounds.y + selection.bounds.height};
  const core::Point br {selection.bounds.x + selection.bounds.width, selection.bounds.y + selection.bounds.height};
  for (const core::Point& p : {tl, tr, bl, br}) {
    OverlayPrimitive h;
    h.kind = OverlayPrimitive::Kind::HandlePoint;
    h.p1 = p;
    out.primitives.push_back(h);
  }
  OverlayPrimitive stem;
  stem.kind = OverlayPrimitive::Kind::Line;
  stem.p1 = core::Point {selection.bounds.x + selection.bounds.width / 2, selection.bounds.y};
  stem.p2 = core::Point {selection.bounds.x + selection.bounds.width / 2, selection.bounds.y - 18};
  out.primitives.push_back(stem);
  OverlayPrimitive rot;
  rot.kind = OverlayPrimitive::Kind::HandlePoint;
  rot.p1 = stem.p2;
  out.primitives.push_back(rot);
  return out;
}

} // namespace features::object_editing
