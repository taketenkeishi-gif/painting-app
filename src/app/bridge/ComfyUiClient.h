#pragma once

#include <functional>
#include <map>

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(QNetworkReply)

namespace app::bridge {

// ─────────────────────────────────────────────────────────────────────────────
// ComfyUiClient
//
// ComfyUI の HTTP REST API に接続するクライアント。
// デフォルト URL: http://localhost:8188
//
// ── 接続フロー ──────────────────────────────────────────────────────────────
//   connectToServer()  →  健全性確認 (GET /system_stats)  →  接続完了
//
// ── ワークフロー実行フロー ──────────────────────────────────────────────────
//   queuePrompt(workflow)  →  POST /prompt  →  promptId を返す
//   内部ポーリング (GET /history/{promptId}) で完了を検出
//   executionComplete / executionError シグナルを emit
//   fetchImage(filename)   →  GET /view?filename=... →  QByteArray (PNG)
//
// ── 組み込みワークフロー ────────────────────────────────────────────────────
//   buildInpaintWorkflow()   - SD 標準インペイントワークフロー
//   buildSamWorkflow()       - Segment Anything 2 選択ワークフロー
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

  struct SamRequest {
    QString imageBase64;
    int     pointX         {0};
    int     pointY         {0};
    bool    positivePoint  {true};
    QString samModel       {"sam2_hiera_large.pt"};
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
  /// ワークフロー JSON をキューに追加。非同期。戻り値: promptId
  QString queuePrompt(const QJsonObject& workflow);

  /// 実行履歴を取得（結果画像ファイル名の取り出しに使う）
  void getHistory(const QString& promptId,
                  std::function<void(QJsonObject)> callback);

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

  // ── 組み込みワークフロー ─────────────────────────────────────────────────
  static QJsonObject buildInpaintWorkflow(const InpaintRequest& req);
  static QJsonObject buildSamWorkflow    (const SamRequest&     req);

signals:
  void stateChanged       (State newState);
  void connectionError    (QString message);
  void progressUpdate     (QString promptId, int nodeIndex, int totalNodes, float value);
  void executionComplete  (QString promptId, QStringList outputImages);
  void executionError     (QString promptId, QString message);
  void systemStatsReceived(QJsonObject stats);

private slots:
  void onPollTimer();

private:
  void setState(State s);
  void startPolling(const QString& promptId);
  QNetworkReply* get (const QString& path);
  QNetworkReply* post(const QString& path, const QByteArray& body,
                      const QString& contentType = "application/json");

  QUrl                  m_baseUrl   {"http://localhost:8188"};
  QString               m_clientId  {QUuid::createUuid().toString(QUuid::WithoutBraces)};
  QNetworkAccessManager m_nam       {this};
  State                 m_state     {State::Disconnected};
  QTimer                m_reconnectTimer {this};
  QTimer                m_pollTimer      {this};

  // pending prompts being polled: promptId → poll count
  std::map<QString, int> m_pendingPrompts;
};

} // namespace app::bridge
