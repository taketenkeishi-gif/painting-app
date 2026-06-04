#pragma once

#include <string_view>

#include "core/buffer/PixelBuffer.h"
#include "core/selection/SelectionRequest.h"
#include "core/selection/SelectionResult.h"

namespace core {

// ─────────────────────────────────────────────────────────────────────────────
// ISelectionProvider
//
// 選択プロバイダーの純粋仮想インターフェース。
//
// 実装一覧:
//   ClassicProvider — 矩形/ラッソ/自動選択 (現在)
//   AIProvider      — SAM2 / BiRefNet / MODNet 等 (将来)
//
// Contract:
//   • execute() は SelectionOp の適用前の「新規マスク」を返す。
//     既存選択との合成は SelectionEngine が担う。
//   • reference は MagicWand / AI が参照する画像バッファ。
//   • canHandle() で対応 Type かどうかを確認できる。
// ─────────────────────────────────────────────────────────────────────────────
class ISelectionProvider {
public:
  virtual ~ISelectionProvider() = default;

  /// このプロバイダーが対応している SelectionRequest::Type か返す。
  virtual bool canHandle(SelectionRequest::Type type) const noexcept = 0;

  /// 選択を実行し SelectionResult を返す。
  /// @param request  選択要求 (ジオメトリ・パラメーター)
  /// @param reference ピクセル参照バッファ (MagicWand / AI が使用)
  /// @param canvasW  キャンバス幅
  /// @param canvasH  キャンバス高さ
  virtual SelectionResult execute(const SelectionRequest& request,
                                   const PixelBuffer&      reference,
                                   int canvasW,
                                   int canvasH) const = 0;

  /// プロバイダー名 (ログ・デバッグ用)
  virtual std::string_view name() const noexcept = 0;
};

} // namespace core
