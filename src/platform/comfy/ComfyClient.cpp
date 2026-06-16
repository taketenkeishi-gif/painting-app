#include "platform/comfy/ComfyClient.h"

#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrlQuery>

#include "platform/comfy/WorkflowBinding.h"

namespace platform::comfy {

ComfyClient::ComfyClient(QObject* parent)
    : QObject(parent) {}

// ─────────────────────────────────────────────────────────────────────────────
// HTTP ヘルパー
// ─────────────────────────────────────────────────────────────────────────────
QNetworkReply* ComfyClient::get(const QString& path) {
    QUrl url = m_baseUrl;
    url.setPath(path);
    QNetworkRequest req(url);
    req.setRawHeader("Accept", "application/json");
    return m_nam.get(req);
}

QNetworkReply* ComfyClient::post(const QString& path,
                                   const QByteArray& body,
                                   const QString& contentType) {
    QUrl url = m_baseUrl;
    url.setPath(path);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    return m_nam.post(req, body);
}

// ─────────────────────────────────────────────────────────────────────────────
// uploadImage  —  POST /upload/image (multipart/form-data)
// ─────────────────────────────────────────────────────────────────────────────
void ComfyClient::uploadImage(const QByteArray& pngData,
                                const QString& name,
                                std::function<void(QString, QString)> cb) {
    QUrl url = m_baseUrl;
    url.setPath("/upload/image");

    auto* mp = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart imgPart;
    imgPart.setHeader(QNetworkRequest::ContentTypeHeader,
                      QVariant("image/png"));
    imgPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                      QVariant(QString(R"(form-data; name="image"; filename="%1")").arg(name)));
    imgPart.setBody(pngData);
    mp->append(imgPart);

    QHttpPart typePart;
    typePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(R"(form-data; name="type")"));
    typePart.setBody("input");
    mp->append(typePart);

    QHttpPart overwritePart;
    overwritePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                             QVariant(R"(form-data; name="overwrite")"));
    overwritePart.setBody("true");
    mp->append(overwritePart);

    QNetworkRequest req(url);
    QNetworkReply* reply = m_nam.post(req, mp);
    mp->setParent(reply);

    connect(reply, &QNetworkReply::finished, this, [reply, cb = std::move(cb)]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            cb({}, reply->errorString());
            return;
        }
        const QJsonObject resp =
            QJsonDocument::fromJson(reply->readAll()).object();
        const QString saved = resp.value("name").toString();
        if (saved.isEmpty()) {
            cb({}, "Upload succeeded but response has no 'name' field");
        } else {
            cb(saved, {});
        }
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// queueWorkflow  —  POST /prompt
// ─────────────────────────────────────────────────────────────────────────────
void ComfyClient::queueWorkflow(const WorkflowDocument& doc,
                                  std::function<void(QString, QString)> cb) {
    QJsonObject body;
    body.insert("prompt",    doc.toJson());
    body.insert("client_id", m_clientId);
    const QByteArray bodyBytes =
        QJsonDocument(body).toJson(QJsonDocument::Compact);

#ifdef PAINT_DEBUG_SERVER
    m_dbgLastPayload = bodyBytes;
#endif

    QNetworkReply* reply = post("/prompt", bodyBytes);
    connect(reply, &QNetworkReply::finished, this, [this, reply, cb = std::move(cb)]() mutable {
        // rawBody を先に読む（error 時も HTTP レスポンス本文を取得するため）
        const QByteArray rawBody = reply->readAll();
        const int httpStatus =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();
#ifdef PAINT_DEBUG_SERVER
        m_dbgLastResponseStatus = httpStatus;
        m_dbgLastResponseBody   = rawBody;
#endif
        if (reply->error() != QNetworkReply::NoError) {
            cb({}, reply->errorString());
            return;
        }
        const QJsonObject resp = QJsonDocument::fromJson(rawBody).object();
        const QString id = resp.value("prompt_id").toString();
        if (id.isEmpty()) {
            const QString detail = resp.value("error").toString();
            cb({}, detail.isEmpty() ? "No prompt_id in response" : detail);
        } else {
            cb(id, {});
        }
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// waitForOutputs  —  GET /history/{promptId} のポーリング
// ─────────────────────────────────────────────────────────────────────────────
void ComfyClient::waitForOutputs(const QString& promptId,
                                   std::function<void(QStringList, QString)> cb,
                                   int pollIntervalMs,
                                   int timeoutMs) {
    pollHistory(promptId, pollIntervalMs, timeoutMs, std::move(cb));
}

void ComfyClient::pollHistory(const QString& promptId,
                                int pollIntervalMs,
                                int remainingMs,
                                std::function<void(QStringList, QString)> cb) {
    if (remainingMs <= 0) {
        cb({}, QString("Timeout waiting for prompt %1").arg(promptId));
        return;
    }

    QNetworkReply* reply = get("/history/" + promptId);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, promptId, pollIntervalMs, remainingMs,
             cb = std::move(cb)]() mutable {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            cb({}, reply->errorString());
            return;
        }

        const QJsonObject root =
            QJsonDocument::fromJson(reply->readAll()).object();

        // レスポンス: { "<promptId>": { "outputs": {...}, "status": {...} } }
        const QJsonObject entry = root.value(promptId).toObject();

        if (!entry.isEmpty()) {
            const QJsonObject status  = entry.value("status").toObject();
            const bool completed      = status.value("completed").toBool();
            const QString statusStr   = status.value("status_str").toString();

            if (statusStr == "error") {
                const QJsonArray msgs =
                    entry.value("status").toObject()
                         .value("messages").toArray();
                QString errMsg;
                for (const QJsonValue& v : msgs) {
                    const QJsonArray pair = v.toArray();
                    if (pair.size() >= 2 &&
                        pair.at(0).toString() == "execution_error") {
                        errMsg = pair.at(1).toObject()
                                     .value("exception_message").toString();
                        break;
                    }
                }
                cb({}, errMsg.isEmpty() ? "Execution error" : errMsg);
                return;
            }

            if (completed) {
                // outputs: { nodeId: { "images": [{"filename","subfolder","type"},...] } }
                QStringList filenames;
                const QJsonObject outputs = entry.value("outputs").toObject();
                for (const QString& nid : outputs.keys()) {
                    const QJsonArray imgs =
                        outputs.value(nid).toObject().value("images").toArray();
                    for (const QJsonValue& img : imgs) {
                        const QString fname =
                            img.toObject().value("filename").toString();
                        if (!fname.isEmpty()) filenames << fname;
                    }
                }
                cb(filenames, {});
                return;
            }

            // 実行中: progress があれば emit
            const QJsonObject prompt = entry.value("prompt").toArray()
                                           .at(3).toObject();  // unused for now
            (void)prompt;
        }

        // まだ完了していない → 再ポーリング
        const int next = remainingMs - pollIntervalMs;
        QTimer::singleShot(pollIntervalMs, this,
                           [this, promptId, pollIntervalMs, next,
                            cb = std::move(cb)]() mutable {
            pollHistory(promptId, pollIntervalMs, next, std::move(cb));
        });
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// fetchImage  —  GET /view?filename=...&subfolder=...&type=...
// ─────────────────────────────────────────────────────────────────────────────
void ComfyClient::fetchImage(const QString& filename,
                               const QString& subfolder,
                               const QString& type,
                               std::function<void(QByteArray, QString)> cb) {
    QUrl url = m_baseUrl;
    url.setPath("/view");
    QUrlQuery q;
    q.addQueryItem("filename", filename);
    if (!subfolder.isEmpty()) q.addQueryItem("subfolder", subfolder);
    if (!type.isEmpty())      q.addQueryItem("type",      type);
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setTransferTimeout(30000);
    QNetworkReply* reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, cb = std::move(cb)]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            cb({}, reply->errorString());
            return;
        }
        const QByteArray data = reply->readAll();
        if (data.isEmpty()) {
            cb({}, "Empty response from /view");
        } else {
            cb(data, {});
        }
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// fetchLoras  —  GET /object_info/LoraLoader → lora_name の候補一覧
// ─────────────────────────────────────────────────────────────────────────────
void ComfyClient::fetchLoras(std::function<void(QStringList, QString)> cb) {
    QNetworkReply* reply = get("/object_info/LoraLoader");
    connect(reply, &QNetworkReply::finished, this, [reply, cb = std::move(cb)]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            cb({}, reply->errorString());
            return;
        }
        // レスポンス: { "LoraLoader": { "input": { "required": { "lora_name": [["a.safetensors",...], "LORA_MODEL"] } } } }
        const QJsonObject root =
            QJsonDocument::fromJson(reply->readAll()).object();
        const QJsonArray choices =
            root.value("LoraLoader").toObject()
                .value("input").toObject()
                .value("required").toObject()
                .value("lora_name").toArray()
                .at(0).toArray();

        QStringList names;
        names.reserve(choices.size());
        for (const QJsonValue& v : choices) {
            const QString s = v.toString();
            if (!s.isEmpty()) names << s;
        }
        if (names.isEmpty()) {
            cb({}, "No LoRA models found in /object_info/LoraLoader");
        } else {
            cb(names, {});
        }
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// fetchUpscaleModels  —  GET /object_info/UpscaleModelLoader → model_name 候補
// ─────────────────────────────────────────────────────────────────────────────
void ComfyClient::fetchUpscaleModels(std::function<void(QStringList, QString)> cb) {
    QNetworkReply* reply = get("/object_info/UpscaleModelLoader");
    connect(reply, &QNetworkReply::finished, this, [reply, cb = std::move(cb)]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            cb({}, reply->errorString());
            return;
        }
        const QJsonObject root =
            QJsonDocument::fromJson(reply->readAll()).object();
        const QJsonArray choices =
            root.value("UpscaleModelLoader").toObject()
                .value("input").toObject()
                .value("required").toObject()
                .value("model_name").toArray()
                .at(0).toArray();
        QStringList names;
        names.reserve(choices.size());
        for (const QJsonValue& v : choices) {
            const QString s = v.toString();
            if (!s.isEmpty()) names << s;
        }
        cb(names, names.isEmpty() ? QString("No upscale models found") : QString());
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// interrupt  —  POST /interrupt (fire-and-forget)
// ─────────────────────────────────────────────────────────────────────────────
void ComfyClient::interrupt() {
    auto* reply = post("/interrupt", QByteArray{});
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

// ─────────────────────────────────────────────────────────────────────────────
// execute  —  upload → queue → wait → fetch を一括実行
// ─────────────────────────────────────────────────────────────────────────────
void ComfyClient::execute(const ExecuteRequest& req,
                            std::function<void(QByteArray, QString)> cb) {
    // inputPng がある場合はアップロードして LoadImage ノードにバインド
    if (!req.inputPng.isEmpty()) {
        const QString uploadName =
            req.inputNodeId.isEmpty()
                ? "comfy_input.png"
                : QString("comfy_input_%1.png").arg(req.inputNodeId);

        // コピーを保持して非同期コールバック内で使う
        ExecuteRequest reqCopy = req;
        uploadImage(req.inputPng, uploadName,
                    [this, reqCopy = std::move(reqCopy),
                     cb = std::move(cb)](QString savedName, QString err) mutable {
            if (!err.isEmpty()) {
                cb({}, "Upload failed: " + err);
                return;
            }
            // LoadImage ノードにアップロード済みファイル名をバインド
            WorkflowDocument doc = reqCopy.doc;
            const QString nodeId =
                reqCopy.inputNodeId.isEmpty()
                    ? (doc.findNodesByClass("LoadImage").isEmpty()
                           ? QString{}
                           : doc.findNodesByClass("LoadImage").first())
                    : reqCopy.inputNodeId;

            if (!nodeId.isEmpty()) {
                doc.apply(WorkflowBinding::loadImage(nodeId, savedName));
            }

            // queue → wait → fetch
            ExecuteRequest next = reqCopy;
            next.doc     = std::move(doc);
            next.inputPng = {};  // アップロード済みなので不要
            execute(std::move(next), std::move(cb));
        });
        return;
    }

    // キューに投入
    WorkflowDocument docCopy = req.doc;
    const int timeout = req.timeoutMs;
    queueWorkflow(docCopy,
                  [this, timeout, cb = std::move(cb)](QString promptId, QString err) mutable {
        if (!err.isEmpty()) {
            cb({}, "Queue failed: " + err);
            return;
        }
        // 完了を待つ
        waitForOutputs(promptId,
                       [this, cb = std::move(cb)](QStringList files, QString err2) mutable {
            if (!err2.isEmpty()) {
                cb({}, "Wait failed: " + err2);
                return;
            }
            if (files.isEmpty()) {
                cb({}, "No output files");
                return;
            }
            // 最初の出力画像を取得
            fetchImage(files.first(), "", "output",
                       [cb = std::move(cb)](QByteArray data, QString err3) mutable {
                if (!err3.isEmpty()) {
                    cb({}, "Fetch failed: " + err3);
                } else {
                    cb(data, {});
                }
            });
        },
        500, timeout);
    });
}

} // namespace platform::comfy
