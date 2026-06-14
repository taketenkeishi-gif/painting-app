#pragma once
/**
 * DebugServer — minimal HTTP server for Dev_Bridge runtime observation.
 *
 * Active ONLY when the app is launched with --debug-server.
 * Never compiled into release / production builds (guarded by PAINT_DEBUG_SERVER).
 *
 * Implements the Dev_Bridge HTTP Debug Protocol:
 *
 *   GET  /debug/health
 *   GET  /debug/state
 *   GET  /debug/pixels?size=32
 *   POST /debug/action     { type, target, [op] }
 *   GET  /debug/screenshot  (optional)
 *
 * The server runs on the Qt event loop thread.
 * All state reads happen synchronously via AppController::debugState().
 */

#ifdef PAINT_DEBUG_SERVER

#include <QObject>
#include <QTcpServer>
#include <QByteArray>
#include <QString>

namespace app::bridge { class AppController; }
class QTcpSocket;

class DebugServer : public QObject {
    Q_OBJECT

public:
    explicit DebugServer(app::bridge::AppController* controller, int port = 9223, QObject* parent = nullptr);

    /// Start listening. Returns true if the port was bound.
    bool listen();

    int  port() const { return m_port; }

private slots:
    void onNewConnection();
    void onClientReadyRead();

private:
    void     handleRequest(QTcpSocket* socket, const QByteArray& raw);
    QByteArray routeGet(const QString& path, const QString& query);
    QByteArray routePost(const QString& path, const QByteArray& body);

    // Endpoint handlers
    QByteArray endpointHealth();
    QByteArray endpointState();
    QByteArray endpointAiState();
    QByteArray endpointPixels(int size);
    QByteArray endpointCanvasPixels(int size);
    QByteArray endpointAction(const QByteArray& body);
    QByteArray endpointWidgetTree();

    // HTTP helpers
    static QByteArray okJson(const QByteArray& json);
    static QByteArray errorJson(int code, const QString& message);

    app::bridge::AppController* m_controller;
    QTcpServer                  m_server;
    int                         m_port;
};

#endif // PAINT_DEBUG_SERVER
