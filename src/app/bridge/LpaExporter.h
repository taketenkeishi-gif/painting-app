#pragma once

#include <string>

namespace core { class Document; }

namespace app::lpa {

// ─────────────────────────────────────────────────────────────────────────────
// LpaExporter  — LayeredPaintApp ネイティブプロジェクト形式 (.lpa) 保存
//
// 形式: ZIP コンテナ
//   project.json   … キャンバス・レイヤーメタデータ (JSON, version=1)
//   layers/<id>.png       … ラスターレイヤーのピクセルデータ
//   layers/<id>_mask.png  … レイヤーマスク (存在する場合)
//
// ベクターパスと調整レイヤーパラメータは project.json に直接埋め込む。
// ─────────────────────────────────────────────────────────────────────────────
struct SaveResult {
    bool        success {false};
    std::string error;
};

SaveResult saveLpa(const core::Document& doc, const std::string& path);

} // namespace app::lpa
