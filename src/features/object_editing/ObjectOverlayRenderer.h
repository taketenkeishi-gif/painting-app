#pragma once

#include <optional>
#include <vector>

#include "features/object_editing/ObjectSelectionService.h"

namespace features::object_editing {

struct OverlayPrimitive {
  enum class Kind {
    Rect,
    HandlePoint,
    Line
  };
  Kind kind {Kind::Rect};
  core::Rect rect {0, 0, 0, 0};
  core::Point p1 {0, 0};
  core::Point p2 {0, 0};
};

struct ObjectOverlayModel {
  std::vector<OverlayPrimitive> primitives;
};

class ObjectOverlayRenderer {
public:
  ObjectOverlayModel buildSelectionOverlay(const SelectionResult& selection) const;
};

} // namespace features::object_editing

