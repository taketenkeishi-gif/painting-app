#include "app/bridge/MinimalWebSocket.h"

#include <QTcpSocket>

namespace app::bridge {

MinimalWebSocket::MinimalWebSocket(QObject* parent) : QObject(parent) {}

MinimalWebSocket::~MinimalWebSocket() = default;

void MinimalWebSocket::open(const QUrl& url) {
  m_url          = url;
  m_rxBuf.clear();
  m_handshakeDone = false;

  if (m_socket != nullptr) {
    m_socket->disconnectFromHost();
    m_socket->deleteLater();
  }
  m_socket = new QTcpSocket(this);
  connect(m_socket, &QTcpSocket::connected,
          this, &MinimalWebSocket::onSocketConnected);
  connect(m_socket, &QTcpSocket::readyRead,
          this, &MinimalWebSocket::onReadyRead);
  connect(m_socket, &QTcpSocket::disconnected,
          this, &MinimalWebSocket::onSocketDisconnected);

  const int port = (url.port() > 0) ? url.port() : 80;
  m_socket->connectToHost(url.host(), static_cast<quint16>(port));
}

void MinimalWebSocket::close() {
  if (m_socket != nullptr) m_socket->disconnectFromHost();
}

void MinimalWebSocket::onSocketConnected() {
  // RFC 6455 opening handshake
  QString path = m_url.path();
  if (path.isEmpty()) path = "/";
  if (m_url.hasQuery()) path += "?" + m_url.query(QUrl::FullyEncoded);

  const int port = (m_url.port() > 0) ? m_url.port() : 80;
  const QByteArray host =
      (m_url.host() + ":" + QString::number(port)).toUtf8();

  // Static base64 key (valid per spec)
  const QByteArray wsKey = "dGhlIHNhbXBsZSBub25jZQ==";

  QByteArray req;
  req += "GET " + path.toUtf8() + " HTTP/1.1\r\n";
  req += "Host: " + host + "\r\n";
  req += "Upgrade: websocket\r\n";
  req += "Connection: Upgrade\r\n";
  req += "Sec-WebSocket-Key: " + wsKey + "\r\n";
  req += "Sec-WebSocket-Version: 13\r\n\r\n";
  m_socket->write(req);
}

void MinimalWebSocket::onReadyRead() {
  m_rxBuf += m_socket->readAll();

  if (!m_handshakeDone) {
    const int hdrEnd = m_rxBuf.indexOf("\r\n\r\n");
    if (hdrEnd < 0) return;

    if (!m_rxBuf.startsWith("HTTP/1.1 101")) {
      m_socket->disconnectFromHost();
      return;
    }
    m_handshakeDone = true;
    m_rxBuf = m_rxBuf.mid(hdrEnd + 4);
    emit connected();
  }

  parseFrames();
}

void MinimalWebSocket::onSocketDisconnected() {
  m_handshakeDone = false;
  emit disconnected();
}

void MinimalWebSocket::parseFrames() {
  while (m_rxBuf.size() >= 2) {
    const quint8 byte0 = static_cast<quint8>(m_rxBuf[0]);
    const quint8 byte1 = static_cast<quint8>(m_rxBuf[1]);
    const quint8 opcode = byte0 & 0x0F;
    const bool   masked  = (byte1 & 0x80) != 0;
    quint64 payloadLen   = byte1 & 0x7F;
    int headerLen = 2;

    if (payloadLen == 126) {
      if (m_rxBuf.size() < 4) return;
      payloadLen =
          (static_cast<quint64>(static_cast<quint8>(m_rxBuf[2])) << 8) |
           static_cast<quint64>(static_cast<quint8>(m_rxBuf[3]));
      headerLen = 4;
    } else if (payloadLen == 127) {
      if (m_rxBuf.size() < 10) return;
      payloadLen = 0;
      for (int i = 0; i < 8; ++i) {
        payloadLen = (payloadLen << 8) |
                      static_cast<quint64>(static_cast<quint8>(m_rxBuf[2 + i]));
      }
      headerLen = 10;
    }

    if (masked) headerLen += 4;

    // payloadLen could be enormous if data is corrupt — sanity cap at 64 MB
    if (payloadLen > 64 * 1024 * 1024) {
      m_rxBuf.clear();
      return;
    }

    const qint64 totalNeeded = static_cast<qint64>(headerLen) +
                                static_cast<qint64>(payloadLen);
    if (m_rxBuf.size() < totalNeeded) return;  // wait for more data

    QByteArray payload = m_rxBuf.mid(headerLen, static_cast<qsizetype>(payloadLen));

    if (masked) {
      const QByteArray maskKey = m_rxBuf.mid(headerLen - 4, 4);
      for (qsizetype i = 0; i < payload.size(); ++i)
        payload[i] = static_cast<char>(
            static_cast<quint8>(payload[i]) ^ static_cast<quint8>(maskKey[i % 4]));
    }

    m_rxBuf = m_rxBuf.mid(static_cast<qsizetype>(totalNeeded));

    switch (opcode) {
      case 0x1:  // text frame
        emit textMessageReceived(QString::fromUtf8(payload));
        break;
      case 0x2:  // binary frame
        emit binaryMessageReceived(payload);
        break;
      case 0x8:  // close
        m_socket->disconnectFromHost();
        break;
      case 0x9:  // ping → pong
        if (m_socket != nullptr && m_socket->isOpen()) {
          const QByteArray pong("\x8a\x00", 2);
          m_socket->write(pong);
        }
        break;
      default:
        break;
    }
  }
}

} // namespace app::bridge
