#pragma once

#include "core/color/Color.h"
#include "core/selection/SelectionResult.h"
#include "core/selection/providers/ISelectionProvider.h"

namespace core {

// ─────────────────────────────────────────────────────────────────────────────
// ClassicProvider
//
// 従来の選択ロジック (矩形/ラッソ/多角形ラッソ/自動選択) を
// ISelectionProvider として実装したもの。
//
// RectSelectionTool が直接 SelectionMask を操作していたロジックを
// このクラスに集約し、ツールから分離する。
//
// 対応 Type:
//   Rectangle, FreeLasso, PolygonLasso, MagicWand, Object
//
// 非対応 (AI が担う):
//   Ellipse (将来), Subject, Semantic, AI
// ─────────────────────────────────────────────────────────────────────────────
class ClassicProvider final : public ISelectionProvider {
public:
  ClassicProvider() = default;
  ~ClassicProvider() override = default;

  bool canHandle(SelectionRequest::Type type) const noexcept override;

  SelectionResult execute(const SelectionRequest& request,
                           const PixelBuffer&      reference,
                           int canvasW,
                           int canvasH) const override;

  std::string_view name() const noexcept override { return "Classic"; }

private:
  // ── ラスタライズ ─────────────────────────────────────────────────────────
  static SelectionMask rasterizeRect   (const Rect& rect, int w, int h);
  static SelectionMask rasterizePolygon(const std::vector<Point>& pts,
                                         int w, int h, bool antiAlias);

  /// スキャンライン多角形塗りつぶし (0/255)
  static std::vector<std::uint8_t> scanFillPolygon(
      const std::vector<Point>& poly, int w, int h);
  /// スキャンライン + 1px ボックスフィルタ AA
  static std::vector<std::uint8_t> antiAliasedFillPolygon(
      const std::vector<Point>& poly, int w, int h);

  // ── 自動選択 ─────────────────────────────────────────────────────────────
  /// Lab色差エッジアウェアフラッドフィル。SelectionResult を直接返す。
  static SelectionResult floodFillSelect(const PixelBuffer& reference,
                                          const Point& seed,
                                          int tolerance,
                                          bool contiguous,
                                          bool antiAlias,
                                          bool edgeAware,
                                          int w, int h);
};

} // namespace core
