#pragma once

namespace core {

struct Rect {
  int x {0};
  int y {0};
  int width {0};
  int height {0};
};

inline bool operator==(const Rect& lhs, const Rect& rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y &&
         lhs.width == rhs.width && lhs.height == rhs.height;
}

inline bool operator!=(const Rect& lhs, const Rect& rhs) noexcept {
  return !(lhs == rhs);
}

} // namespace core
