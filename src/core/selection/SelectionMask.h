#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "core/common/Rect.h"

namespace core {

/// 選択操作の種類 (RectSelectionTool と SelectionMask が共用)
enum class SelectionOp {
  New,       ///< 既存選択を破棄して新規作成
  Add,       ///< 既存選択に追加 (Shift)
  Subtract,  ///< 既存選択から除外 (Alt)
  Intersect, ///< 既存選択との交差 (Shift+Alt)
};

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
  bool setPixels(const std::vector<std::uint8_t>& pixels);
  bool invert();

  /// 選択範囲を (dx, dy) だけ平行移動する。範囲外はクリップ。
  bool translate(int dx, int dy);
  /// ガウスぼかし近似でエッジをフェザリングする（radius ピクセル）。
  bool feather(int radius);
  /// 選択範囲を radius ピクセル拡張する（モルフォロジー膨張）。
  bool expand(int radius);
  /// 選択範囲を radius ピクセル縮小する（モルフォロジー収縮）。
  bool contract(int radius);
  /// 選択エッジをスムージングする（ガウスぼかし後に再二値化）。
  bool smooth(int radius);
  /// マスク値を返す（0=非選択、255=完全選択、中間値=フェザー部分）。
  std::uint8_t maskValue(int x, int y) const noexcept;

  /// SelectionOp を考慮した矩形適用。New = 従来の setRect 相当。
  bool applyRect(SelectionOp op, const Rect& rect);
  /// SelectionOp を考慮したピクセルマスク適用。
  bool applyPixels(SelectionOp op, const std::vector<std::uint8_t>& pixels);

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
