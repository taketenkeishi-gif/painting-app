#pragma once

#include <string>
#include <vector>

#include "features/object_editing/ObjectModel.h"

namespace features::object_editing {

struct ObjectLayerModel {
  std::string id;
  std::string name;
  bool visible {true};
  bool locked {false};
  std::vector<ObjectModel> objects;
};

} // namespace features::object_editing

