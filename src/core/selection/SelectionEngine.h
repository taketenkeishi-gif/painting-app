#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "core/buffer/PixelBuffer.h"
#include "core/selection/SelectionMask.h"

namespace core {

class Document;

/// Photoshop 相当の統合選択エンジン。
/// 全選択操作はこのクラスを通じて行い、Document の SelectionMask に commit する。
class SelectionEngine {
public:
  enum class SelectionSource {
    UserDrawn,       ///< ユーザー直接描画（矩形・ラッソ等）
    ColorBased,      ///< 色域選択・自動選択
    AiGenerated,     ///< AI セグメンテーション結果
    ObjectDetection, ///< オブジェクト検出結果
  };

  struct RefinementOptions {
    int featherRadius  {0};
    int smoothRadius   {0};
    int expandPixels   {0};
    int contractPixels {0};
  };

  explicit SelectionEngine() = default;

  void setDocument(Document* doc) noexcept { m_doc = doc; }

  // ── 選択適用 ──────────────────────────────────────────────────────────────
  bool applyRect(SelectionOp op, const Rect& rect);
  bool applyPixels(SelectionOp op, const std::vector<std::uint8_t>& pixels,
                   SelectionSource source = SelectionSource::UserDrawn);
  bool floodFill(const PixelBuffer& reference, int x, int y,
                 int threshold, SelectionOp op);

  // ── 後処理オプション ──────────────────────────────────────────────────────
  bool applyRefinement(const RefinementOptions& opts);
  bool expand(int radius);
  bool contract(int radius);
  bool smooth(int radius);
  bool feather(int radius);

  // ── 全体操作 ──────────────────────────────────────────────────────────────
  bool selectAll();
  bool deselect();
  bool invert();

  // ── アクセス ──────────────────────────────────────────────────────────────
  const SelectionMask* mask() const noexcept;

private:
  SelectionMask* mutableMask() noexcept;

  Document* m_doc {nullptr};
};

} // namespace core
