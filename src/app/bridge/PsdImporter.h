#pragma once
#include <string>

namespace core { class Document; }

namespace app::psd {

struct ImportResult {
  bool success {false};
  std::string error;
};

// PSD v1 インポーター
// ラスターレイヤーのみ対応（調整レイヤー・スマートオブジェクトはフラット化）
ImportResult importPsd(const std::string& path, core::Document& doc);

} // namespace app::psd
