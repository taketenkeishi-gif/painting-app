#include "core/ai/UpscaleEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>

#ifdef PAINT_USE_ONNX
#  include <onnxruntime_cxx_api.h>
#  include <mutex>
#endif

namespace core::ai {

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// 内部ユーティリティ
// ─────────────────────────────────────────────────────────────────────────────
namespace {

inline uint8_t clamp8(float v) {
    return static_cast<uint8_t>(std::min(255.f, std::max(0.f, v + 0.5f)));
}

Color sampleBilinear(const PixelBuffer& src, float sx, float sy) {
    int x0 = static_cast<int>(std::floor(sx));
    int y0 = static_cast<int>(std::floor(sy));
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    x0 = std::max(0, std::min(x0, src.width()  - 1));
    y0 = std::max(0, std::min(y0, src.height() - 1));
    x1 = std::max(0, std::min(x1, src.width()  - 1));
    y1 = std::max(0, std::min(y1, src.height() - 1));
    const float dx = sx - std::floor(sx);
    const float dy = sy - std::floor(sy);
    const Color c00 = src.pixel(x0, y0);
    const Color c10 = src.pixel(x1, y0);
    const Color c01 = src.pixel(x0, y1);
    const Color c11 = src.pixel(x1, y1);
    return {
        clamp8((1-dy)*((1-dx)*c00.r + dx*c10.r) + dy*((1-dx)*c01.r + dx*c11.r)),
        clamp8((1-dy)*((1-dx)*c00.g + dx*c10.g) + dy*((1-dx)*c01.g + dx*c11.g)),
        clamp8((1-dy)*((1-dx)*c00.b + dx*c10.b) + dy*((1-dx)*c01.b + dx*c11.b)),
        clamp8((1-dy)*((1-dx)*c00.a + dx*c10.a) + dy*((1-dx)*c01.a + dx*c11.a))
    };
}

PixelBuffer bilinearUpscale(const PixelBuffer& src, int scale,
                             UpscaleEngine::ProgressCallback progress) {
    const int dstW = src.width()  * scale;
    const int dstH = src.height() * scale;
    PixelBuffer dst(dstW, dstH);
    const float invScale = 1.0f / scale;
    for (int y = 0; y < dstH; ++y) {
        const float sy = (y + 0.5f) * invScale - 0.5f;
        for (int x = 0; x < dstW; ++x) {
            const float sx = (x + 0.5f) * invScale - 0.5f;
            dst.setPixel(x, y, sampleBilinear(src, sx, sy));
        }
        if (progress && (y & 63) == 0) {
            progress(y * 100 / dstH);
        }
    }
    if (progress) progress(100);
    return dst;
}

// ファイル名をもとにネイティブ倍率を推測
int guessScale(const std::string& stem) {
    std::string lower = stem;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower.find("x2") != std::string::npos || lower.find("2x") != std::string::npos) return 2;
    if (lower.find("x8") != std::string::npos || lower.find("8x") != std::string::npos) return 8;
    return 4;  // Real-ESRGAN の大半は ×4
}


} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Impl
// ─────────────────────────────────────────────────────────────────────────────
struct UpscaleEngine::Impl {
    bool        loaded     {false};
    int         scale      {4};
    std::string error;

#ifdef PAINT_USE_ONNX
    Ort::Env            env {ORT_LOGGING_LEVEL_ERROR, "upscale"};
    Ort::SessionOptions opts;
    std::unique_ptr<Ort::Session> session;
    std::string inputName;
    std::string outputName;

    void load(const std::string& modelPath) {
        if (modelPath.empty()) return;
        try {
#  ifdef _WIN32
            std::wstring wpath(modelPath.begin(), modelPath.end());
            session = std::make_unique<Ort::Session>(env, wpath.c_str(), opts);
#  else
            session = std::make_unique<Ort::Session>(env, modelPath.c_str(), opts);
#  endif
            Ort::AllocatorWithDefaultOptions alloc;
            inputName  = session->GetInputNameAllocated(0, alloc).get();
            outputName = session->GetOutputNameAllocated(0, alloc).get();

            // ネイティブ倍率を入出力シェイプから推測
            auto inShape  = session->GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
            auto outShape = session->GetOutputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
            if (inShape.size() == 4 && outShape.size() == 4
                && inShape[2] > 0 && outShape[2] > 0) {
                scale = static_cast<int>(outShape[2] / inShape[2]);
            }
            loaded = true;
        } catch (const Ort::Exception& e) {
            error = e.what();
        }
    }

    // タイルサイズ（モデル入力サイズに合わせて調整可能）
    static constexpr int kTile    = 128;
    static constexpr int kPad     = 8;   // タイル間オーバーラップ

    PixelBuffer runTiled(const PixelBuffer& src, int targetScale,
                         ProgressCallback progress) const {
        if (!session) return {};

        // タイルを ONNX 入力テンソルに変換して推論
        const int sw = src.width();
        const int sh = src.height();
        const int dw = sw * targetScale;
        const int dh = sh * targetScale;
        PixelBuffer dst(dw, dh);

        const int tileStep = kTile;
        const int nTilesX = (sw + tileStep - 1) / tileStep;
        const int nTilesY = (sh + tileStep - 1) / tileStep;
        const int totalTiles = nTilesX * nTilesY;
        int doneCount = 0;

        for (int ty = 0; ty < nTilesY; ++ty) {
            for (int tx = 0; tx < nTilesX; ++tx) {
                // ソースタイル領域（パディング含む）
                int sx0 = std::max(0, tx * tileStep - kPad);
                int sy0 = std::max(0, ty * tileStep - kPad);
                int sx1 = std::min(sw, (tx + 1) * tileStep + kPad);
                int sy1 = std::min(sh, (ty + 1) * tileStep + kPad);
                int tw  = sx1 - sx0;
                int th  = sy1 - sy0;

                // PixelBuffer → float32 RGB テンソル [1, 3, th, tw]
                std::vector<float> input(3 * th * tw);
                for (int y = 0; y < th; ++y) {
                    for (int x = 0; x < tw; ++x) {
                        Color c = src.pixel(sx0 + x, sy0 + y);
                        int idx = y * tw + x;
                        input[0 * th * tw + idx] = c.r / 255.f;
                        input[1 * th * tw + idx] = c.g / 255.f;
                        input[2 * th * tw + idx] = c.b / 255.f;
                    }
                }

                std::array<int64_t, 4> inShape{1, 3, th, tw};
                Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(
                    OrtArenaAllocator, OrtMemTypeDefault);
                auto inTensor = Ort::Value::CreateTensor<float>(
                    memInfo, input.data(), input.size(),
                    inShape.data(), inShape.size());

                const char* inNames[]  = {inputName.c_str()};
                const char* outNames[] = {outputName.c_str()};
                auto outputs = session->Run(Ort::RunOptions{nullptr},
                                            inNames, &inTensor, 1,
                                            outNames, 1);

                const float* outPtr   = outputs[0].GetTensorData<float>();
                const int    outh     = th * scale;
                const int    outw     = tw * scale;

                // 書き込み先: 元のタイル範囲に対応する出力領域
                int px0 = (tx * tileStep - sx0) * scale;  // パディングオフセット
                int py0 = (ty * tileStep - sy0) * scale;
                int px1 = std::min(outw, px0 + tileStep * scale);
                int py1 = std::min(outh, py0 + tileStep * scale);
                int dx0 = tx * tileStep * scale;
                int dy0 = ty * tileStep * scale;

                for (int y = py0; y < py1; ++y) {
                    for (int x = px0; x < px1; ++x) {
                        int idx = y * outw + x;
                        Color c {
                            clamp8(outPtr[0 * outh * outw + idx] * 255.f),
                            clamp8(outPtr[1 * outh * outw + idx] * 255.f),
                            clamp8(outPtr[2 * outh * outw + idx] * 255.f),
                            255
                        };
                        int dstX = dx0 + (x - px0);
                        int dstY = dy0 + (y - py0);
                        if (dstX < dw && dstY < dh) {
                            // アルファは元ソースから引き継ぐ
                            Color orig = src.pixel(sx0 + x / scale, sy0 + y / scale);
                            c.a = orig.a;
                            dst.setPixel(dstX, dstY, c);
                        }
                    }
                }

                ++doneCount;
                if (progress) progress(doneCount * 99 / totalTiles);
            }
        }
        if (progress) progress(100);
        return dst;
    }
#endif
};

// ─────────────────────────────────────────────────────────────────────────────
// UpscaleEngine
// ─────────────────────────────────────────────────────────────────────────────
UpscaleEngine::UpscaleEngine(const Config& config)
    : m_impl(std::make_unique<Impl>()) {
#ifdef PAINT_USE_ONNX
    if (!config.modelPath.empty()) {
        m_impl->load(config.modelPath);
    }
#else
    (void)config;
#endif
}

UpscaleEngine::~UpscaleEngine() = default;

bool UpscaleEngine::isAiLoaded() const noexcept { return m_impl->loaded; }
int  UpscaleEngine::nativeScale() const noexcept { return m_impl->scale; }
std::string UpscaleEngine::loadError() const noexcept { return m_impl->error; }

PixelBuffer UpscaleEngine::upscale(const PixelBuffer& src, int scale,
                                    ProgressCallback progress) const {
    if (src.width() <= 0 || src.height() <= 0) return {};

    const int clampedScale = (scale == 2) ? 2 : 4;

#ifdef PAINT_USE_ONNX
    if (m_impl->loaded && m_impl->session) {
        // ONNX 推論パス
        if (m_impl->scale == clampedScale) {
            // モデルが要求倍率に一致
            return m_impl->runTiled(src, clampedScale, progress);
        }
        if (m_impl->scale == 2 && clampedScale == 4) {
            // 2x モデルを 2 回適用
            PixelBuffer mid = m_impl->runTiled(src, 2, [&progress](int p) {
                if (progress) progress(p / 2);
            });
            return m_impl->runTiled(mid, 2, [&progress](int p) {
                if (progress) progress(50 + p / 2);
            });
        }
    }
#endif

    // スタブ: バイリニア補間
    return bilinearUpscale(src, clampedScale, progress);
}

// ─────────────────────────────────────────────────────────────────────────────
// scanModels
// ─────────────────────────────────────────────────────────────────────────────
std::vector<UpscaleEngine::ModelInfo>
UpscaleEngine::scanModels(const std::string& modelsDir) {
    std::vector<ModelInfo> result;
    if (modelsDir.empty()) return result;

    std::error_code ec;
    // サブディレクトリも含めて .onnx を全列挙（名前フィルターなし）
    for (const auto& entry : fs::recursive_directory_iterator(modelsDir, ec)) {
        if (ec) { ec.clear(); continue; }
        if (!entry.is_regular_file()) continue;
        const auto& path = entry.path();
        if (path.extension() != ".onnx") continue;

        const std::string stem = path.stem().string();
        result.push_back({path.string(), stem, guessScale(stem)});
    }

    std::sort(result.begin(), result.end(), [](const ModelInfo& a, const ModelInfo& b) {
        return a.displayName < b.displayName;
    });
    return result;
}

} // namespace core::ai
