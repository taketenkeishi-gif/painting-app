#pragma once

#include <cstdint>
#include <vector>

namespace core {

class SelectionMask;
struct SelectionResult;

// ─────────────────────────────────────────────────────────────────────────────
// MaskExporter
//
// SelectionMask / SelectionResult を ComfyUI インペイントや外部ツール向けに
// バイト配列として書き出すユーティリティ。
//
// フォーマット:
//   Binary  — 選択あり = 255, なし = 0  (8bit グレースケール)
//   Soft    — 中間値をそのまま保持      (8bit グレースケール, フェザー対応)
//   Alpha   — Soft と同じだが意味的に「アルファチャンネル」として扱う
//
// 使い方:
//   auto bytes = MaskExporter::exportMask(mask, MaskExporter::Format::Soft);
//   // bytes を QImage に渡すか HTTP multipart で送信する
//
// すべてのメソッドは static — インスタンス化不要。
// ─────────────────────────────────────────────────────────────────────────────
class MaskExporter {
public:
  MaskExporter() = delete;

  enum class Format {
    Binary,  ///< 選択あり=255 / なし=0 のハードマスク
    Soft,    ///< 中間値をそのまま (フェザー / AA 対応)
    Alpha,   ///< Soft と同義。アルファチャンネル用途であることを明示
  };

  /// SelectionMask から flat byte array [width * height] を生成する。
  static std::vector<std::uint8_t> exportMask(const SelectionMask& mask,
                                               Format format);

  /// SelectionResult から flat byte array を生成する。
  /// result.hasConfidenceMap() == true かつ format != Binary の場合は
  /// confidenceMap をそのまま [0-255] にスケールして返す。
  /// それ以外は exportMask(result.mask, format) と同じ。
  static std::vector<std::uint8_t> exportMask(const SelectionResult& result,
                                               Format format);
};

} // namespace core
