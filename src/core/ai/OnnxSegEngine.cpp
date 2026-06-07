#include "core/ai/OnnxSegEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// PAINT_USE_ONNX が OFF のときはスタブ実装のみビルド
// ─────────────────────────────────────────────────────────────────────────────
#ifdef PAINT_USE_ONNX
#include <onnxruntime_cxx_api.h>
#include <mutex>
#endif

namespace core::ai {

// ─────────────────────────────────────────────────────────────────────────────
// Impl
// ─────────────────────────────────────────────────────────────────────────────
struct OnnxSegEngine::Impl {
#ifdef PAINT_USE_ONNX
    Ort::Env                      env{ORT_LOGGING_LEVEL_WARNING, "OnnxSegEngine"};
    Ort::SessionOptions           sessionOpts;
    std::unique_ptr<Ort::Session> encoderSession;
    std::unique_ptr<Ort::Session> decoderSession;

    // キャッシュされた画像埋め込み
    std::vector<float> imageEmbed;      ///< 1×256×64×64
    std::vector<float> highResFeats0;   ///< 1×32×256×256
    std::vector<float> highResFeats1;   ///< 1×64×128×128
    bool               embeddingValid {false};
    int                lastW {0}, lastH {0};

    mutable std::mutex mutex;
#endif
    std::string loadError;
    bool        loaded {false};
};

// ─────────────────────────────────────────────────────────────────────────────
// 定数
// ─────────────────────────────────────────────────────────────────────────────
static constexpr int   kInputSize = 1024;  ///< SAM2 入力解像度
static constexpr int   kMaskSize  = 256;   ///< デコーダ出力解像度
static constexpr int   kNumMasks  = 4;     ///< デコーダが出力するマスク数

// ImageNet 正規化パラメータ（SAM2 標準）
static constexpr float kMean[3] = {123.675f, 116.28f,  103.53f};
static constexpr float kStd[3]  = { 58.395f,  57.12f,   57.375f};

// ─────────────────────────────────────────────────────────────────────────────
// コンストラクタ
// ─────────────────────────────────────────────────────────────────────────────
OnnxSegEngine::OnnxSegEngine(const Config& config)
    : m_impl(std::make_unique<Impl>())
{
#ifdef PAINT_USE_ONNX
    try {
        m_impl->sessionOpts.SetIntraOpNumThreads(4);
        m_impl->sessionOpts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        // Windows: std::wstring へ変換
        auto toWide = [](const std::string& s) {
            std::wstring w(s.begin(), s.end());
            return w;
        };

        m_impl->encoderSession = std::make_unique<Ort::Session>(
            m_impl->env, toWide(config.encoderModelPath).c_str(), m_impl->sessionOpts);
        m_impl->decoderSession = std::make_unique<Ort::Session>(
            m_impl->env, toWide(config.decoderModelPath).c_str(), m_impl->sessionOpts);

        m_impl->loaded = true;
    } catch (const Ort::Exception& e) {
        m_impl->loadError = e.what();
    } catch (const std::exception& e) {
        m_impl->loadError = e.what();
    }
#else
    m_impl->loadError = "ONNX not compiled (PAINT_USE_ONNX=OFF)";
#endif
}

OnnxSegEngine::~OnnxSegEngine() = default;

bool        OnnxSegEngine::isLoaded()  const noexcept { return m_impl->loaded; }
std::string OnnxSegEngine::loadError() const noexcept { return m_impl->loadError; }
bool        OnnxSegEngine::hasEmbedding() const noexcept {
#ifdef PAINT_USE_ONNX
    return m_impl->embeddingValid;
#else
    return false;
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// encodeImage  — 1024×1024 にリサイズして ImageNet 正規化 → エンコーダ実行
// ─────────────────────────────────────────────────────────────────────────────
bool OnnxSegEngine::encodeImage(const PixelBuffer& image)
{
#ifdef PAINT_USE_ONNX
    if (!m_impl->loaded) return false;
    std::lock_guard<std::mutex> lock(m_impl->mutex);

    const int W = image.width();
    const int H = image.height();
    if (W <= 0 || H <= 0) return false;

    // ── 前処理: リサイズ + 正規化 → NCHW float32 ─────────────────────────
    static constexpr int SZ = kInputSize;
    std::vector<float> inputData(3 * SZ * SZ);
    for (int ty = 0; ty < SZ; ++ty) {
        const int sy = ty * H / SZ;
        for (int tx = 0; tx < SZ; ++tx) {
            const int sx = tx * W / SZ;
            const Color c = image.pixel(sx, sy);
            const int base = ty * SZ + tx;
            inputData[0 * SZ * SZ + base] = (static_cast<float>(c.r) - kMean[0]) / kStd[0];
            inputData[1 * SZ * SZ + base] = (static_cast<float>(c.g) - kMean[1]) / kStd[1];
            inputData[2 * SZ * SZ + base] = (static_cast<float>(c.b) - kMean[2]) / kStd[2];
        }
    }

    auto memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<int64_t> imageShape = {1, 3, SZ, SZ};
    auto imageTensor = Ort::Value::CreateTensor<float>(
        memInfo, inputData.data(), inputData.size(),
        imageShape.data(), imageShape.size());

    // ── エンコーダ実行 ────────────────────────────────────────────────────
    const char* inNames[]  = {"image"};
    const char* outNames[] = {"image_embed", "high_res_feats_0", "high_res_feats_1"};
    Ort::RunOptions runOpts{nullptr};

    auto outputs = m_impl->encoderSession->Run(
        runOpts, inNames, &imageTensor, 1, outNames, 3);

    // ── 埋め込みをキャッシュ ──────────────────────────────────────────────
    auto copyOutput = [](const Ort::Value& v, std::vector<float>& dst) {
        const auto* data = v.GetTensorData<float>();
        auto typeInfo = v.GetTensorTypeAndShapeInfo();
        const std::size_t count = typeInfo.GetElementCount();
        dst.assign(data, data + count);
    };
    copyOutput(outputs[0], m_impl->imageEmbed);
    copyOutput(outputs[1], m_impl->highResFeats0);
    copyOutput(outputs[2], m_impl->highResFeats1);

    m_impl->lastW = W;
    m_impl->lastH = H;
    m_impl->embeddingValid = true;
    return true;
#else
    (void)image;
    return false;
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// decode  — ポイントプロンプト → マスク
// ─────────────────────────────────────────────────────────────────────────────
OnnxSegEngine::DecodeResult OnnxSegEngine::decode(
    const std::vector<Point>& positivePoints,
    const std::vector<Point>& negativePoints,
    int originalWidth,
    int originalHeight,
    int granularity) const
{
#ifdef PAINT_USE_ONNX
    if (!m_impl->loaded || !m_impl->embeddingValid || positivePoints.empty()) return {};
    std::lock_guard<std::mutex> lock(m_impl->mutex);

    // ── ポイント座標・ラベル構築 ──────────────────────────────────────────
    // SAM2 はパディングポイントを 1 個末尾に要求する
    const int numPoints =
        static_cast<int>(positivePoints.size() + negativePoints.size()) + 1;
    std::vector<float> pointCoords(numPoints * 2, 0.f);
    std::vector<float> pointLabels(numPoints, -1.f);

    const float scaleX = static_cast<float>(kInputSize) / originalWidth;
    const float scaleY = static_cast<float>(kInputSize) / originalHeight;
    int idx = 0;
    for (const auto& p : positivePoints) {
        pointCoords[idx*2+0] = p.x * scaleX;
        pointCoords[idx*2+1] = p.y * scaleY;
        pointLabels[idx]     = 1.f;
        ++idx;
    }
    for (const auto& p : negativePoints) {
        pointCoords[idx*2+0] = p.x * scaleX;
        pointCoords[idx*2+1] = p.y * scaleY;
        pointLabels[idx]     = 0.f;
        ++idx;
    }
    // パディングポイント（最後の要素は label=-1 のまま）

    auto memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    // ── デコーダ入力テンソル ──────────────────────────────────────────────
    auto makeTensor = [&](std::vector<float>& data,
                          std::vector<int64_t> shape) {
        return Ort::Value::CreateTensor<float>(
            memInfo, data.data(), data.size(),
            shape.data(), shape.size());
    };

    // image_embed [1,256,64,64]
    std::vector<int64_t> embedShape  = {1, 256, 64, 64};
    std::vector<int64_t> hrf0Shape   = {1, 32, 256, 256};
    std::vector<int64_t> hrf1Shape   = {1, 64, 128, 128};
    std::vector<int64_t> coordShape  = {1, numPoints, 2};
    std::vector<int64_t> labelShape  = {1, numPoints};
    std::vector<int64_t> maskInShape = {1, 1, 256, 256};
    std::vector<int64_t> hasMaskShape= {1};

    std::vector<float> maskInput(1*1*256*256, 0.f);
    std::vector<float> hasMask = {0.f};

    // コピーが必要なのでローカル変数に取り出す
    std::vector<float> ie  = m_impl->imageEmbed;
    std::vector<float> h0  = m_impl->highResFeats0;
    std::vector<float> h1  = m_impl->highResFeats1;

    Ort::Value inTensors[] = {
        makeTensor(ie,          embedShape),
        makeTensor(h0,          hrf0Shape),
        makeTensor(h1,          hrf1Shape),
        makeTensor(pointCoords, coordShape),
        makeTensor(pointLabels, labelShape),
        makeTensor(maskInput,   maskInShape),
        makeTensor(hasMask,     hasMaskShape),
    };

    const char* inNames[]  = {
        "image_embed", "high_res_feats_0", "high_res_feats_1",
        "point_coords", "point_labels", "mask_input", "has_mask_input"
    };
    const char* outNames[] = {"masks", "iou_predictions"};

    Ort::RunOptions runOpts{nullptr};
    auto outputs = m_impl->decoderSession->Run(
        runOpts, inNames, inTensors, 7, outNames, 2);

    // ── 出力: masks[1,4,256,256], iou[1,4] ───────────────────────────────
    const float* masksData = outputs[0].GetTensorData<float>();
    const float* iouData   = outputs[1].GetTensorData<float>();

    // granularity に対応するマスクインデックスを選ぶ
    int maskIdx = (granularity < 0)
        ? static_cast<int>(std::max_element(iouData, iouData + kNumMasks) - iouData)
        : std::clamp(granularity, 0, kNumMasks - 1);
    const float bestIou = iouData[maskIdx];

    // ── logit マスクを原寸に拡大してピクセル化 ───────────────────────────
    const float* plane = masksData + maskIdx * kMaskSize * kMaskSize;
    const std::size_t totalPx = static_cast<std::size_t>(originalWidth) * originalHeight;
    std::vector<std::uint8_t> pixels(totalPx, 0);

    for (int y = 0; y < originalHeight; ++y) {
        for (int x = 0; x < originalWidth; ++x) {
            // 双線形補間で 256→原寸へ
            const float mx = (x + 0.5f) * kMaskSize / originalWidth  - 0.5f;
            const float my = (y + 0.5f) * kMaskSize / originalHeight - 0.5f;
            const int mx0 = std::max(0, static_cast<int>(mx));
            const int my0 = std::max(0, static_cast<int>(my));
            const int mx1 = std::min(kMaskSize - 1, mx0 + 1);
            const int my1 = std::min(kMaskSize - 1, my0 + 1);
            const float fx = mx - mx0, fy = my - my0;
            const float logit =
                plane[my0 * kMaskSize + mx0] * (1-fx) * (1-fy) +
                plane[my0 * kMaskSize + mx1] * fx     * (1-fy) +
                plane[my1 * kMaskSize + mx0] * (1-fx) * fy     +
                plane[my1 * kMaskSize + mx1] * fx     * fy;
            if (logit > 0.f) {  // sigmoid(0)=0.5 → logit>0 で確率>50%
                pixels[static_cast<std::size_t>(y) * originalWidth + x] = 255;
            }
        }
    }

    SelectionMask result(originalWidth, originalHeight);
    result.setPixels(pixels);
    return {std::move(result), bestIou, true};
#else
    (void)positivePoints; (void)negativePoints;
    (void)originalWidth;  (void)originalHeight; (void)granularity;
    return {};
#endif
}

} // namespace core::ai
