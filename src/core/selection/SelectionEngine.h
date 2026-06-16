#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "core/buffer/PixelBuffer.h"
#include "core/selection/SelectionMask.h"
#include "core/selection/SelectionRequest.h"
#include "core/selection/SelectionResult.h"
#include "core/selection/providers/ISelectionProvider.h"

namespace core {

class Document;

// ─────────────────────────────────────────────────────────────────────────────
// SelectionEngine
//
// Photoshop 相当の統合選択エンジン。
//
// ── プロバイダーパイプライン (新規) ──────────────────────────────────────────
//   Tool → SelectionRequest → execute() → ISelectionProvider → SelectionResult
//   → SelectionOp 合成 → Document::SelectionMask にコミット
//
// ── 互換 API (既存) ─────────────────────────────────────────────────────────
//   applyRect / applyPixels / floodFill など従来の直接操作は引き続き使用可。
// ─────────────────────────────────────────────────────────────────────────────
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

  // ── プロバイダー設定 ──────────────────────────────────────────────────────
  /// プロバイダーを設定する。nullptr を渡すと既存プロバイダーを解除する。
  void setProvider(std::unique_ptr<ISelectionProvider> provider) noexcept;

  // ── プロバイダーパイプライン実行 ─────────────────────────────────────────
  /// SelectionRequest をプロバイダーに渡して実行し、結果を Document に反映する。
  /// @param request  選択要求 (タイプ・ジオメトリ・パラメーター)
  /// @param reference ピクセル参照バッファ (MagicWand / AI が使用)
  /// @return true = 選択範囲が変化した
  bool execute(const SelectionRequest& request, const PixelBuffer& reference);

  /// 最後の execute() で得た SelectionResult を返す。
  /// execute() 未呼び出し・失敗時は nullptr。
  const SelectionResult* lastResult() const noexcept { return m_lastResult.get(); }

  // ── 選択適用 (互換 API) ───────────────────────────────────────────────────
  bool applyRect(SelectionOp op, const Rect& rect);
  bool applyPixels(SelectionOp op, const std::vector<std::uint8_t>& pixels,
                   SelectionSource source = SelectionSource::UserDrawn);
  bool floodFill(const PixelBuffer& reference, int x, int y,
                 int threshold, SelectionOp op);

  // ── 後処理オプション (互換 API) ───────────────────────────────────────────
  bool applyRefinement(const RefinementOptions& opts);
  bool expand(int radius);
  bool contract(int radius);
  bool smooth(int radius);
  bool feather(int radius);

  // ── 全体操作 (互換 API) ───────────────────────────────────────────────────
  bool selectAll();
  bool deselect();
  bool invert();

  // ── アクセス ──────────────────────────────────────────────────────────────
  const SelectionMask* mask() const noexcept;

private:
  SelectionMask* mutableMask() noexcept;

  Document* m_doc {nullptr};
  std::unique_ptr<ISelectionProvider> m_provider;
  std::unique_ptr<SelectionResult>    m_lastResult;
};

} // namespace core
