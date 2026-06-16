#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/buffer/PixelBuffer.h"

namespace core::ai {

// ─────────────────────────────────────────────────────────────────────────────
// UpscaleEngine — AI 超解像エンジン
//
// PAINT_USE_ONNX=ON + モデルファイルあり → Real-ESRGAN タイル推論
// それ以外                               → バイリニア補間スタブ
//
// 使い方:
//   UpscaleEngine engine({modelPath});          // modelPath="" でスタブ
//   auto dst = engine.upscale(src, 4, callback);
//
// scanModels() で <exeDir>/models/ を走査し、利用可能なモデルを列挙できる。
// ─────────────────────────────────────────────────────────────────────────────
class UpscaleEngine {
public:
    struct ModelInfo {
        std::string path;
        std::string displayName;
        int         nativeScale {4};  // モデルのネイティブ倍率（2 or 4）
    };

    struct Config {
        std::string modelPath;  // 空 = AI なし（バイリニアスタブ）
    };

    using ProgressCallback = std::function<void(int percent)>;

    explicit UpscaleEngine(const Config& config = {});
    ~UpscaleEngine();

    UpscaleEngine(const UpscaleEngine&)            = delete;
    UpscaleEngine& operator=(const UpscaleEngine&) = delete;

    bool        isAiLoaded()  const noexcept;  // ONNX モデルが使用可能か
    int         nativeScale() const noexcept;  // ロードしたモデルのネイティブ倍率
    std::string loadError()   const noexcept;

    // 拡大実行（同期）
    //   scale: 2 or 4  (モデルが 2x の場合は 2 回連続適用で 4x を達成)
    //   progress: 0〜100 を通知するコールバック（スレッドから呼ばれる場合は注意）
    PixelBuffer upscale(const PixelBuffer& src, int scale,
                        ProgressCallback progress = nullptr) const;

    // modelsDir 内の ONNX アップスケールモデルを列挙
    static std::vector<ModelInfo> scanModels(const std::string& modelsDir);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace core::ai
