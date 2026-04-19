#include "core/selection/SelectionMask.h"

#include <algorithm>
#include <cstddef>

namespace core {

SelectionMask::SelectionMask(int width, int height) {
  resize(width, height);
}

void SelectionMask::resize(int width, int height) {
  m_width = std::max(0, width);
  m_height = std::max(0, height);
  m_mask.assign(static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height), 0U);
  m_hasSelection = false;
  m_bounds.reset();
}

bool SelectionMask::inBounds(int x, int y) const noexcept {
  return x >= 0 && y >= 0 && x < m_width && y < m_height;
}

void SelectionMask::clear() noexcept {
  std::fill(m_mask.begin(), m_mask.end(), 0U);
  m_hasSelection = false;
  m_bounds.reset();
}

bool SelectionMask::setRect(const Rect& rect) {
  if (m_width <= 0 || m_height <= 0) {
    return false;
  }

  const int left = std::clamp(rect.x, 0, m_width - 1);
  const int top = std::clamp(rect.y, 0, m_height - 1);
  const int right = std::clamp(rect.x + std::max(0, rect.width) - 1, 0, m_width - 1);
  const int bottom = std::clamp(rect.y + std::max(0, rect.height) - 1, 0, m_height - 1);

  SelectionMask before = *this;
  clear();

  if (right < left || bottom < top) {
    return before != *this;
  }

  for (int y = top; y <= bottom; ++y) {
    for (int x = left; x <= right; ++x) {
      m_mask[indexOf(x, y)] = 1U;
    }
  }

  m_hasSelection = true;
  m_bounds = Rect {left, top, right - left + 1, bottom - top + 1};
  return before != *this;
}

bool SelectionMask::contains(int x, int y) const noexcept {
  if (!m_hasSelection) {
    return false;
  }
  if (!inBounds(x, y)) {
    return false;
  }
  return m_mask[indexOf(x, y)] != 0U;
}

std::optional<Rect> SelectionMask::boundingRect() const noexcept {
  return m_bounds;
}

bool SelectionMask::operator==(const SelectionMask& other) const noexcept {
  return m_width == other.m_width &&
         m_height == other.m_height &&
         m_hasSelection == other.m_hasSelection &&
         m_bounds == other.m_bounds &&
         m_mask == other.m_mask;
}

std::size_t SelectionMask::indexOf(int x, int y) const noexcept {
  return static_cast<std::size_t>(y) * static_cast<std::size_t>(m_width) + static_cast<std::size_t>(x);
}

} // namespace core
