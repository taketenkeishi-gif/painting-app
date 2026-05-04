#pragma once

#include <optional>
#include <string>
#include <vector>

#include "features/object_editing/ObjectLayerModel.h"

namespace features::object_editing {

enum class HandleHit {
  None,
  Move,
  TopLeft,
  TopRight,
  BottomLeft,
  BottomRight,
  Rotate,
  RulerStart,
  RulerEnd
};

struct SelectionResult {
  std::string selectedObjectId;
  core::Rect bounds {0, 0, 0, 0};
  HandleHit handleHit {HandleHit::None};
  ObjectKind editableKind {ObjectKind::VectorPath};
  bool hasSelection {false};
};

class ObjectSelectionService {
public:
  SelectionResult hitTest(const std::vector<ObjectLayerModel>& layers, core::Point canvasPoint) const noexcept;
};

} // namespace features::object_editing

