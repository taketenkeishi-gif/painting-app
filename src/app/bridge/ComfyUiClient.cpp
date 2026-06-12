#include "app/bridge/ComfyUiClient.h"

#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include "app/bridge/MinimalWebSocket.h"

namespace app::bridge {

// ─────────────────────────────────────────────────────────────────────────────
// ctor / dtor
// ─────────────────────────────────────────────────────────────────────────────
ComfyUiClient::ComfyUiClient(QObject* parent)
    : QObject(parent) {
  // 再接続タイマー (接続失敗時のバックオフ)
  m_reconnectTimer.setInterval(8000);
  m_reconnectTimer.setSingleShot(true);
  connect(&m_reconnectTimer, &QTimer::timeout, this, [this]() {
    if (m_state == State::Error || m_state == State::Disconnected) {
      connectToServer(m_baseUrl);
    }
  });
}

ComfyUiClient::~ComfyUiClient() = default;

// ─────────────────────────────────────────────────────────────────────────────
// 接続管理
// ─────────────────────────────────────────────────────────────────────────────
void ComfyUiClient::connectToServer(const QUrl& url) {
  m_baseUrl = url;
  setState(State::Connecting);

  // HTTP で /system_stats を確認してから WebSocket を開く
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
    emit systemStatsReceived(
        QJsonDocument::fromJson(reply->readAll()).object());
    // HTTP 到達確認後に WebSocket を接続
    connectWebSocket();
  });
}

void ComfyUiClient::connectWebSocket() {
  // 既存 WS があれば閉じて削除
  if (m_ws != nullptr) {
    m_ws->close();
    m_ws->deleteLater();
    m_ws = nullptr;
  }
  // (m_ws will be set below)

  m_ws = new MinimalWebSocket(this);

  connect(m_ws, &MinimalWebSocket::connected, this, [this]() {
    setState(State::Connected);
  });
  connect(m_ws, &MinimalWebSocket::disconnected,
          this, &ComfyUiClient::onWsDisconnected);
  connect(m_ws, &MinimalWebSocket::textMessageReceived,
          this, &ComfyUiClient::onWsTextMessage);
  connect(m_ws, &MinimalWebSocket::binaryMessageReceived,
          this, &ComfyUiClient::onWsBinaryMessage);

  // ws://host:port/ws?clientId=<uuid>
  QUrl wsUrl = m_baseUrl;
  wsUrl.setScheme(wsUrl.scheme() == "https" ? "wss" : "ws");
  wsUrl.setPath("/ws");
  QUrlQuery q;
  q.addQueryItem("clientId", m_clientId);
  wsUrl.setQuery(q);
  // MinimalWebSocket uses ws:// scheme — convert
  wsUrl.setScheme(wsUrl.scheme() == "wss" ? "wss" : "ws");
  m_ws->open(wsUrl);
}

void ComfyUiClient::disconnect() {
  m_reconnectTimer.stop();
  m_pendingOutputs.clear();
  if (m_ws != nullptr) {
    m_ws->close();
    // m_ws is parented to this; do not deleteLater here — just nullify the pointer
    // (it will be replaced next connect or destroyed with this object)
  }
  setState(State::Disconnected);
}

void ComfyUiClient::setState(State s) {
  if (m_state != s) {
    m_state = s;
    emit stateChanged(s);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// WebSocket メッセージハンドラ
// ─────────────────────────────────────────────────────────────────────────────
void ComfyUiClient::onWsTextMessage(const QString& text) {
  const QJsonObject obj = QJsonDocument::fromJson(text.toUtf8()).object();
  const QString type    = obj.value("type").toString();
  const QJsonObject data = obj.value("data").toObject();
  const QString promptId = data.value("prompt_id").toString();

  if (type == "progress") {
    const int step    = data.value("value").toInt();
    const int maxStep = data.value("max").toInt();
    const QString nodeId = data.value("node").toString();
    if (m_pendingOutputs.count(promptId) || promptId.isEmpty()) {
      emit progressUpdate(promptId, step, maxStep, nodeId);
    }

  } else if (type == "executing") {
    const QString nodeId = data.value("node").toString();
    if (nodeId.isEmpty()) {
      // node == null → このプロンプトの実行完了
      auto it = m_pendingOutputs.find(promptId);
      if (it != m_pendingOutputs.end()) {
        const QStringList images = it->second;
        m_pendingOutputs.erase(it);
        emit executionComplete(promptId, images);
      }
    }

  } else if (type == "executed") {
    // ノード完了 → 出力画像を収集
    const QJsonObject output = data.value("output").toObject();
    const QJsonArray  images = output.value("images").toArray();
    auto it = m_pendingOutputs.find(promptId);
    if (it != m_pendingOutputs.end()) {
      for (const QJsonValue& imgVal : images) {
        const QString fname = imgVal.toObject().value("filename").toString();
        if (!fname.isEmpty()) {
          it->second.append(fname);
        }
      }
    }

  } else if (type == "execution_error") {
    auto it = m_pendingOutputs.find(promptId);
    if (it != m_pendingOutputs.end()) {
      m_pendingOutputs.erase(it);
      const QString msg = data.value("exception_message").toString();
      emit executionError(promptId, msg.isEmpty() ? "不明なエラー" : msg);
    }

  } else if (type == "execution_interrupted") {
    auto it = m_pendingOutputs.find(promptId);
    if (it != m_pendingOutputs.end()) {
      m_pendingOutputs.erase(it);
      emit executionError(promptId, "実行がキャンセルされました");
    }
  }
}

void ComfyUiClient::onWsBinaryMessage(const QByteArray& data) {
  // ComfyUI プレビュー形式:
  //   [0..3]  event_type (LE uint32): 1 = preview image
  //   [4..7]  image_type (LE uint32): 1 = JPEG, 2 = PNG
  //   [8..]   画像データ
  if (data.size() < 8) return;

  const auto readU32LE = [&](int offset) -> quint32 {
    return static_cast<quint32>(static_cast<unsigned char>(data[offset]))
         | (static_cast<quint32>(static_cast<unsigned char>(data[offset+1])) << 8)
         | (static_cast<quint32>(static_cast<unsigned char>(data[offset+2])) << 16)
         | (static_cast<quint32>(static_cast<unsigned char>(data[offset+3])) << 24);
  };

  const quint32 eventType = readU32LE(0);
  if (eventType != 1) return;  // 1 = preview image

  const QByteArray imgData = data.mid(8);
  QPixmap px;
  if (px.loadFromData(imgData)) {
    emit previewImageReceived(px);
  }
}

void ComfyUiClient::onWsDisconnected() {
  if (m_state == State::Connected) {
    setState(State::Error);
    m_reconnectTimer.start();
  }
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
void ComfyUiClient::queuePrompt(const QJsonObject& workflow,
                                 std::function<void(QString)> onQueued) {
  QJsonObject body;
  body.insert("prompt",    workflow);
  body.insert("client_id", m_clientId);
  const QByteArray bodyBytes = QJsonDocument(body).toJson(QJsonDocument::Compact);
  QNetworkReply* reply = post("/prompt", bodyBytes);

  connect(reply, &QNetworkReply::finished, this, [this, reply, onQueued]() {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      qWarning() << "ComfyUI /prompt POST failed:" << reply->errorString();
      if (onQueued) onQueued({});
      return;
    }
    const QJsonObject resp = QJsonDocument::fromJson(reply->readAll()).object();
    const QString id = resp.value("prompt_id").toString();
    if (!id.isEmpty()) {
      m_pendingOutputs[id] = {};  // register as pending
    }
    if (onQueued) onQueued(id);
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
// ワークフローパラメーター注入
// ─────────────────────────────────────────────────────────────────────────────
QJsonObject ComfyUiClient::injectWorkflowParams(
    QJsonObject workflow,
    const QString& positivePrompt,
    const QString& negativePrompt,
    int seed,
    const QString& checkpoint) {
  int clipEncodeCount = 0;
  for (const QString& nodeId : workflow.keys()) {
    QJsonObject node = workflow.value(nodeId).toObject();
    const QString classType = node.value("class_type").toString();
    QJsonObject inputs = node.value("inputs").toObject();

    if (classType == "CLIPTextEncode") {
      ++clipEncodeCount;
      if (clipEncodeCount == 1 && !positivePrompt.isEmpty()) {
        inputs["text"] = positivePrompt;
      } else if (clipEncodeCount == 2 && !negativePrompt.isEmpty()) {
        inputs["text"] = negativePrompt;
      }
      node["inputs"] = inputs;
      workflow[nodeId] = node;

    } else if (classType == "KSampler" || classType == "KSamplerAdvanced") {
      if (seed >= 0) {
        inputs["seed"] = seed;
        node["inputs"] = inputs;
        workflow[nodeId] = node;
      }

    } else if (classType == "CheckpointLoaderSimple" && !checkpoint.isEmpty()) {
      inputs["ckpt_name"] = checkpoint;
      node["inputs"] = inputs;
      workflow[nodeId] = node;
    }
  }
  return workflow;
}

// ─────────────────────────────────────────────────────────────────────────────
// 組み込みワークフロー: SD インペイント
// ─────────────────────────────────────────────────────────────────────────────
QJsonObject ComfyUiClient::buildInpaintWorkflow(const InpaintRequest& req) {
  QJsonObject wf;

  // 1: Checkpoint loader
  { QJsonObject n; QJsonObject inp;
    inp.insert("ckpt_name", req.checkpointName);
    n.insert("class_type", "CheckpointLoaderSimple");
    n.insert("inputs", inp);
    wf.insert("1", n); }
  // 2: Positive prompt
  { QJsonObject n; QJsonObject inp;
    inp.insert("text", req.prompt.isEmpty() ? "high quality, detailed" : req.prompt);
    inp.insert("clip", QJsonArray{QJsonArray{"1"}, 1});
    n.insert("class_type", "CLIPTextEncode");
    n.insert("inputs", inp);
    wf.insert("2", n); }
  // 3: Negative prompt
  { QJsonObject n; QJsonObject inp;
    inp.insert("text", req.negativePrompt.isEmpty()
                           ? "blurry, low quality, artifacts" : req.negativePrompt);
    inp.insert("clip", QJsonArray{QJsonArray{"1"}, 1});
    n.insert("class_type", "CLIPTextEncode");
    n.insert("inputs", inp);
    wf.insert("3", n); }
  // 4: Load image
  { QJsonObject n; QJsonObject inp;
    inp.insert("image",  "comfyui_input.png");
    inp.insert("upload", "image");
    n.insert("class_type", "LoadImage");
    n.insert("inputs", inp);
    wf.insert("4", n); }
  // 5: Load mask
  { QJsonObject n; QJsonObject inp;
    inp.insert("image",  "comfyui_mask.png");
    inp.insert("upload", "image");
    inp.insert("channel","red");
    n.insert("class_type", "LoadImageMask");
    n.insert("inputs", inp);
    wf.insert("5", n); }
  // 6: VAE encode for inpaint
  { QJsonObject n; QJsonObject inp;
    inp.insert("pixels",       QJsonArray{QJsonArray{"4"}, 0});
    inp.insert("vae",          QJsonArray{QJsonArray{"1"}, 2});
    inp.insert("mask",         QJsonArray{QJsonArray{"5"}, 0});
    inp.insert("grow_mask_by", 6);
    n.insert("class_type", "VAEEncodeForInpaint");
    n.insert("inputs", inp);
    wf.insert("6", n); }
  // 7: KSampler
  { QJsonObject n; QJsonObject inp;
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
    wf.insert("7", n); }
  // 8: VAE decode
  { QJsonObject n; QJsonObject inp;
    inp.insert("samples", QJsonArray{QJsonArray{"7"}, 0});
    inp.insert("vae",     QJsonArray{QJsonArray{"1"}, 2});
    n.insert("class_type", "VAEDecode");
    n.insert("inputs", inp);
    wf.insert("8", n); }
  // 9: Save image
  { QJsonObject n; QJsonObject inp;
    inp.insert("images",          QJsonArray{QJsonArray{"8"}, 0});
    inp.insert("filename_prefix", "paintapp_inpaint");
    n.insert("class_type", "SaveImage");
    n.insert("inputs", inp);
    wf.insert("9", n); }
  return wf;
}

// ─────────────────────────────────────────────────────────────────────────────
// 組み込みワークフロー: SAM2 オブジェクト選択
// ─────────────────────────────────────────────────────────────────────────────
QJsonObject ComfyUiClient::buildSamWorkflow(const SamRequest& req) {
  QJsonObject wf;

  { QJsonObject n; QJsonObject inp;
    inp.insert("image",  "sam_input.png");
    inp.insert("upload", "image");
    n.insert("class_type", "LoadImage");
    n.insert("inputs", inp);
    wf.insert("1", n); }
  { QJsonObject n; QJsonObject inp;
    inp.insert("model",  req.samModel);
    inp.insert("device", "cuda");
    n.insert("class_type", "SAM2ModelLoader");
    n.insert("inputs", inp);
    wf.insert("2", n); }
  { QJsonObject n; QJsonObject inp;
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
    wf.insert("3", n); }
  { QJsonObject n; QJsonObject inp;
    inp.insert("mask", QJsonArray{QJsonArray{"3"}, 0});
    n.insert("class_type", "MaskToImage");
    n.insert("inputs", inp);
    wf.insert("4", n); }
  { QJsonObject n; QJsonObject inp;
    inp.insert("images",          QJsonArray{QJsonArray{"4"}, 0});
    inp.insert("filename_prefix", "paintapp_sam");
    n.insert("class_type", "SaveImage");
    n.insert("inputs", inp);
    wf.insert("5", n); }
  return wf;
}

// ─────────────────────────────────────────────────────────────────────────────
// fetchUpscaleModels
// ─────────────────────────────────────────────────────────────────────────────
void ComfyUiClient::fetchUpscaleModels(std::function<void(QStringList)> callback) {
  auto* reply = get("/object_info/UpscaleModelLoader");
  connect(reply, &QNetworkReply::finished, this, [reply, cb = std::move(callback)]() {
    QStringList models;
    if (reply->error() == QNetworkReply::NoError) {
      const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
      // {"UpscaleModelLoader": {"input": {"required": {"model_name": [[name,...], {}]}}}}
      const QJsonArray names =
          root["UpscaleModelLoader"].toObject()
              ["input"].toObject()
              ["required"].toObject()
              ["model_name"].toArray()
              .first().toArray();
      for (const QJsonValue& v : names) {
        const QString s = v.toString();
        if (!s.isEmpty()) models << s;
      }
    }
    if (cb) cb(models);
    reply->deleteLater();
  });
}

// ─────────────────────────────────────────────────────────────────────────────
// buildUpscaleWorkflow
// ─────────────────────────────────────────────────────────────────────────────
QJsonObject ComfyUiClient::buildUpscaleWorkflow(const QString& inputFilename,
                                                  const QString& modelName) {
  // Node 1: LoadImage
  QJsonObject n1, i1;
  i1["image"]  = inputFilename;
  i1["upload"] = QString("image");
  n1["class_type"] = "LoadImage";  n1["inputs"] = i1;

  // Node 2: UpscaleModelLoader
  QJsonObject n2, i2;
  i2["model_name"] = modelName;
  n2["class_type"] = "UpscaleModelLoader";  n2["inputs"] = i2;

  // Node 3: ImageUpscaleWithModel
  QJsonObject n3, i3;
  i3["upscale_model"] = QJsonArray{QJsonArray{"2"}, 0};
  i3["image"]         = QJsonArray{QJsonArray{"1"}, 0};
  n3["class_type"] = "ImageUpscaleWithModel";  n3["inputs"] = i3;

  // Node 4: SaveImage
  QJsonObject n4, i4;
  i4["images"]          = QJsonArray{QJsonArray{"3"}, 0};
  i4["filename_prefix"] = QString("lpa_upscale_");
  n4["class_type"] = "SaveImage";  n4["inputs"] = i4;

  return QJsonObject{{"1", n1}, {"2", n2}, {"3", n3}, {"4", n4}};
}

} // namespace app::bridge
