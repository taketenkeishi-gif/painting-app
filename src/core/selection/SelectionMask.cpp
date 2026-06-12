#include "core/selection/SelectionMask.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>

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
      m_mask[indexOf(x, y)] = 255U;
    }
  }

  m_hasSelection = true;
  m_bounds = Rect {left, top, right - left + 1, bottom - top + 1};
  return before != *this;
}

bool SelectionMask::setPixels(const std::vector<std::uint8_t>& pixels) {
  if (pixels.size() != m_mask.size()) {
    return false;
  }

  SelectionMask before = *this;
  m_mask = pixels;

  int minX = m_width;
  int minY = m_height;
  int maxX = -1;
  int maxY = -1;
  for (int y = 0; y < m_height; ++y) {
    for (int x = 0; x < m_width; ++x) {
      if (m_mask[indexOf(x, y)] == 0U) {
        continue;
      }
      minX = std::min(minX, x);
      minY = std::min(minY, y);
      maxX = std::max(maxX, x);
      maxY = std::max(maxY, y);
    }
  }

  if (maxX < minX || maxY < minY) {
    m_hasSelection = false;
    m_bounds.reset();
  } else {
    m_hasSelection = true;
    m_bounds = Rect {minX, minY, maxX - minX + 1, maxY - minY + 1};
  }
  return before != *this;
}

bool SelectionMask::invert() {
  if (m_width <= 0 || m_height <= 0) {
    return false;
  }

  SelectionMask before = *this;
  for (std::uint8_t& value : m_mask) {
    value = value == 0U ? 1U : 0U;
  }

  int minX = m_width;
  int minY = m_height;
  int maxX = -1;
  int maxY = -1;
  for (int y = 0; y < m_height; ++y) {
    for (int x = 0; x < m_width; ++x) {
      if (m_mask[indexOf(x, y)] == 0U) {
        continue;
      }
      minX = std::min(minX, x);
      minY = std::min(minY, y);
      maxX = std::max(maxX, x);
      maxY = std::max(maxY, y);
    }
  }

  if (maxX < minX || maxY < minY) {
    m_hasSelection = false;
    m_bounds.reset();
  } else {
    m_hasSelection = true;
    m_bounds = Rect {minX, minY, maxX - minX + 1, maxY - minY + 1};
  }
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

// ── 共通: マスク変更後に m_hasSelection / m_bounds を再計算 ───────────────
static void recomputeBounds(
    std::vector<std::uint8_t>& mask, int width, int height,
    bool& hasSelection, std::optional<core::Rect>& bounds) {
  int minX = width, minY = height, maxX = -1, maxY = -1;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (mask[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)] != 0U) {
        if (x < minX) minX = x;
        if (y < minY) minY = y;
        if (x > maxX) maxX = x;
        if (y > maxY) maxY = y;
      }
    }
  }
  if (maxX < minX) {
    hasSelection = false;
    bounds.reset();
  } else {
    hasSelection = true;
    bounds = core::Rect {minX, minY, maxX - minX + 1, maxY - minY + 1};
  }
}

bool SelectionMask::applyRect(SelectionOp op, const Rect& rect) {
  if (op == SelectionOp::New) {
    return setRect(rect);
  }
  // Build temp mask for the rectangle
  const int total = m_width * m_height;
  if (total <= 0) return false;
  std::vector<std::uint8_t> tmp(static_cast<std::size_t>(total), 0U);
  const int left   = std::clamp(rect.x, 0, m_width - 1);
  const int top    = std::clamp(rect.y, 0, m_height - 1);
  const int right  = std::clamp(rect.x + std::max(0, rect.width)  - 1, 0, m_width - 1);
  const int bottom = std::clamp(rect.y + std::max(0, rect.height) - 1, 0, m_height - 1);
  for (int y = top; y <= bottom; ++y) {
    for (int x = left; x <= right; ++x) {
      tmp[static_cast<std::size_t>(y) * static_cast<std::size_t>(m_width) + static_cast<std::size_t>(x)] = 255U;
    }
  }
  return applyPixels(op, tmp);
}

bool SelectionMask::applyPixels(SelectionOp op, const std::vector<std::uint8_t>& pixels) {
  if (pixels.size() != m_mask.size()) return false;
  if (op == SelectionOp::New) {
    return setPixels(pixels);
  }
  const std::size_t n = m_mask.size();
  std::vector<std::uint8_t> before = m_mask;
  if (op == SelectionOp::Add) {
    for (std::size_t i = 0; i < n; ++i) {
      if (pixels[i] != 0U) m_mask[i] = 255U;
    }
  } else if (op == SelectionOp::Subtract) {
    for (std::size_t i = 0; i < n; ++i) {
      if (pixels[i] != 0U) m_mask[i] = 0U;
    }
  } else { // Intersect
    for (std::size_t i = 0; i < n; ++i) {
      m_mask[i] = (m_mask[i] != 0U && pixels[i] != 0U) ? 255U : 0U;
    }
  }
  if (m_mask == before) return false;
  recomputeBounds(m_mask, m_width, m_height, m_hasSelection, m_bounds);
  return true;
}

std::uint8_t SelectionMask::maskValue(int x, int y) const noexcept {
  if (!inBounds(x, y)) return 0U;
  return m_mask[indexOf(x, y)];
}

bool SelectionMask::translate(int dx, int dy) {
  if (m_width <= 0 || m_height <= 0 || !m_hasSelection) return false;
  if (dx == 0 && dy == 0) return false;

  std::vector<std::uint8_t> newMask(m_mask.size(), 0U);
  for (int y = 0; y < m_height; ++y) {
    const int sy = y - dy;
    if (sy < 0 || sy >= m_height) continue;
    for (int x = 0; x < m_width; ++x) {
      const int sx = x - dx;
      if (sx < 0 || sx >= m_width) continue;
      newMask[indexOf(x, y)] = m_mask[indexOf(sx, sy)];
    }
  }
  m_mask = newMask;
  recomputeBounds(m_mask, m_width, m_height, m_hasSelection, m_bounds);
  return true;
}

// ボックスフィルタ3回適用 ≈ ガウスぼかし
bool SelectionMask::feather(int radius) {
  if (radius <= 0 || m_width <= 0 || m_height <= 0) return false;

  // uint8 → float
  std::vector<float> buf(m_mask.size());
  for (std::size_t i = 0; i < m_mask.size(); ++i) {
    buf[i] = static_cast<float>(m_mask[i]) / 1.0f;  // 0 or 1
  }

  // 分離型ボックスフィルタを3回
  const int r = radius;
  std::vector<float> tmp(m_mask.size());
  for (int pass = 0; pass < 3; ++pass) {
    // 水平方向
    for (int y = 0; y < m_height; ++y) {
      for (int x = 0; x < m_width; ++x) {
        float sum = 0.0f;
        int count = 0;
        for (int dx = -r; dx <= r; ++dx) {
          const int nx = x + dx;
          if (nx >= 0 && nx < m_width) {
            sum += buf[indexOf(nx, y)];
            ++count;
          }
        }
        tmp[indexOf(x, y)] = count > 0 ? sum / static_cast<float>(count) : 0.0f;
      }
    }
    // 垂直方向
    for (int y = 0; y < m_height; ++y) {
      for (int x = 0; x < m_width; ++x) {
        float sum = 0.0f;
        int count = 0;
        for (int dy = -r; dy <= r; ++dy) {
          const int ny = y + dy;
          if (ny >= 0 && ny < m_height) {
            sum += tmp[indexOf(x, ny)];
            ++count;
          }
        }
        buf[indexOf(x, y)] = count > 0 ? sum / static_cast<float>(count) : 0.0f;
      }
    }
  }

  // float → uint8 (0–255)
  const std::vector<std::uint8_t> before = m_mask;
  for (std::size_t i = 0; i < m_mask.size(); ++i) {
    m_mask[i] = static_cast<std::uint8_t>(std::clamp(static_cast<int>(buf[i] * 255.0f + 0.5f), 0, 255));
  }
  recomputeBounds(m_mask, m_width, m_height, m_hasSelection, m_bounds);
  return m_mask != before;
}

bool SelectionMask::expand(int radius) {
  if (radius <= 0 || m_width <= 0 || m_height <= 0 || !m_hasSelection) return false;

  // モルフォロジー膨張: 各ピクセルを中心とする円形近傍の最大値を取る
  const std::vector<std::uint8_t> src = m_mask;
  const int r2 = radius * radius;
  for (int y = 0; y < m_height; ++y) {
    for (int x = 0; x < m_width; ++x) {
      if (src[indexOf(x, y)] != 0U) continue;  // すでに選択済みはスキップ
      bool hit = false;
      for (int dy = -radius; dy <= radius && !hit; ++dy) {
        const int ny = y + dy;
        if (ny < 0 || ny >= m_height) continue;
        for (int dx = -radius; dx <= radius && !hit; ++dx) {
          if (dx * dx + dy * dy > r2) continue;
          const int nx = x + dx;
          if (nx < 0 || nx >= m_width) continue;
          if (src[indexOf(nx, ny)] != 0U) hit = true;
        }
      }
      if (hit) m_mask[indexOf(x, y)] = 255U;
    }
  }
  recomputeBounds(m_mask, m_width, m_height, m_hasSelection, m_bounds);
  return true;
}

bool SelectionMask::contract(int radius) {
  if (radius <= 0 || m_width <= 0 || m_height <= 0 || !m_hasSelection) return false;

  // モルフォロジー収縮: 各ピクセルの円形近傍に非選択が1つでもあれば除外
  const std::vector<std::uint8_t> src = m_mask;
  const int r2 = radius * radius;
  for (int y = 0; y < m_height; ++y) {
    for (int x = 0; x < m_width; ++x) {
      if (src[indexOf(x, y)] == 0U) continue;  // 非選択はスキップ
      bool removeIt = false;
      for (int dy = -radius; dy <= radius && !removeIt; ++dy) {
        const int ny = y + dy;
        if (ny < 0 || ny >= m_height) { removeIt = true; continue; }
        for (int dx = -radius; dx <= radius && !removeIt; ++dx) {
          if (dx * dx + dy * dy > r2) continue;
          const int nx = x + dx;
          if (nx < 0 || nx >= m_width) { removeIt = true; continue; }
          if (src[indexOf(nx, ny)] == 0U) removeIt = true;
        }
      }
      if (removeIt) m_mask[indexOf(x, y)] = 0U;
    }
  }
  recomputeBounds(m_mask, m_width, m_height, m_hasSelection, m_bounds);
  return true;
}

bool SelectionMask::smooth(int radius) {
  if (radius <= 0 || m_width <= 0 || m_height <= 0 || !m_hasSelection) return false;
  // ガウスぼかし後に 50% 閾値で再二値化
  std::vector<float> buf(m_mask.size());
  for (std::size_t i = 0; i < m_mask.size(); ++i) {
    buf[i] = m_mask[i] != 0U ? 1.0f : 0.0f;
  }
  const int r = radius;
  std::vector<float> tmp(m_mask.size());
  for (int pass = 0; pass < 3; ++pass) {
    for (int y = 0; y < m_height; ++y) {
      for (int x = 0; x < m_width; ++x) {
        float sum = 0.0f; int cnt = 0;
        for (int dx = -r; dx <= r; ++dx) {
          const int nx = x + dx;
          if (nx >= 0 && nx < m_width) { sum += buf[indexOf(nx, y)]; ++cnt; }
        }
        tmp[indexOf(x, y)] = cnt > 0 ? sum / static_cast<float>(cnt) : 0.0f;
      }
    }
    for (int y = 0; y < m_height; ++y) {
      for (int x = 0; x < m_width; ++x) {
        float sum = 0.0f; int cnt = 0;
        for (int dy = -r; dy <= r; ++dy) {
          const int ny = y + dy;
          if (ny >= 0 && ny < m_height) { sum += tmp[indexOf(x, ny)]; ++cnt; }
        }
        buf[indexOf(x, y)] = cnt > 0 ? sum / static_cast<float>(cnt) : 0.0f;
      }
    }
  }
  const std::vector<std::uint8_t> before = m_mask;
  for (std::size_t i = 0; i < m_mask.size(); ++i) {
    m_mask[i] = buf[i] >= 0.5f ? 1U : 0U;
  }
  recomputeBounds(m_mask, m_width, m_height, m_hasSelection, m_bounds);
  return m_mask != before;
}

} // namespace core
