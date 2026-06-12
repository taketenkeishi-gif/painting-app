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
#include <cstdio>
#endif

namespace core::ai {

struct OnnxSegEngine::Impl {
#ifdef PAINT_USE_ONNX
    Ort::Env                      env{ORT_LOGGING_LEVEL_WARNING, "OnnxSegEngine"};
    Ort::SessionOptions           sessionOpts;
    std::unique_ptr<Ort::Session> encoderSession;
    std::unique_ptr<Ort::Session> decoderSession;

    // モデルから動的に取得したテンソル名
    std::string              encInputName;
    std::string              encOutputName;
    std::vector<std::string> decInputNames;
    std::string              decMaskOutName;
    std::string              decIouOutName;

    // キャッシュされた埋め込み
    std::vector<float>   imageEmbed;
    std::vector<int64_t> embedShape;   // エンコーダ出力の実際のシェイプ
    float                imageScale{1.f};
    bool                 embeddingValid{false};
    int                  lastW{0}, lastH{0};

    mutable std::mutex mutex;
#endif
    std::string loadError;
    bool        loaded{false};
};

static constexpr int   kInputSize = 1024;
static constexpr float kMean[3]   = {123.675f, 116.28f,  103.53f};
static constexpr float kStd[3]    = { 58.395f,  57.12f,   57.375f};

// ─────────────────────────────────────────────────────────────────────────────
// コンストラクタ: モデル読み込み + テンソル名を動的取得
// ─────────────────────────────────────────────────────────────────────────────
OnnxSegEngine::OnnxSegEngine(const Config& config)
    : m_impl(std::make_unique<Impl>())
{
#ifdef PAINT_USE_ONNX
    try {
        m_impl->sessionOpts.SetIntraOpNumThreads(4);
        m_impl->sessionOpts.SetGraphOptimizationLevel(
            GraphOptimizationLevel::ORT_ENABLE_ALL);

        auto toWide = [](const std::string& s) {
            return std::wstring(s.begin(), s.end());
        };

        m_impl->encoderSession = std::make_unique<Ort::Session>(
            m_impl->env, toWide(config.encoderModelPath).c_str(),
            m_impl->sessionOpts);
        m_impl->decoderSession = std::make_unique<Ort::Session>(
            m_impl->env, toWide(config.decoderModelPath).c_str(),
            m_impl->sessionOpts);

        Ort::AllocatorWithDefaultOptions alloc;

        // エンコーダのテンソル名を動的取得
        m_impl->encInputName  = m_impl->encoderSession->GetInputNameAllocated(0, alloc).get();
        m_impl->encOutputName = m_impl->encoderSession->GetOutputNameAllocated(0, alloc).get();

        // デコーダの全入力名を取得
        const std::size_t numDecIn = m_impl->decoderSession->GetInputCount();
        for (std::size_t i = 0; i < numDecIn; ++i)
            m_impl->decInputNames.push_back(
                m_impl->decoderSession->GetInputNameAllocated(i, alloc).get());

        // デコーダ出力名
        m_impl->decMaskOutName = m_impl->decoderSession->GetOutputNameAllocated(0, alloc).get();
        if (m_impl->decoderSession->GetOutputCount() > 1)
            m_impl->decIouOutName = m_impl->decoderSession->GetOutputNameAllocated(1, alloc).get();

        m_impl->loaded = true;

    } catch (const Ort::Exception& e) {
        m_impl->loadError = std::string("ONNX load: ") + e.what();
    } catch (const std::exception& e) {
        m_impl->loadError = std::string("load: ") + e.what();
    }
#else
    m_impl->loadError = "ONNX not compiled (PAINT_USE_ONNX=OFF)";
#endif
}

OnnxSegEngine::~OnnxSegEngine() = default;

bool        OnnxSegEngine::isLoaded()     const noexcept { return m_impl->loaded; }
std::string OnnxSegEngine::loadError()    const noexcept { return m_impl->loadError; }
bool        OnnxSegEngine::hasEmbedding() const noexcept {
#ifdef PAINT_USE_ONNX
    return m_impl->embeddingValid;
#else
    return false;
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// encodeImage — アスペクト比維持リサイズ + ImageNet 正規化 → エンコーダ実行
// ─────────────────────────────────────────────────────────────────────────────
bool OnnxSegEngine::encodeImage(const PixelBuffer& image)
{
#ifdef PAINT_USE_ONNX
    if (!m_impl->loaded) return false;
    std::lock_guard<std::mutex> lock(m_impl->mutex);

    const int W = image.width();
    const int H = image.height();
    if (W <= 0 || H <= 0) return false;

    // アスペクト比を保って kInputSize に収める (余白はゼロパディング)
    const float scale = std::min(
        static_cast<float>(kInputSize) / W,
        static_cast<float>(kInputSize) / H);
    const int newW = static_cast<int>(std::round(W * scale));
    const int newH = static_cast<int>(std::round(H * scale));

    static constexpr int SZ = kInputSize;
    std::vector<float> inputData(3 * SZ * SZ, 0.f);

    for (int ty = 0; ty < newH; ++ty) {
        const int sy = std::clamp(static_cast<int>(ty / scale), 0, H - 1);
        for (int tx = 0; tx < newW; ++tx) {
            const int sx = std::clamp(static_cast<int>(tx / scale), 0, W - 1);
            const Color c = image.pixel(sx, sy);
            const int base = ty * SZ + tx;
            inputData[0 * SZ * SZ + base] = (static_cast<float>(c.r) - kMean[0]) / kStd[0];
            inputData[1 * SZ * SZ + base] = (static_cast<float>(c.g) - kMean[1]) / kStd[1];
            inputData[2 * SZ * SZ + base] = (static_cast<float>(c.b) - kMean[2]) / kStd[2];
        }
    }

    try {
        auto memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<int64_t> imageShape = {1, 3, SZ, SZ};
        auto imageTensor = Ort::Value::CreateTensor<float>(
            memInfo, inputData.data(), inputData.size(),
            imageShape.data(), imageShape.size());

        const char* inName  = m_impl->encInputName.c_str();
        const char* outName = m_impl->encOutputName.c_str();

        Ort::RunOptions runOpts{nullptr};
        auto outputs = m_impl->encoderSession->Run(
            runOpts, &inName, &imageTensor, 1, &outName, 1);

        auto info = outputs[0].GetTensorTypeAndShapeInfo();
        m_impl->embedShape = info.GetShape();
        const std::size_t cnt = info.GetElementCount();

        const float* data = outputs[0].GetTensorData<float>();
        m_impl->imageEmbed.assign(data, data + cnt);
        m_impl->imageScale     = scale;
        m_impl->lastW          = W;
        m_impl->lastH          = H;
        m_impl->embeddingValid = true;
        return true;

    } catch (const Ort::Exception& e) {
        std::fprintf(stderr, "[OnnxSegEngine] encode error: %s\n", e.what());
        m_impl->embeddingValid = false;
        return false;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[OnnxSegEngine] encode error: %s\n", e.what());
        m_impl->embeddingValid = false;
        return false;
    }
#else
    (void)image;
    return false;
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// decode — ポイントプロンプト + ボックスプロンプト → マスク
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

    try {
        const float scale = m_impl->imageScale;
        static FILE* dbg = std::fopen("C:/Users/Keishi/onnx_debug.log", "a");
        if (dbg) {
            std::fprintf(dbg, "[OnnxSeg] decode: origW=%d origH=%d scale=%.4f pos=%d neg=%d\n",
                originalWidth, originalHeight, scale,
                (int)positivePoints.size(), (int)negativePoints.size());
            if (!positivePoints.empty())
                std::fprintf(dbg, "[OnnxSeg] first posPoint: px=%d py=%d -> samX=%.1f samY=%.1f\n",
                    positivePoints[0].x, positivePoints[0].y,
                    positivePoints[0].x * scale, positivePoints[0].y * scale);
            std::fflush(dbg);
        }

        // ── プロンプト点を間引き (max 8 fg / 5 bg) ──────────────────────────
        auto samplePts = [](const std::vector<Point>& pts, int maxN) {
            if (static_cast<int>(pts.size()) <= maxN) return pts;
            std::vector<Point> out;
            out.reserve(maxN);
            const float step = static_cast<float>(pts.size() - 1) / (maxN - 1);
            for (int i = 0; i < maxN; ++i)
                out.push_back(pts[static_cast<int>(std::round(i * step))]);
            return out;
        };
        const auto sampledPos = samplePts(positivePoints, 8);
        const auto sampledNeg = samplePts(negativePoints, 5);

        // ── ボックスプロンプト (FG 点の AABB + 25% パディング) ────────────────
        struct BoxPt { float x, y; int label; };
        std::vector<BoxPt> boxPts;
        if (!sampledPos.empty()) {
            float minX = static_cast<float>(sampledPos[0].x);
            float minY = static_cast<float>(sampledPos[0].y);
            float maxX = minX, maxY = minY;
            for (const auto& p : sampledPos) {
                minX = std::min(minX, static_cast<float>(p.x));
                minY = std::min(minY, static_cast<float>(p.y));
                maxX = std::max(maxX, static_cast<float>(p.x));
                maxY = std::max(maxY, static_cast<float>(p.y));
            }
            const float padX = std::max((maxX - minX) * 0.25f, originalWidth  * 0.03f);
            const float padY = std::max((maxY - minY) * 0.25f, originalHeight * 0.03f);
            boxPts.push_back({std::max(0.f, minX - padX), std::max(0.f, minY - padY), 2});
            boxPts.push_back({
                std::min(static_cast<float>(originalWidth  - 1), maxX + padX),
                std::min(static_cast<float>(originalHeight - 1), maxY + padY), 3});
        }

        // ── テンソル構築 ──────────────────────────────────────────────────────
        // SAM パディング点 (label=-1) を末尾に 1 個追加
        const int numPts = static_cast<int>(
            sampledPos.size() + sampledNeg.size() + boxPts.size()) + 1;

        std::vector<float> pointCoords(static_cast<std::size_t>(numPts) * 2, 0.f);
        std::vector<float> pointLabels(static_cast<std::size_t>(numPts), -1.f);

        int idx = 0;
        for (const auto& p : sampledPos) {
            pointCoords[idx * 2]     = p.x * scale;
            pointCoords[idx * 2 + 1] = p.y * scale;
            pointLabels[idx]         = 1.f;
            ++idx;
        }
        for (const auto& p : sampledNeg) {
            pointCoords[idx * 2]     = p.x * scale;
            pointCoords[idx * 2 + 1] = p.y * scale;
            pointLabels[idx]         = 0.f;
            ++idx;
        }
        for (const auto& bp : boxPts) {
            pointCoords[idx * 2]     = bp.x * scale;
            pointCoords[idx * 2 + 1] = bp.y * scale;
            pointLabels[idx]         = static_cast<float>(bp.label);
            ++idx;
        }

        auto memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        auto mkTf = [&memInfo](float* ptr, std::size_t sz, std::vector<int64_t> sh) {
            return Ort::Value::CreateTensor<float>(
                memInfo, ptr, sz, sh.data(), sh.size());
        };

        // image_embeddings
        std::vector<float>   ie = m_impl->imageEmbed;
        std::vector<int64_t> es = m_impl->embedShape;
        auto embedTensor = Ort::Value::CreateTensor<float>(
            memInfo, ie.data(), ie.size(), es.data(), es.size());

        // point_coords
        std::vector<int64_t> coordShape = {1, numPts, 2};
        auto coordTensor = mkTf(pointCoords.data(), pointCoords.size(), coordShape);

        // point_labels
        std::vector<int64_t> labelShape = {1, numPts};
        auto labelTensor = mkTf(pointLabels.data(), pointLabels.size(), labelShape);

        // mask_input (ゼロ)
        std::vector<float>   maskInput(1 * 1 * 256 * 256, 0.f);
        std::vector<int64_t> maskInShape = {1, 1, 256, 256};
        auto maskInTensor = mkTf(maskInput.data(), maskInput.size(), maskInShape);

        // has_mask_input = 0
        std::vector<float>   hasMask     = {0.f};
        std::vector<int64_t> hasMaskShape= {1};
        auto hasMaskTensor = mkTf(hasMask.data(), hasMask.size(), hasMaskShape);

        // orig_im_size = [H, W]
        std::vector<float>   origImSize  = {static_cast<float>(originalHeight),
                                             static_cast<float>(originalWidth)};
        std::vector<int64_t> origImShape = {2};
        auto origImTensor = mkTf(origImSize.data(), origImSize.size(), origImShape);

        // デコーダ入力: 動的に取得した名前順に対応付け
        std::vector<Ort::Value> inTensors;
        inTensors.reserve(m_impl->decInputNames.size());

        for (const auto& name : m_impl->decInputNames) {
            if      (name == "image_embeddings" || name == "image_embed")
                inTensors.push_back(std::move(embedTensor));
            else if (name == "point_coords")
                inTensors.push_back(std::move(coordTensor));
            else if (name == "point_labels")
                inTensors.push_back(std::move(labelTensor));
            else if (name == "mask_input")
                inTensors.push_back(std::move(maskInTensor));
            else if (name == "has_mask_input")
                inTensors.push_back(std::move(hasMaskTensor));
            else if (name == "orig_im_size")
                inTensors.push_back(std::move(origImTensor));
            else {
                // 未知の入力: ゼロスカラーで埋める
                std::vector<float>   ph = {0.f};
                std::vector<int64_t> ps = {1};
                inTensors.push_back(Ort::Value::CreateTensor<float>(
                    memInfo, ph.data(), 1, ps.data(), 1));
            }
        }

        // 名前ポインタ配列
        std::vector<const char*> inNamePtrs;
        for (const auto& n : m_impl->decInputNames)
            inNamePtrs.push_back(n.c_str());

        std::vector<const char*> outNamePtrs;
        outNamePtrs.push_back(m_impl->decMaskOutName.c_str());
        if (!m_impl->decIouOutName.empty())
            outNamePtrs.push_back(m_impl->decIouOutName.c_str());

        Ort::RunOptions runOpts{nullptr};
        auto outputs = m_impl->decoderSession->Run(
            runOpts,
            inNamePtrs.data(), inTensors.data(), inTensors.size(),
            outNamePtrs.data(), outNamePtrs.size());

        const float* masksData = outputs[0].GetTensorData<float>();
        const float* iouData   = (outputs.size() > 1)
            ? outputs[1].GetTensorData<float>() : nullptr;

        auto maskDims = outputs[0].GetTensorTypeAndShapeInfo().GetShape();
        const int numMasks = static_cast<int>(maskDims[1]);
        const int maskH    = static_cast<int>(maskDims[2]);
        const int maskW    = static_cast<int>(maskDims[3]);
        if (dbg) {
            std::fprintf(dbg, "[OnnxSeg] mask: numMasks=%d maskH=%d maskW=%d origH=%d origW=%d\n",
                numMasks, maskH, maskW, originalHeight, originalWidth);
            if (iouData)
                for (int i = 0; i < numMasks; ++i)
                    std::fprintf(dbg, "[OnnxSeg] iou[%d]=%.4f\n", i, iouData[i]);
            std::fflush(dbg);
        }

        // グラニュラリティに対応するマスクを選択 (-1 = IoU 最大)
        int maskIdx = 0;
        if (granularity < 0 && iouData) {
            maskIdx = static_cast<int>(
                std::max_element(iouData, iouData + numMasks) - iouData);
        } else if (granularity >= 0) {
            maskIdx = std::clamp(granularity, 0, numMasks - 1);
        }
        const float bestIou = iouData ? iouData[maskIdx] : 1.f;

        // ── マスクを元画像サイズへ展開 ───────────────────────────────────────
        // SAM デコーダは orig_im_size を受け取っても内部的には
        // パディング済み 1024×1024 から maskH×maskW へ直接リサイズする実装が多い。
        // その場合、パディング分だけ座標が圧縮される（景観画像の Y 軸ずれ等）。
        // 正しいマッピング: canvas(x,y) → encoder(x*scale, y*scale) → mask
        //   mx = (x+0.5) * scale * maskW / kInputSize - 0.5
        //   my = (y+0.5) * scale * maskH / kInputSize - 0.5
        const float* plane = masksData + maskIdx * maskH * maskW;
        std::vector<std::uint8_t> pixels(
            static_cast<std::size_t>(originalWidth) * originalHeight, 0);

        {
            // スケール考慮バイリニア補間（パディング有無に関わらず正確）
            const float scaleW = scale * static_cast<float>(maskW) / static_cast<float>(kInputSize);
            const float scaleH = scale * static_cast<float>(maskH) / static_cast<float>(kInputSize);
            if (dbg) {
                std::fprintf(dbg, "[OnnxSeg] remap: scale=%.4f scaleW=%.4f scaleH=%.4f maskW=%d maskH=%d\n",
                    scale, scaleW, scaleH, maskW, maskH);
                std::fflush(dbg);
            }
            for (int y = 0; y < originalHeight; ++y) {
                for (int x = 0; x < originalWidth; ++x) {
                    const float mxf = (x + 0.5f) * scaleW - 0.5f;
                    const float myf = (y + 0.5f) * scaleH - 0.5f;
                    const int mx0 = std::max(0, static_cast<int>(mxf));
                    const int my0 = std::max(0, static_cast<int>(myf));
                    const int mx1 = std::min(maskW - 1, mx0 + 1);
                    const int my1 = std::min(maskH - 1, my0 + 1);
                    const float fx = mxf - mx0, fy = myf - my0;
                    const float logit =
                        plane[my0 * maskW + mx0] * (1 - fx) * (1 - fy) +
                        plane[my0 * maskW + mx1] *      fx  * (1 - fy) +
                        plane[my1 * maskW + mx0] * (1 - fx) *      fy  +
                        plane[my1 * maskW + mx1] *      fx  *      fy;
                    if (logit > 0.f)
                        pixels[static_cast<std::size_t>(y) * originalWidth + x] = 255;
                }
            }
        }

        SelectionMask result(originalWidth, originalHeight);
        result.setPixels(pixels);
        return {std::move(result), bestIou, true};

    } catch (const Ort::Exception& e) {
        std::fprintf(stderr, "[OnnxSegEngine] decode error: %s\n", e.what());
        return {};
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[OnnxSegEngine] decode error: %s\n", e.what());
        return {};
    }
#else
    (void)positivePoints; (void)negativePoints;
    (void)originalWidth;  (void)originalHeight; (void)granularity;
    return {};
#endif
}

} // namespace core::ai
