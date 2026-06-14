#pragma once
#include <QObject>
#include <QUrl>

class QProcess;
class QTimer;

namespace platform::comfy {

// ComfyUI プロセスの起動・ヘルスチェック・設定永続化を担う独立クラス。
// AppController の肥大化を防ぐために分離。
//
// 使い方:
//   manager->ensureRunning(url);
//   connect(manager, &ComfyProcessManager::ready, this, [this](QUrl u){ client->connectToServer(u); });
class ComfyProcessManager : public QObject {
  Q_OBJECT
public:
  explicit ComfyProcessManager(QObject* parent = nullptr);
  ~ComfyProcessManager() override;

  // QSettings "ai/comfyFolderPath" の読み書き
  static QString savedFolderPath();
  static void    saveFolderPath(const QString& path);

  // 未起動なら起動し、ヘルスチェック後に ready() を emit する。
  // 既に起動済み（ポート疎通あり）の場合は即 ready() を emit する。
  // ローカルホスト以外の場合は起動スキップで即 ready()。
  void ensureRunning(const QUrl& serverUrl);

  bool isRunning() const;

Q_SIGNALS:
  void ready(QUrl serverUrl);          ///< サーバーが応答可能になった
  void failed(QString reason);         ///< タイムアウト or パス未設定

private Q_SLOTS:
  void onHealthCheckTick();

private:
  QProcess* m_process    {nullptr};
  QTimer*   m_healthTimer{nullptr};
  int       m_retries    {0};
  QUrl      m_pendingUrl;

  static constexpr int kMaxRetries = 120;  // 60 秒
};

}  // namespace platform::comfy
