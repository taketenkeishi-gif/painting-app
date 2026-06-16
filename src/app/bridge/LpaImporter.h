#pragma once

#include <memory>
#include <string>

namespace core { class Document; }

namespace app::lpa {

// ─────────────────────────────────────────────────────────────────────────────
// LpaImporter  — .lpa プロジェクトファイルの読み込み
// ─────────────────────────────────────────────────────────────────────────────
struct LoadResult {
    bool                          success {false};
    std::string                   error;
    std::unique_ptr<core::Document> document;   ///< 成功時のみ非 null
};

LoadResult loadLpa(const std::string& path);

} // namespace app::lpa
