#pragma once

#include <functional>

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QUuid>

#include "platform/comfy/WorkflowDocument.h"

QT_FORWARD_DECLARE_CLASS(QNetworkReply)
QT_FORWARD_DECLARE_CLASS(QTimer)

namespace platform::comfy {

// ─────────────────────────────────────────────────────────────────────────────
// ComfyClient
//
// ComfyUI HTTP API クライアント（platform 層 / Qt 依存のみ）。
// WebSocket は使わず GET /history ポーリングで完了を検出する。
//
// ── 基本フロー ────────────────────────────────────────────────────────────────
//   client.uploadImage(png, "input.png", [&](QString name, QString err){...});
//   client.queueWorkflow(doc, [&](QString id, QString err){...});
//   client.waitForOutputs(id, [&](QStringList files, QString err){...});
//   client.fetchImage(file, "", "output", [&](QByteArray data, QString err){...});
//
// ── 一発実行 (execute) ────────────────────────────────────────────────────────
//   ComfyClient::ExecuteRequest req;
//   req.doc        = doc;
//   req.inputPng   = pngBytes;    // アップロードして LoadImage に自動バインド
//   req.inputNodeId = "4";        // 空なら最初の LoadImage ノードを使う
//   req.timeoutMs  = 60000;
//   client.execute(req, [&](QByteArray result, QString err){...});
// ─────────────────────────────────────────────────────────────────────────────
class ComfyClient : public QObject {
  Q_OBJECT

public:
  explicit ComfyClient(QObject* parent = nullptr);

  // ── 設定 ─────────────────────────────────────────────────────────────────
  void setBaseUrl(const QUrl& url) noexcept { m_baseUrl = url; }
  QUrl baseUrl()   const noexcept { return m_baseUrl; }
  QString clientId() const noexcept { return m_clientId; }

  // ── 低レベル API ─────────────────────────────────────────────────────────

  /// PNG バイト列を ComfyUI input フォルダへアップロード。
  /// callback: (savedFilename, errorString) — 空ファイル名 = 失敗。
  void uploadImage(const QByteArray& pngData,
                   const QString& name,
                   std::function<void(QString, QString)> cb);

  /// WorkflowDocument をキューに投入。
  /// callback: (promptId, errorString) — 空 promptId = 失敗。
  void queueWorkflow(const WorkflowDocument& doc,
                     std::function<void(QString, QString)> cb);

  /// promptId の実行完了を待ち、出力ファイル名リストを返す。
  /// pollIntervalMs: ポーリング間隔 (デフォルト 500ms)
  /// timeoutMs:      最大待機時間 (デフォルト 60 秒)
  void waitForOutputs(const QString& promptId,
                      std::function<void(QStringList, QString)> cb,
                      int pollIntervalMs = 500,
                      int timeoutMs      = 60000);

  /// ComfyUI /view からファイルをダウンロード。
  /// callback: (pngBytes, errorString)
  void fetchImage(const QString& filename,
                  const QString& subfolder,
                  const QString& type,
                  std::function<void(QByteArray, QString)> cb);

  // ── 高レベル API ─────────────────────────────────────────────────────────

  struct ExecuteRequest {
    WorkflowDocument doc;

    /// 空でなければアップロードして inputNodeId の LoadImage に自動バインド。
    QByteArray inputPng;

    /// LoadImage バインド先ノード ID (空 = doc 内の最初の LoadImage ノード)。
    QString inputNodeId;

    /// 完了待ちタイムアウト [ms]
    int timeoutMs {60000};
  };

  /// upload → queue → wait → fetch を一括実行。
  /// callback: (resultPng, errorString) — resultPng は最初の出力画像の PNG バイト列。
  void execute(const ExecuteRequest& req,
               std::function<void(QByteArray, QString)> cb);

signals:
  /// waitForOutputs / execute からのポーリング進捗通知 (0..totalSteps; totalSteps=0 は不明)
  void progressUpdate(QString promptId, int step, int totalSteps);

private:
  QNetworkReply* get (const QString& path);
  QNetworkReply* post(const QString& path, const QByteArray& body,
                      const QString& contentType = "application/json");

  // /history/{promptId} をポーリングしてファイル名を収集するヘルパー
  void pollHistory(const QString& promptId,
                   int pollIntervalMs, int remainingMs,
                   std::function<void(QStringList, QString)> cb);

  QUrl                  m_baseUrl   {"http://localhost:8188"};
  QString               m_clientId  {QUuid::createUuid().toString(QUuid::WithoutBraces)};
  QNetworkAccessManager m_nam       {this};
};

} // namespace platform::comfy
