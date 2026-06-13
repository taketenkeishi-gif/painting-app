#include "app/bridge/AiGenerationController.h"

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QRandomGenerator>

#include "app/bridge/AppController.h"
#include "core/selection/MaskExporter.h"
#include "platform/comfy/ComfyClient.h"
#include "platform/comfy/WorkflowBinding.h"
#include "platform/qt/QtImageConverter.h"

namespace app::bridge {

using namespace platform::comfy;

int AiGenerationController::s_nextInstanceId = 0;

AiGenerationController::AiGenerationController(
    AppController* appController,
    ComfyClient*   comfyClient,
    QObject*       parent)
    : QObject(parent)
    , m_app        (appController)
    , m_comfy      (comfyClient)
    , m_instanceId (++s_nextInstanceId)
{}

// ─────────────────────────────────────────────────────────────────────────────
// PNG エンコードヘルパー
// ─────────────────────────────────────────────────────────────────────────────
QByteArray AiGenerationController::layerToPng(AppController* ac,
                                                const Request& req) {
    const core::PixelBuffer* buf = nullptr;

    if (req.useCompositedBuffer) {
        buf = &ac->compositedBuffer();
    } else {
        const core::Layer* layer = ac->document().activeLayer();
        if (layer != nullptr && layer->isRaster()) {
            buf = &layer->buffer();
        } else {
            // ラスターでないレイヤーの場合は合成バッファにフォールバック
            buf = &ac->compositedBuffer();
        }
    }

    if (buf == nullptr || buf->width() <= 0 || buf->height() <= 0) {
        return {};
    }

    const QImage img = platform::qt::QtImageConverter::toQImage(*buf);
    if (img.isNull()) return {};

    QByteArray out;
    QBuffer qbuf(&out);
    qbuf.open(QIODevice::WriteOnly);
    img.save(&qbuf, "PNG");
    return out;
}

QByteArray AiGenerationController::selectionToPng(AppController* ac) {
    const core::SelectionMask& mask = ac->document().selection();
    if (!mask.hasSelection()) return {};

    const std::vector<std::uint8_t> bytes =
        core::MaskExporter::exportMask(mask, core::MaskExporter::Format::Soft);

    const int w = ac->document().canvasSize().width;
    const int h = ac->document().canvasSize().height;
    if (static_cast<int>(bytes.size()) != w * h) return {};

    // 8-bit グレースケール QImage に変換（scanline コピー）
    QImage img(w, h, QImage::Format_Grayscale8);
    for (int y = 0; y < h; ++y) {
        std::memcpy(img.scanLine(y), bytes.data() + y * w,
                    static_cast<std::size_t>(w));
    }

    QByteArray out;
    QBuffer qbuf(&out);
    qbuf.open(QIODevice::WriteOnly);
    img.save(&qbuf, "PNG");
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// execute  — エントリポイント
// ─────────────────────────────────────────────────────────────────────────────
static void aiTrace(const char* msg) {
    QFile f(QStringLiteral("debug_generate_trace.txt"));
    f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    f.write(QByteArray(msg) + "\n");
}

// ─────────────────────────────────────────────────────────────────────────────
// workflow ロード + バインド適用ヘルパー
// ─────────────────────────────────────────────────────────────────────────────
bool AiGenerationController::loadWorkflowDoc(const Request& req,
                                               WorkflowDocument& out) {
    if (req.workflowDoc.isValid()) {
        out = req.workflowDoc;
    } else if (!req.workflowPath.isEmpty()) {
        bool ok = false;
        out = WorkflowDocument::load(req.workflowPath, &ok);
        if (!ok) {
            fail(QString("workflow.json の読み込みに失敗しました: %1 — %2")
                     .arg(req.workflowPath, out.errorString()));
            return false;
        }
    } else {
        fail("workflowPath と workflowDoc の両方が未設定です。");
        return false;
    }
    for (const WorkflowBinding& b : req.extraBindings) {
        out.apply(b);
    }
    return true;
}

void AiGenerationController::execute(const Request& req) {
    aiTrace("execute: entered");
    if (m_busy) {
        emit errorOccurred("AI 生成が実行中です。完了を待ってから再試行してください。");
        return;
    }
    if (m_app == nullptr || m_comfy == nullptr) {
        emit errorOccurred("AiGenerationController が初期化されていません。");
        return;
    }

    WorkflowDocument doc;
    if (!loadWorkflowDoc(req, doc)) return;

    aiTrace("execute: loading workflow ok, applying bindings");
    m_batchTotal = 1;
    m_busy = true;
    emit started();
    aiTrace("execute: started emitted");

    if (req.useActiveLayer || req.useCompositedBuffer) {
        doUploadInputImage(std::move(doc), req);
    } else {
        const QString empty;
        doUploadMask(std::move(doc), req, empty);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// executeBatch — batch 生成エントリポイント
// ─────────────────────────────────────────────────────────────────────────────
void AiGenerationController::executeBatch(const Request& req, int batchCount) {
    if (batchCount <= 1) {
        execute(req);
        return;
    }

    if (m_busy) {
        emit errorOccurred("AI 生成が実行中です。完了を待ってから再試行してください。");
        return;
    }
    if (m_app == nullptr || m_comfy == nullptr) {
        emit errorOccurred("AiGenerationController が初期化されていません。");
        return;
    }

    m_batchTotal     = batchCount;
    m_batchRemaining = batchCount;
    m_batchResults.clear();
    m_batchBaseReq   = req;

    // 1枚目は execute() 経由（m_batchTotal を設定済みなので batch フローに入る）
    WorkflowDocument doc;
    if (!loadWorkflowDoc(req, doc)) return;

    m_busy = true;
    emit started();

    if (req.useActiveLayer || req.useCompositedBuffer) {
        doUploadInputImage(std::move(doc), req);
    } else {
        const QString empty;
        doUploadMask(std::move(doc), req, empty);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// runNextBatchIteration — 次バッチを投入する（m_busy チェックなし）
// ─────────────────────────────────────────────────────────────────────────────
void AiGenerationController::runNextBatchIteration(const Request& req) {
    WorkflowDocument doc;
    if (!loadWorkflowDoc(req, doc)) return;

    // サンプラーノードの seed / noise_seed をランダム化して変種を生成する。
    // KSampler 以外のカスタムサンプラー (GLIDE_Sampler 等) にも対応するため
    // findSamplerNodes() でノードをスキャンする。
    const QStringList samplerNodes = doc.findSamplerNodes();
    const int newSeed = static_cast<int>(QRandomGenerator::global()->generate());
    for (const QString& nid : samplerNodes) {
        const QJsonObject inputs = doc.nodeInputs(nid);
        if (inputs.contains(QLatin1String("seed"))) {
            doc.setInput(nid, QLatin1String("seed"), newSeed);
        }
        if (inputs.contains(QLatin1String("noise_seed"))) {
            // QJsonValue は double 精度のため、安全に表現できる範囲で乱数を生成する
            const double bigSeed = static_cast<double>(
                (static_cast<quint64>(QRandomGenerator::global()->generate()) << 16)
                | static_cast<quint64>(QRandomGenerator::global()->generate() & 0xFFFF));
            doc.setInput(nid, QLatin1String("noise_seed"), bigSeed);
        }
    }

    if (req.useActiveLayer || req.useCompositedBuffer) {
        doUploadInputImage(std::move(doc), req);
    } else {
        const QString empty;
        doUploadMask(std::move(doc), req, empty);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ステップ 1: 入力画像アップロード
// ─────────────────────────────────────────────────────────────────────────────
void AiGenerationController::doUploadInputImage(WorkflowDocument doc,
                                                  const Request&   req) {
    const QByteArray png = layerToPng(m_app, req);
    if (png.isEmpty()) {
        fail("アクティブレイヤーの PNG 変換に失敗しました。");
        return;
    }

    const QString uploadName = "lpa_input.png";
    m_comfy->uploadImage(png, uploadName,
        [this, doc = std::move(doc), req](QString saved, QString err) mutable {
            if (!err.isEmpty()) {
                fail("入力画像のアップロードに失敗: " + err);
                return;
            }
            // LoadImage バインド適用
            const QString nodeId =
                req.inputImageNodeId.isEmpty()
                    ? (!doc.findNodesByClass("LoadImage").isEmpty()
                           ? doc.findNodesByClass("LoadImage").first()
                           : QString{})
                    : req.inputImageNodeId;
            if (!nodeId.isEmpty()) {
                doc.apply(WorkflowBinding::loadImage(nodeId, saved));
            }
            doUploadMask(std::move(doc), req, saved);
        });
}

// ─────────────────────────────────────────────────────────────────────────────
// ステップ 2: 選択マスクアップロード
// ─────────────────────────────────────────────────────────────────────────────
void AiGenerationController::doUploadMask(WorkflowDocument doc,
                                            const Request&   req,
                                            const QString& /*savedImageName*/) {
    aiTrace("doUploadMask: entered");
    if (!req.useSelectionAsMask) {
        aiTrace("doUploadMask: skipping mask, going to doQueue");
        doQueue(std::move(doc), req);
        return;
    }

    const QByteArray maskPng = selectionToPng(m_app);
    if (maskPng.isEmpty()) {
        // マスクがなければスキップして続行
        doQueue(std::move(doc), req);
        return;
    }

    const QString maskUploadName = "lpa_mask.png";
    m_comfy->uploadImage(maskPng, maskUploadName,
        [this, doc = std::move(doc), req](QString saved, QString err) mutable {
            if (!err.isEmpty()) {
                fail("マスク画像のアップロードに失敗: " + err);
                return;
            }
            // LoadImageMask バインド適用
            const QString maskNodeId =
                req.maskImageNodeId.isEmpty()
                    ? (!doc.findNodesByClass("LoadImageMask").isEmpty()
                           ? doc.findNodesByClass("LoadImageMask").first()
                           : QString{})
                    : req.maskImageNodeId;
            if (!maskNodeId.isEmpty()) {
                doc.apply(WorkflowBinding::byClass("LoadImageMask",
                          maskNodeId.isEmpty() ? 0 : 0)
                         .set("image",  saved)
                         .set("upload", QString("image")));
            }
            doQueue(std::move(doc), req);
        });
}

// ─────────────────────────────────────────────────────────────────────────────
// ステップ 3: キュー投入
// ─────────────────────────────────────────────────────────────────────────────
void AiGenerationController::doQueue(WorkflowDocument doc, const Request& req) {
    aiTrace("doQueue: entered");
#ifdef PAINT_DEBUG_SERVER
    // 送信直前に workflow JSON を Dev_Bridge 観測バッファへ記録する。
    aiTrace("doQueue: capturing queued workflow");
    m_app->debugCaptureQueuedWorkflow(doc.toJson());
    aiTrace("doQueue: calling queueWorkflow");
#endif
    m_comfy->queueWorkflow(doc,
        [this, req](QString promptId, QString err) mutable {
            aiTrace("doQueue cb: fired");
#ifdef PAINT_DEBUG_SERVER
            // POST /prompt 結果（成功・失敗問わず）をキャプチャして Dev_Bridge へ公開する。
            aiTrace("doQueue cb: getting payload bytes");
            const QByteArray dbgPl = m_comfy->dbgLastPayload();
            aiTrace("doQueue cb: calling debugCaptureComfyPayload");
            m_app->debugCaptureComfyPayload(dbgPl);
            aiTrace("doQueue cb: getting response bytes");
            const int dbgStatus = m_comfy->dbgLastResponseStatus();
            const QByteArray dbgBody = m_comfy->dbgLastResponseBody();
            aiTrace("doQueue cb: calling debugCaptureComfyResponse");
            m_app->debugCaptureComfyResponse(dbgStatus, dbgBody);
            aiTrace("doQueue cb: capture done");
#endif
            if (!err.isEmpty()) {
                aiTrace("doQueue cb: error path");
                fail("ワークフロー投入に失敗: " + err);
                return;
            }
            aiTrace("doQueue cb: success, calling doWait");
            doWait(promptId, req);
        });
}

// ─────────────────────────────────────────────────────────────────────────────
// ステップ 4: 完了待ち
// ─────────────────────────────────────────────────────────────────────────────
void AiGenerationController::doWait(const QString& promptId,
                                      const Request& req) {
    m_comfy->waitForOutputs(promptId,
        [this, req](QStringList files, QString err) mutable {
            if (!err.isEmpty()) {
                fail("ComfyUI 実行待ちに失敗: " + err);
                return;
            }
            if (files.isEmpty()) {
                fail("ComfyUI から出力ファイルが返りませんでした。");
                return;
            }
            doFetch(files, req);
        },
        /* pollIntervalMs */ 500,
        req.timeoutMs);
}

// ─────────────────────────────────────────────────────────────────────────────
// ステップ 5: 結果画像取得
// ─────────────────────────────────────────────────────────────────────────────
void AiGenerationController::doFetch(const QStringList& filenames,
                                       const Request& req) {
    // 最初の出力画像だけ取得する
    m_comfy->fetchImage(filenames.first(), "", "output",
        [this, req](QByteArray data, QString err) mutable {
            if (!err.isEmpty()) {
                fail("結果画像の取得に失敗: " + err);
                return;
            }
            doApplyResult(data, req);
        });
}

// ─────────────────────────────────────────────────────────────────────────────
// ステップ 6: PNG → PixelBuffer → レイヤー貼り付け or 候補収集
// ─────────────────────────────────────────────────────────────────────────────
void AiGenerationController::doApplyResult(const QByteArray& pngBytes,
                                             const Request& req) {
    QImage img;
    if (!img.loadFromData(pngBytes, "PNG")) {
        if (!img.loadFromData(pngBytes)) {
            fail("結果画像のデコードに失敗しました。");
            return;
        }
    }

    if (m_batchTotal > 1) {
        // ── Batch モード: 候補として収集 ────────────────────────────────────
        m_batchResults.append(QPixmap::fromImage(img));
        --m_batchRemaining;

        if (m_batchRemaining > 0) {
            // 次イテレーション: seed をランダムに差し替えて再投入
            Request nextReq = m_batchBaseReq;
            for (WorkflowBinding& b : nextReq.extraBindings) {
                if (b.target() == WorkflowBinding::Target::KSampler) {
                    b.seed(static_cast<int>(
                        QRandomGenerator::global()->generate()));
                }
            }
            runNextBatchIteration(nextReq);
        } else {
            // 全バッチ完了 → 候補グリッドへ
            m_busy = false;
            QList<QPixmap> results = m_batchResults;
            m_batchResults.clear();
            m_batchTotal = 1;
            emit batchCandidatesReady(results);
        }
        return;
    }

    // ── Single モード: 即レイヤー追加 ───────────────────────────────────────
    const core::PixelBuffer buf =
        platform::qt::QtImageConverter::fromQImage(img);

    if (buf.width() <= 0 || buf.height() <= 0) {
        fail("結果画像が空です。");
        return;
    }

    m_app->pasteBufferAsNewRasterLayer(buf, req.outputLayerName.toStdString());

    m_busy = false;
    emit finished();
}

// ─────────────────────────────────────────────────────────────────────────────
// エラーハンドラ
// ─────────────────────────────────────────────────────────────────────────────
void AiGenerationController::fail(const QString& message) {
    m_busy = false;
    emit errorOccurred(message);
}

} // namespace app::bridge
