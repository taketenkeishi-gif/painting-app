#pragma once

#include "features/object_editing/ObjectSelectionService.h"

namespace features::object_editing {

class ObjectTransformController {
public:
  bool applyDrag(ObjectModel& object, HandleHit handle, int dx, int dy) const noexcept;
};

} // namespace features::object_editing

