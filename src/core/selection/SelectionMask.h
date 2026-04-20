#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "core/common/Rect.h"

namespace core {

class SelectionMask {
public:
  SelectionMask() = default;
  SelectionMask(int width, int height);

  int width() const noexcept { return m_width; }
  int height() const noexcept { return m_height; }

  void resize(int width, int height);
  bool inBounds(int x, int y) const noexcept;

  void clear() noexcept;
  bool setRect(const Rect& rect);
  bool invert();

  bool hasSelection() const noexcept { return m_hasSelection; }
  bool contains(int x, int y) const noexcept;
  std::optional<Rect> boundingRect() const noexcept;

  bool operator==(const SelectionMask& other) const noexcept;
  bool operator!=(const SelectionMask& other) const noexcept { return !(*this == other); }

private:
  std::size_t indexOf(int x, int y) const noexcept;

  int m_width {0};
  int m_height {0};
  std::vector<std::uint8_t> m_mask;
  bool m_hasSelection {false};
  std::optional<Rect> m_bounds;
};

} // namespace core
