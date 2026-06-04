#include "core/selection/MaskExporter.h"

#include <algorithm>
#include <cmath>

#include "core/selection/SelectionMask.h"
#include "core/selection/SelectionResult.h"

namespace core {

// ─────────────────────────────────────────────────────────────────────────────
// exportMask (SelectionMask)
// ─────────────────────────────────────────────────────────────────────────────
std::vector<std::uint8_t> MaskExporter::exportMask(const SelectionMask& mask,
                                                     Format format) {
  const int w = mask.width();
  const int h = mask.height();
  const std::size_t total = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
  std::vector<std::uint8_t> out(total, 0U);

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const std::uint8_t v = mask.maskValue(x, y);
      const std::size_t i =
          static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
          static_cast<std::size_t>(x);

      if (format == Format::Binary) {
        out[i] = (v >= 128U) ? 255U : 0U;
      } else {
        // Soft / Alpha: 中間値をそのまま
        out[i] = v;
      }
    }
  }
  return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// exportMask (SelectionResult)
// ─────────────────────────────────────────────────────────────────────────────
std::vector<std::uint8_t> MaskExporter::exportMask(const SelectionResult& result,
                                                     Format format) {
  // AI 確信度マップが存在し、ソフト/アルファ形式を要求されている場合は
  // confidenceMap を [0-255] にスケールして返す
  if (format != Format::Binary && result.hasConfidenceMap()) {
    const auto& cm = result.confidenceMap;
    std::vector<std::uint8_t> out(cm.size());
    for (std::size_t i = 0; i < cm.size(); ++i) {
      out[i] = static_cast<std::uint8_t>(
          std::clamp(static_cast<int>(cm[i] * 255.0f + 0.5f), 0, 255));
    }
    return out;
  }

  return exportMask(result.mask, format);
}

} // namespace core
