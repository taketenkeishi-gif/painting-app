#pragma once

#include <functional>
#include <map>

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPixmap>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(QNetworkReply)

namespace app::bridge { class MinimalWebSocket; }

namespace app::bridge {

// ─────────────────────────────────────────────────────────────────────────────
// ComfyUiClient
//
// ComfyUI HTTP REST + WebSocket クライアント。
// デフォルト URL: http://localhost:8188
//
// ── 接続フロー ──────────────────────────────────────────────────────────────
//   connectToServer()  →  GET /system_stats  →  WebSocket /ws  →  接続完了
//
// ── ワークフロー実行フロー ──────────────────────────────────────────────────
//   queuePrompt(workflow)  →  POST /prompt  →  promptId を記録
//   WebSocket イベント (progress / executing / executed) で進捗・完了を検出
//   executionComplete / executionError シグナルを emit
//   fetchImage(filename)  →  GET /view?filename=...  →  QByteArray (PNG)
//
// ── プレビュー ──────────────────────────────────────────────────────────────
//   ComfyUI が WebSocket バイナリメッセージで KSampler 中間プレビューを送信。
//   先頭 4 バイト = event_type (1=preview), 次 4 バイト = format, 残り = 画像。
//   previewImageReceived シグナルで QPixmap を emit。
// ─────────────────────────────────────────────────────────────────────────────
class ComfyUiClient : public QObject {
  Q_OBJECT

public:
  enum class State {
    Disconnected,
    Connecting,
    Connected,
    Error,
  };

  struct InpaintRequest {
    QString imageBase64;       ///< 入力画像 (PNG, base64)
    QString maskBase64;        ///< マスク画像 (白=塗りつぶし範囲, base64)
    QString prompt;
    QString negativePrompt;
    int     steps          {20};
    float   cfg            {7.5f};
    float   denoise        {0.80f};
    int     seed           {-1};
    QString checkpointName {"v1-5-pruned-emaonly.ckpt"};
  };

  explicit ComfyUiClient(QObject* parent = nullptr);
  ~ComfyUiClient() override;

  // ── 接続管理 ─────────────────────────────────────────────────────────────
  void connectToServer(const QUrl& url = QUrl("http://localhost:8188"));
  void disconnect();
  State state()      const noexcept { return m_state; }
  bool  isConnected()const noexcept { return m_state == State::Connected; }
  QUrl  serverUrl()  const noexcept { return m_baseUrl; }
  QString clientId() const noexcept { return m_clientId; }

  // ── API ──────────────────────────────────────────────────────────────────
  /// ワークフロー JSON をキューに追加。非同期。
  void queuePrompt(const QJsonObject& workflow,
                   std::function<void(QString)> onQueued = {});

  /// ComfyUI view エンドポイントから画像バイト列を取得
  void fetchImage(const QString& filename,
                  const QString& subfolder,
                  const QString& type,
                  std::function<void(QByteArray)> callback);

  /// PNG バイト列を ComfyUI input フォルダへアップロード。
  /// callback の引数は ComfyUI が付けた保存ファイル名 (空文字=失敗)
  void uploadImage(const QByteArray& pngData, const QString& name,
                   std::function<void(QString)> callback);

  /// 実行中のプロンプトをキャンセル
  void interruptExecution();

  /// 利用可能なチェックポイント名一覧を取得
  void fetchCheckpoints(std::function<void(QStringList)> callback);

  /// 利用可能な LoRA ファイル名一覧を取得
  void fetchLoras(std::function<void(QStringList)> callback);

  // ── 組み込みワークフロー ─────────────────────────────────────────────────
  static QJsonObject buildInpaintWorkflow(const InpaintRequest& req);

  /// ワークフロー JSON に prompt/seed/checkpoint を注入して返す。
  /// CLIPTextEncode ノード(1番目=positive, 2番目=negative), KSampler, CheckpointLoader を検索。
  static QJsonObject injectWorkflowParams(
      QJsonObject workflow,
      const QString& positivePrompt,
      const QString& negativePrompt,
      int seed,
      const QString& checkpoint = {});

signals:
  void stateChanged       (State newState);
  void connectionError    (QString message);
  /// progress: step/totalSteps + 現在ノード ID (class_type があれば)
  void progressUpdate     (QString promptId, int step, int totalSteps, QString nodeId);
  void executionComplete  (QString promptId, QStringList outputImages);
  void executionError     (QString promptId, QString message);
  void systemStatsReceived(QJsonObject stats);
  /// KSampler 中間プレビュー（WebSocket バイナリ）
  void previewImageReceived(QPixmap preview);

private slots:
  void onWsTextMessage  (const QString& text);
  void onWsBinaryMessage(const QByteArray& data);
  void onWsDisconnected ();

private:
  void setState(State s);
  void connectWebSocket();
  QNetworkReply* get (const QString& path);
  QNetworkReply* post(const QString& path, const QByteArray& body,
                      const QString& contentType = "application/json");

  // HTTP
  QUrl                  m_baseUrl   {"http://localhost:8188"};
  QString               m_clientId  {QUuid::createUuid().toString(QUuid::WithoutBraces)};
  QNetworkAccessManager m_nam       {this};
  State                 m_state     {State::Disconnected};
  QTimer                m_reconnectTimer {this};

  // WebSocket
  MinimalWebSocket* m_ws {nullptr};

  // 追跡中のプロンプト: promptId → 収集済み出力画像ファイル名リスト
  std::map<QString, QStringList> m_pendingOutputs;
};

} // namespace app::bridge
