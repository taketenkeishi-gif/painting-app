#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QUrl>

QT_FORWARD_DECLARE_CLASS(QTcpSocket)

namespace app::bridge {

// ─────────────────────────────────────────────────────────────────────────────
// MinimalWebSocket
//
// Qt::WebSockets モジュールなしで動作する最小 WebSocket クライアント。
// QTcpSocket + RFC 6455 フレーム解析のみ。
// クライアント→サーバーフレームはマスクなし（ComfyUI は受け付ける）。
// ─────────────────────────────────────────────────────────────────────────────
class MinimalWebSocket : public QObject {
  Q_OBJECT

public:
  explicit MinimalWebSocket(QObject* parent = nullptr);
  ~MinimalWebSocket() override;

  void open(const QUrl& url);
  void close();
  bool isConnected() const noexcept { return m_handshakeDone; }

signals:
  void connected();
  void disconnected();
  void textMessageReceived(QString text);
  void binaryMessageReceived(QByteArray data);

private slots:
  void onSocketConnected();
  void onReadyRead();
  void onSocketDisconnected();

private:
  void parseFrames();

  QUrl        m_url;
  QTcpSocket* m_socket       {nullptr};
  QByteArray  m_rxBuf;
  bool        m_handshakeDone{false};
};

} // namespace app::bridge
