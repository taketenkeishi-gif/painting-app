#include "platform/comfy/ComfyProcessManager.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QTcpSocket>
#include <QTimer>

namespace platform::comfy {

namespace {
constexpr char kFolderKey[] = "ai/comfyFolderPath";
}

ComfyProcessManager::ComfyProcessManager(QObject* parent) : QObject(parent) {}

ComfyProcessManager::~ComfyProcessManager() {
  if (m_process && m_process->state() != QProcess::NotRunning) {
    m_process->terminate();
    m_process->waitForFinished(2000);
  }
}

static QSettings makeSettings() {
  return QSettings(QStringLiteral("taketenkeishi"), QStringLiteral("LayeredPaintApp"));
}

QString ComfyProcessManager::savedFolderPath() {
  return makeSettings().value(QLatin1String(kFolderKey)).toString();
}

void ComfyProcessManager::saveFolderPath(const QString& path) {
  makeSettings().setValue(QLatin1String(kFolderKey), path);
}

void ComfyProcessManager::ensureRunning(const QUrl& serverUrl) {
  m_pendingUrl = serverUrl;

  const QString host = serverUrl.host().isEmpty() ? QStringLiteral("localhost") : serverUrl.host();
  const int     port = (serverUrl.port() > 0) ? serverUrl.port() : 8188;

  // リモートホストは自動起動対象外。即 ready を通知して接続に委ねる。
  if (host != QLatin1String("localhost") && host != QLatin1String("127.0.0.1")) {
    QTimer::singleShot(0, this, [this]() { emit ready(m_pendingUrl); });
    return;
  }

  // ポート疎通確認を非同期で実施。
  // waitForConnected は Qt メインスレッド上（HTTPハンドラ等）で不安定なため
  // connectToHost + connected/errorOccurred シグナルを使う。
  const QString folder = savedFolderPath();

  auto* quickCheck = new QTcpSocket(this);

  connect(quickCheck, &QAbstractSocket::connected, this,
          [this, quickCheck]() {
    quickCheck->deleteLater();
    // ポートが既に開いている: 非同期で ready を通知
    QTimer::singleShot(0, this, [this]() { emit ready(m_pendingUrl); });
  });

  connect(quickCheck, &QAbstractSocket::errorOccurred, this,
          [this, quickCheck, folder, host, port](QAbstractSocket::SocketError) {
    quickCheck->deleteLater();

    // フォルダ未設定かつサーバー未起動 → 即 failed
    if (folder.isEmpty()) {
      emit failed(QStringLiteral(
          "ComfyUI フォルダが設定されていません。\n"
          "AI パネル「接続・モデル」→「ComfyUI フォルダ」で設定してください。"));
      return;
    }

    // 既にプロセスが起動試行中なら重複起動しない
    if (m_process && m_process->state() != QProcess::NotRunning) {
      return;
    }

    if (!m_process) {
      m_process = new QProcess(this);
      // finished 時も healthTimer は継続する。
      // 外部 ComfyUI が既に動いている場合、子プロセスはすぐ終了するが
      // onHealthCheckTick が疎通確認して ready を発火する。
    }

    // ComfyUI Portable の embedded Python を優先する
    const QDir portableDir(QDir(folder).filePath(QStringLiteral("..")));
    const QString embeddedPython =
        portableDir.absoluteFilePath(QStringLiteral("python_embeded/python.exe"));

    const QString program = QFileInfo::exists(embeddedPython)
        ? QFileInfo(embeddedPython).absoluteFilePath()
        : QStringLiteral("python");

    m_process->setWorkingDirectory(folder);
    m_process->start(program, QStringList{QStringLiteral("main.py")}, QIODevice::NotOpen);

    if (!m_healthTimer) {
      m_healthTimer = new QTimer(this);
      m_healthTimer->setInterval(500);
      connect(m_healthTimer, &QTimer::timeout, this, &ComfyProcessManager::onHealthCheckTick);
    }
    m_retries = 0;
    m_healthTimer->start();
  });

  quickCheck->connectToHost(host, static_cast<quint16>(port));
}

bool ComfyProcessManager::isRunning() const {
  return m_process && m_process->state() == QProcess::Running;
}

void ComfyProcessManager::onHealthCheckTick() {
  ++m_retries;

  if (m_retries >= kMaxRetries) {
    m_healthTimer->stop();
    emit failed(QStringLiteral("ComfyUI の起動がタイムアウトしました（60 秒）"));
    return;
  }

  const QString host = m_pendingUrl.host().isEmpty()
      ? QStringLiteral("localhost") : m_pendingUrl.host();
  const int port = (m_pendingUrl.port() > 0) ? m_pendingUrl.port() : 8188;

  // 非同期チェック: waitForConnected はメインスレッドで不安定なため使用しない
  auto* sock = new QTcpSocket(this);

  connect(sock, &QAbstractSocket::connected, this, [this, sock]() {
    sock->deleteLater();
    if (m_healthTimer && m_healthTimer->isActive()) {
      m_healthTimer->stop();
      emit ready(m_pendingUrl);
    }
  });

  connect(sock, &QAbstractSocket::errorOccurred, this,
          [sock](QAbstractSocket::SocketError) {
    sock->deleteLater();
    // 次の tick で再試行
  });

  sock->connectToHost(host, static_cast<quint16>(port));
}

}  // namespace platform::comfy
