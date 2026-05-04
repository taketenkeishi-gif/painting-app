#include "features/object_editing/ObjectModel.h"

#include <algorithm>
#include <type_traits>

namespace features::object_editing {

namespace {

core::Rect pointsBounds(const std::vector<core::Point>& points, const core::Rect& fallback) noexcept {
  if (points.empty()) {
    return fallback;
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

} // namespace

core::Rect boundsFromPayload(const ObjectPayload& payload, const core::Rect& fallback) noexcept {
  return std::visit(
      [&](const auto& data) -> core::Rect {
        using T = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<T, RulerPayload>) {
          return pointsBounds({data.start, data.end}, fallback);
        } else if constexpr (std::is_same_v<T, VectorPathPayload>) {
          return pointsBounds(data.points, fallback);
        } else {
          return fallback;
        }
      },
      payload);
}

} // namespace features::object_editing
