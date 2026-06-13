#include "app/bridge/AiGenerationController.h"

#include <QBuffer>
#include <QImage>

#include "app/bridge/AppController.h"
#include "core/selection/MaskExporter.h"
#include "platform/comfy/ComfyClient.h"
#include "platform/comfy/WorkflowBinding.h"
#include "platform/qt/QtImageConverter.h"

namespace app::bridge {

using namespace platform::comfy;

AiGenerationController::AiGenerationController(
    AppController* appController,
    ComfyClient*   comfyClient,
    QObject*       parent)
    : QObject(parent)
    , m_app  (appController)
    , m_comfy(comfyClient)
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
void AiGenerationController::execute(const Request& req) {
    if (m_busy) {
        emit errorOccurred("AI 生成が実行中です。完了を待ってから再試行してください。");
        return;
    }
    if (m_app == nullptr || m_comfy == nullptr) {
        emit errorOccurred("AiGenerationController が初期化されていません。");
        return;
    }

    // WorkflowDocument の準備
    WorkflowDocument doc;
    if (req.workflowDoc.isValid()) {
        doc = req.workflowDoc;
    } else if (!req.workflowPath.isEmpty()) {
        bool ok = false;
        doc = WorkflowDocument::load(req.workflowPath, &ok);
        if (!ok) {
            emit errorOccurred(
                QString("workflow.json の読み込みに失敗しました: %1 — %2")
                    .arg(req.workflowPath, doc.errorString()));
            return;
        }
    } else {
        emit errorOccurred("workflowPath と workflowDoc の両方が未設定です。");
        return;
    }

    // ユーザー指定バインドを先に適用
    for (const WorkflowBinding& b : req.extraBindings) {
        doc.apply(b);
    }

    m_busy = true;
    emit started();

    // 入力画像のアップロードが必要なら先に実行
    if (req.useActiveLayer || req.useCompositedBuffer) {
        doUploadInputImage(std::move(doc), req);
    } else {
        // 入力画像なし → マスクの有無を確認
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
    if (!req.useSelectionAsMask) {
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
    m_comfy->queueWorkflow(doc,
        [this, req](QString promptId, QString err) mutable {
            if (!err.isEmpty()) {
                fail("ワークフロー投入に失敗: " + err);
                return;
            }
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
// ステップ 6: PNG → PixelBuffer → 新規レイヤー貼り付け
// ─────────────────────────────────────────────────────────────────────────────
void AiGenerationController::doApplyResult(const QByteArray& pngBytes,
                                             const Request& req) {
    QImage img;
    if (!img.loadFromData(pngBytes, "PNG")) {
        // PNG でなければ形式を自動判定して再試行
        if (!img.loadFromData(pngBytes)) {
            fail("結果画像のデコードに失敗しました。");
            return;
        }
    }

    const core::PixelBuffer buf =
        platform::qt::QtImageConverter::fromQImage(img);

    if (buf.width() <= 0 || buf.height() <= 0) {
        fail("結果画像が空です。");
        return;
    }

    // 新規ラスターレイヤーとして貼り付け（元レイヤーを上書きしない）
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
