#pragma once
#include <string>
#include <vector>

namespace core { class Document; }

namespace app::psd {

// PSD (Photoshop Document) エクスポーター
// PSD v1 (最大 30000x30000px, 8bit/channel, RGBA)
// 全レイヤーをレイヤーとして書き出す。合成済み画像も含む。
struct ExportResult {
  bool success {false};
  std::string error;
};

ExportResult exportPsd(const core::Document& doc, const std::string& path);

} // namespace app::psd
