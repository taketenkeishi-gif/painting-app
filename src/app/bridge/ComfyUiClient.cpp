#include "app/bridge/ComfyUiClient.h"

#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

namespace app::bridge {

// ─────────────────────────────────────────────────────────────────────────────
// ctor / dtor
// ─────────────────────────────────────────────────────────────────────────────
ComfyUiClient::ComfyUiClient(QObject* parent)
    : QObject(parent) {
  // 再接続タイマー
  m_reconnectTimer.setInterval(8000);
  m_reconnectTimer.setSingleShot(true);
  connect(&m_reconnectTimer, &QTimer::timeout, this, [this]() {
    if (m_state == State::Error || m_state == State::Disconnected) {
      connectToServer(m_baseUrl);
    }
  });

  // ポーリングタイマー (1 秒ごとに未完了プロンプトを確認)
  m_pollTimer.setInterval(1000);
  connect(&m_pollTimer, &QTimer::timeout, this, &ComfyUiClient::onPollTimer);
}

ComfyUiClient::~ComfyUiClient() = default;

// ─────────────────────────────────────────────────────────────────────────────
// 接続管理
// ─────────────────────────────────────────────────────────────────────────────
void ComfyUiClient::connectToServer(const QUrl& url) {
  m_baseUrl = url;
  setState(State::Connecting);

  // /system_stats で HTTP 到達性を確認
  QNetworkReply* reply = get("/system_stats");
  connect(reply, &QNetworkReply::finished, this, [this, reply]() {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      emit connectionError(
          QString("ComfyUI に接続できません: %1").arg(reply->errorString()));
      setState(State::Error);
      m_reconnectTimer.start();
      return;
    }
    setState(State::Connected);
    emit systemStatsReceived(
        QJsonDocument::fromJson(reply->readAll()).object());
  });
}

void ComfyUiClient::disconnect() {
  m_reconnectTimer.stop();
  m_pollTimer.stop();
  m_pendingPrompts.clear();
  setState(State::Disconnected);
}

void ComfyUiClient::setState(State s) {
  if (m_state != s) {
    m_state = s;
    emit stateChanged(s);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// ポーリング
// ─────────────────────────────────────────────────────────────────────────────
void ComfyUiClient::startPolling(const QString& promptId) {
  m_pendingPrompts[promptId] = 0;
  if (!m_pollTimer.isActive()) {
    m_pollTimer.start();
  }
}

void ComfyUiClient::onPollTimer() {
  if (m_pendingPrompts.empty()) {
    m_pollTimer.stop();
    return;
  }

  // ひとつずつ /history/{id} を確認（同時リクエストを抑制するため先頭のみ）
  auto it = m_pendingPrompts.begin();
  const QString promptId = it->first;
  int& count = it->second;
  ++count;

  // タイムアウト: 300 秒 (300 ポーリング × 1 秒)
  if (count > 300) {
    m_pendingPrompts.erase(it);
    emit executionError(promptId, "タイムアウト: 実行が 300 秒以内に完了しませんでした");
    return;
  }

  getHistory(promptId, [this, promptId](const QJsonObject& history) {
    // history = { "<promptId>": { "outputs": {...}, "status": {...} } }
    if (!history.contains(promptId)) {
      return; // まだ完了していない
    }
    m_pendingPrompts.erase(promptId);
    if (m_pendingPrompts.empty()) {
      m_pollTimer.stop();
    }

    const QJsonObject entry  = history.value(promptId).toObject();
    const QJsonObject status = entry.value("status").toObject();
    if (status.value("status_str").toString() == "error") {
      const QString msg = status.value("messages").toArray()
                              .last().toArray().last().toObject()
                              .value("exception_message").toString();
      emit executionError(promptId, msg.isEmpty() ? "不明なエラー" : msg);
      return;
    }

    // 出力画像を収集
    QStringList images;
    const QJsonObject outputs = entry.value("outputs").toObject();
    for (const QString& nodeId : outputs.keys()) {
      const QJsonObject nodeOut = outputs.value(nodeId).toObject();
      for (const QJsonValue& imgVal : nodeOut.value("images").toArray()) {
        const QString fname = imgVal.toObject().value("filename").toString();
        if (!fname.isEmpty()) images.push_back(fname);
      }
    }
    emit executionComplete(promptId, images);
  });
}

// ─────────────────────────────────────────────────────────────────────────────
// HTTP ヘルパー
// ─────────────────────────────────────────────────────────────────────────────
QNetworkReply* ComfyUiClient::get(const QString& path) {
  QUrl url = m_baseUrl;
  url.setPath(path);
  QNetworkRequest req(url);
  req.setRawHeader("Accept", "application/json");
  return m_nam.get(req);
}

QNetworkReply* ComfyUiClient::post(const QString& path,
                                    const QByteArray& body,
                                    const QString& contentType) {
  QUrl url = m_baseUrl;
  url.setPath(path);
  QNetworkRequest req(url);
  req.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
  return m_nam.post(req, body);
}

// ─────────────────────────────────────────────────────────────────────────────
// API
// ─────────────────────────────────────────────────────────────────────────────
QString ComfyUiClient::queuePrompt(const QJsonObject& workflow) {
  QJsonObject body;
  body.insert("prompt",    workflow);
  body.insert("client_id", m_clientId);
  const QByteArray bodyBytes = QJsonDocument(body).toJson(QJsonDocument::Compact);
  QNetworkReply* reply = post("/prompt", bodyBytes);

  // POST 完了後に返ってくる {"prompt_id": "..."} からIDを取得してポーリング開始
  connect(reply, &QNetworkReply::finished, this, [this, reply]() {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) return;
    const QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
    const QString id = resp.value("prompt_id").toString();
    if (!id.isEmpty()) {
      startPolling(id);
    }
  });
  return {}; // promptId は POST レスポンスから非同期で取得
}

void ComfyUiClient::getHistory(const QString& promptId,
                                std::function<void(QJsonObject)> callback) {
  QNetworkReply* reply = get(QString("/history/%1").arg(promptId));
  connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      callback({});
      return;
    }
    callback(QJsonDocument::fromJson(reply->readAll()).object());
  });
}

void ComfyUiClient::fetchImage(const QString& filename,
                                const QString& subfolder,
                                const QString& type,
                                std::function<void(QByteArray)> callback) {
  QUrl url = m_baseUrl;
  url.setPath("/view");
  QUrlQuery query;
  query.addQueryItem("filename", filename);
  if (!subfolder.isEmpty()) query.addQueryItem("subfolder", subfolder);
  if (!type.isEmpty())      query.addQueryItem("type", type);
  url.setQuery(query);
  QNetworkRequest req(url);
  QNetworkReply* reply = m_nam.get(req);
  connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      callback({});
      return;
    }
    callback(reply->readAll());
  });
}

void ComfyUiClient::interruptExecution() {
  QNetworkReply* reply = post("/interrupt", {});
  connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void ComfyUiClient::uploadImage(const QByteArray& pngData,
                                 const QString& name,
                                 std::function<void(QString)> callback) {
  QUrl url = m_baseUrl;
  url.setPath("/upload/image");

  auto* multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

  QHttpPart imagePart;
  imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/png"));
  imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
      QVariant(QString("form-data; name=\"image\"; filename=\"%1\"").arg(name)));
  imagePart.setBody(pngData);
  multipart->append(imagePart);

  QHttpPart typePart;
  typePart.setHeader(QNetworkRequest::ContentDispositionHeader,
      QVariant("form-data; name=\"type\""));
  typePart.setBody("input");
  multipart->append(typePart);

  QHttpPart overwritePart;
  overwritePart.setHeader(QNetworkRequest::ContentDispositionHeader,
      QVariant("form-data; name=\"overwrite\""));
  overwritePart.setBody("true");
  multipart->append(overwritePart);

  QNetworkRequest req(url);
  QNetworkReply* reply = m_nam.post(req, multipart);
  multipart->setParent(reply);

  connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      callback({});
      return;
    }
    const QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
    callback(resp.value("name").toString());
  });
}

void ComfyUiClient::fetchCheckpoints(std::function<void(QStringList)> callback) {
  QNetworkReply* reply = get("/object_info/CheckpointLoaderSimple");
  connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
    reply->deleteLater();
    QStringList names;
    if (reply->error() == QNetworkReply::NoError) {
      const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
      const QJsonObject info = obj.value("CheckpointLoaderSimple").toObject();
      const QJsonObject input = info.value("input").toObject();
      const QJsonObject required = input.value("required").toObject();
      const QJsonArray ckptArr = required.value("ckpt_name").toArray()
                                     .first().toArray();
      for (const QJsonValue& v : ckptArr) {
        names << v.toString();
      }
    }
    callback(names);
  });
}

// ─────────────────────────────────────────────────────────────────────────────
// 組み込みワークフロー: SD インペイント
// ─────────────────────────────────────────────────────────────────────────────
QJsonObject ComfyUiClient::buildInpaintWorkflow(const InpaintRequest& req) {
  QJsonObject wf;

  // 1: Checkpoint loader
  {
    QJsonObject n; QJsonObject inp;
    inp.insert("ckpt_name", req.checkpointName);
    n.insert("class_type", "CheckpointLoaderSimple");
    n.insert("inputs", inp);
    wf.insert("1", n);
  }
  // 2: Positive prompt
  {
    QJsonObject n; QJsonObject inp;
    inp.insert("text", req.prompt.isEmpty() ? "high quality, detailed" : req.prompt);
    inp.insert("clip", QJsonArray{QJsonArray{"1"}, 1});
    n.insert("class_type", "CLIPTextEncode");
    n.insert("inputs", inp);
    wf.insert("2", n);
  }
  // 3: Negative prompt
  {
    QJsonObject n; QJsonObject inp;
    inp.insert("text", req.negativePrompt.isEmpty()
                           ? "blurry, low quality, artifacts" : req.negativePrompt);
    inp.insert("clip", QJsonArray{QJsonArray{"1"}, 1});
    n.insert("class_type", "CLIPTextEncode");
    n.insert("inputs", inp);
    wf.insert("3", n);
  }
  // 4: Load image
  {
    QJsonObject n; QJsonObject inp;
    inp.insert("image", "comfyui_input.png");
    inp.insert("upload", "image");
    n.insert("class_type", "LoadImage");
    n.insert("inputs", inp);
    wf.insert("4", n);
  }
  // 5: Load mask
  {
    QJsonObject n; QJsonObject inp;
    inp.insert("image", "comfyui_mask.png");
    inp.insert("upload", "image");
    inp.insert("channel", "red");
    n.insert("class_type", "LoadImageMask");
    n.insert("inputs", inp);
    wf.insert("5", n);
  }
  // 6: VAE encode for inpaint
  {
    QJsonObject n; QJsonObject inp;
    inp.insert("pixels",       QJsonArray{QJsonArray{"4"}, 0});
    inp.insert("vae",          QJsonArray{QJsonArray{"1"}, 2});
    inp.insert("mask",         QJsonArray{QJsonArray{"5"}, 0});
    inp.insert("grow_mask_by", 6);
    n.insert("class_type", "VAEEncodeForInpaint");
    n.insert("inputs", inp);
    wf.insert("6", n);
  }
  // 7: KSampler
  {
    QJsonObject n; QJsonObject inp;
    inp.insert("model",        QJsonArray{QJsonArray{"1"}, 0});
    inp.insert("positive",     QJsonArray{QJsonArray{"2"}, 0});
    inp.insert("negative",     QJsonArray{QJsonArray{"3"}, 0});
    inp.insert("latent_image", QJsonArray{QJsonArray{"6"}, 0});
    inp.insert("seed",         req.seed < 0 ? 0 : req.seed);
    inp.insert("steps",        req.steps);
    inp.insert("cfg",          static_cast<double>(req.cfg));
    inp.insert("sampler_name", "euler");
    inp.insert("scheduler",    "normal");
    inp.insert("denoise",      static_cast<double>(req.denoise));
    n.insert("class_type", "KSampler");
    n.insert("inputs", inp);
    wf.insert("7", n);
  }
  // 8: VAE decode
  {
    QJsonObject n; QJsonObject inp;
    inp.insert("samples", QJsonArray{QJsonArray{"7"}, 0});
    inp.insert("vae",     QJsonArray{QJsonArray{"1"}, 2});
    n.insert("class_type", "VAEDecode");
    n.insert("inputs", inp);
    wf.insert("8", n);
  }
  // 9: Save image
  {
    QJsonObject n; QJsonObject inp;
    inp.insert("images",          QJsonArray{QJsonArray{"8"}, 0});
    inp.insert("filename_prefix", "paintapp_inpaint");
    n.insert("class_type", "SaveImage");
    n.insert("inputs", inp);
    wf.insert("9", n);
  }
  return wf;
}

// ─────────────────────────────────────────────────────────────────────────────
// 組み込みワークフロー: SAM2 オブジェクト選択
// ─────────────────────────────────────────────────────────────────────────────
QJsonObject ComfyUiClient::buildSamWorkflow(const SamRequest& req) {
  QJsonObject wf;

  // 1: Load input image
  {
    QJsonObject n; QJsonObject inp;
    inp.insert("image",  "sam_input.png");
    inp.insert("upload", "image");
    n.insert("class_type", "LoadImage");
    n.insert("inputs", inp);
    wf.insert("1", n);
  }
  // 2: SAM2 model loader
  {
    QJsonObject n; QJsonObject inp;
    inp.insert("model",  req.samModel);
    inp.insert("device", "cuda");
    n.insert("class_type", "SAM2ModelLoader");
    n.insert("inputs", inp);
    wf.insert("2", n);
  }
  // 3: SAM2 segmentation (point prompt)
  {
    QJsonObject n; QJsonObject inp;
    inp.insert("sam2_model", QJsonArray{QJsonArray{"2"}, 0});
    inp.insert("image",      QJsonArray{QJsonArray{"1"}, 0});
    inp.insert("coordinates_positive",
               req.positivePoint
                   ? QString("[[%1, %2]]").arg(req.pointX).arg(req.pointY)
                   : QString("[]"));
    inp.insert("coordinates_negative",
               req.positivePoint
                   ? QString("[]")
                   : QString("[[%1, %2]]").arg(req.pointX).arg(req.pointY));
    inp.insert("mask_hint_threshold", 0.5);
    n.insert("class_type", "SAM2Segmentation");
    n.insert("inputs", inp);
    wf.insert("3", n);
  }
  // 4: Mask → Image
  {
    QJsonObject n; QJsonObject inp;
    inp.insert("mask", QJsonArray{QJsonArray{"3"}, 0});
    n.insert("class_type", "MaskToImage");
    n.insert("inputs", inp);
    wf.insert("4", n);
  }
  // 5: Save mask image
  {
    QJsonObject n; QJsonObject inp;
    inp.insert("images",          QJsonArray{QJsonArray{"4"}, 0});
    inp.insert("filename_prefix", "paintapp_sam");
    n.insert("class_type", "SaveImage");
    n.insert("inputs", inp);
    wf.insert("5", n);
  }
  return wf;
}

} // namespace app::bridge
