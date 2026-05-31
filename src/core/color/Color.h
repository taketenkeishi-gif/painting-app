#pragma once

#include <cstdint>

namespace core {

struct Color {
  std::uint8_t r {0};
  std::uint8_t g {0};
  std::uint8_t b {0};
  std::uint8_t a {0};

  static constexpr Color Transparent() noexcept { return Color {0, 0, 0, 0}; }
  static constexpr Color OpaqueBlack() noexcept { return Color {0, 0, 0, 255}; }
  static constexpr Color OpaqueWhite() noexcept { return Color {255, 255, 255, 255}; }
};

} // namespace core
