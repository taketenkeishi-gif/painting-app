#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/buffer/PixelBuffer.h"
#include "core/common/Point.h"
#include "core/selection/SelectionMask.h"

namespace core::ai {

// ─────────────────────────────────────────────────────────────────────────────
// OnnxSegEngine  — SAM2 ローカル推論エンジン
//
// PAINT_USE_ONNX=ON 時のみ実際の推論を行う。OFF 時は isLoaded()==false を返す
// スタブになるため、常にインクルード可能。
//
// 使い方:
//   OnnxSegEngine engine({encoderPath, decoderPath});
//   if (engine.isLoaded()) {
//     engine.encodeImage(canvas);                 // 画像変化時に再呼出し
//     auto r = engine.decode(pos, neg, W, H, 1);  // クリックごと
//     if (r.valid) applyMask(r.mask);
//   }
//
// granularity 0=小領域（髪一房など） / 1=中 / 2=オブジェクト / 3=被写体全体
// ─────────────────────────────────────────────────────────────────────────────
class OnnxSegEngine {
public:
    struct Config {
        std::string encoderModelPath;
        std::string decoderModelPath;
    };

    struct DecodeResult {
        SelectionMask mask;
        float         iou   {0.f};
        bool          valid {false};
    };

    explicit OnnxSegEngine(const Config& config);
    ~OnnxSegEngine();

    OnnxSegEngine(const OnnxSegEngine&)            = delete;
    OnnxSegEngine& operator=(const OnnxSegEngine&) = delete;

    bool        isLoaded()  const noexcept;
    std::string loadError() const noexcept;

    // 画像全体をエンコード（重い: ~200-500 ms）— 画像が変わった時だけ呼ぶ
    bool encodeImage(const PixelBuffer& image);
    bool hasEmbedding() const noexcept;

    // ポイントからマスクをデコード（軽い: ~10-50 ms）
    // granularity: 0=最小 … 3=最大, -1=IoU最大を自動選択
    DecodeResult decode(
        const std::vector<Point>& positivePoints,
        const std::vector<Point>& negativePoints,
        int originalWidth,
        int originalHeight,
        int granularity = 1) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace core::ai
