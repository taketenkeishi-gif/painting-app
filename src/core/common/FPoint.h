#pragma once

#include <cmath>

namespace core {

struct FPoint {
  float x {0.0f};
  float y {0.0f};

  FPoint operator+(const FPoint& rhs) const noexcept { return {x + rhs.x, y + rhs.y}; }
  FPoint operator-(const FPoint& rhs) const noexcept { return {x - rhs.x, y - rhs.y}; }
  FPoint operator*(float s) const noexcept { return {x * s, y * s}; }

  float length() const noexcept { return std::hypot(x, y); }
  float lengthTo(const FPoint& other) const noexcept { return std::hypot(other.x - x, other.y - y); }
};

} // namespace core
