#include "app/debug/DebugServer.h"

#ifdef PAINT_DEBUG_SERVER

#include "app/bridge/AppController.h"

#include <QApplication>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QImage>
#include <QBuffer>
#include <QDebug>
#include <QAbstractSlider>
#include <QFrame>
#include <QPushButton>
#include <QWidget>

// ── DebugServer ──────────────────────────────────────────────────────────────

DebugServer::DebugServer(app::bridge::AppController* controller, int port, QObject* parent)
    : QObject(parent)
    , m_controller(controller)
    , m_port(port)
{
    connect(&m_server, &QTcpServer::newConnection, this, &DebugServer::onNewConnection);
}

bool DebugServer::listen() {
    if (!m_server.listen(QHostAddress::LocalHost, static_cast<quint16>(m_port))) {
        qWarning() << "[DebugServer] Failed to bind port" << m_port << ":" << m_server.errorString();
        return false;
    }
    qDebug() << "[DebugServer] Listening on http://localhost:" + QString::number(m_port);
    return true;
}

// ── Incoming connection ──────────────────────────────────────────────────────

void DebugServer::onNewConnection() {
    while (m_server.hasPendingConnections()) {
        QTcpSocket* socket = m_server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, &DebugServer::onClientReadyRead);
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }
}

void DebugServer::onClientReadyRead() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    const QByteArray raw = socket->readAll();
    handleRequest(socket, raw);
    socket->disconnectFromHost();
}

// ── HTTP request parser ──────────────────────────────────────────────────────

void DebugServer::handleRequest(QTcpSocket* socket, const QByteArray& raw) {
    // First line: "GET /debug/state HTTP/1.1"
    const int firstLineEnd = raw.indexOf('\r');
    const QByteArray firstLine = (firstLineEnd > 0) ? raw.left(firstLineEnd) : raw;
    const QList<QByteArray> parts = firstLine.split(' ');
    if (parts.size() < 2) {
        socket->write(errorJson(400, "Bad request"));
        return;
    }

    const QString method = QString::fromLatin1(parts[0]);
    const QString fullPath = QString::fromLatin1(parts[1]);

    // Split path and query string
    QString path, query;
    const int qpos = fullPath.indexOf('?');
    if (qpos >= 0) {
        path  = fullPath.left(qpos);
        query = fullPath.mid(qpos + 1);
    } else {
        path  = fullPath;
    }

    // Find body (after blank line)
    QByteArray body;
    const int bodyStart = raw.indexOf("\r\n\r\n");
    if (bodyStart >= 0)
        body = raw.mid(bodyStart + 4);

    // CORS header helper — always add for dev tools
    QByteArray response;
    if (method == QLatin1String("GET")) {
        response = routeGet(path, query);
    } else if (method == QLatin1String("POST")) {
        response = routePost(path, body);
    } else {
        response = errorJson(405, "Method not allowed");
    }

    socket->write(response);
}

QByteArray DebugServer::routeGet(const QString& path, const QString& query) {
    if (path == QLatin1String("/debug/health"))       return endpointHealth();
    if (path == QLatin1String("/debug/state"))        return endpointState();
    if (path == QLatin1String("/debug/ai-state"))     return endpointAiState();
    if (path == QLatin1String("/debug/widget-tree"))  return endpointWidgetTree();
    if (path == QLatin1String("/debug/pixels")) {
        int size = 32;
        // Parse ?size=N
        for (const QString& kv : query.split('&')) {
            if (kv.startsWith(QLatin1String("size="))) {
                bool ok;
                const int n = kv.mid(5).toInt(&ok);
                if (ok && n > 0 && n <= 256) size = n;
            }
        }
        return endpointPixels(size);
    }
    if (path == QLatin1String("/debug/canvas-pixels")) {
        int size = 16;
        for (const QString& kv : query.split('&')) {
            if (kv.startsWith(QLatin1String("size="))) {
                bool ok;
                const int n = kv.mid(5).toInt(&ok);
                if (ok && n > 0 && n <= 64) size = n;
            }
        }
        return endpointCanvasPixels(size);
    }
    return errorJson(404, "Not found: " + path);
}

QByteArray DebugServer::routePost(const QString& path, const QByteArray& body) {
    if (path == QLatin1String("/debug/action")) return endpointAction(body);
    return errorJson(404, "Not found: " + path);
}

// ── Endpoint: /debug/health ──────────────────────────────────────────────────

QByteArray DebugServer::endpointHealth() {
    QJsonObject obj;
    obj[QLatin1String("running")]  = true;
    obj[QLatin1String("version")]  = QLatin1String("1.0");
    obj[QLatin1String("appName")]  = QLatin1String("LayeredPaint");
    return okJson(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

// ── Endpoint: /debug/state ──────────────────────────────────────────────────

QByteArray DebugServer::endpointState() {
    const app::bridge::AppController::DebugState s = m_controller->debugState();

    // Selection sub-object
    QJsonObject selection;
    selection[QLatin1String("active")]  = s.selectionWidth > 0 && s.selectionHeight > 0;
    selection[QLatin1String("pixels")]  = s.selectionPixels;
    selection[QLatin1String("op")]      = s.selectionOp;
    selection[QLatin1String("width")]   = s.selectionWidth;
    selection[QLatin1String("height")]  = s.selectionHeight;

    // AiSelect sub-object
    QJsonObject aiSelect;
    aiSelect[QLatin1String("active")]         = s.aiSelectActive;
    aiSelect[QLatin1String("hasPendingMask")] = s.hasPendingMask;
    aiSelect[QLatin1String("previewVisible")] = s.previewVisible;
    aiSelect[QLatin1String("operation")]      = s.selectionOp;

    // Preview sub-object
    QJsonObject preview;
    preview[QLatin1String("visible")] = s.previewVisible;

    // Canvas / document
    QJsonObject canvas;
    canvas[QLatin1String("width")]  = s.canvasWidth;
    canvas[QLatin1String("height")] = s.canvasHeight;

    QJsonObject layers;
    layers[QLatin1String("count")]  = s.layerCount;
    layers[QLatin1String("active")] = s.activeLayerName;
    QJsonArray layerNames;
    for (const QString& n : s.layerNames) layerNames.append(n);
    layers[QLatin1String("names")]  = layerNames;

    QJsonObject aiGen;
    aiGen[QLatin1String("busy")]               = s.aiGenBusy;
    aiGen[QLatin1String("lastError")]          = s.aiGenLastError;
    aiGen[QLatin1String("controllerInstance")] = s.aiControllerInstanceId;

    QJsonObject history;
    history[QLatin1String("undoDepth")] = s.undoDepth;
    history[QLatin1String("canUndo")]   = s.canUndo;

    QJsonObject quickMask;
    quickMask[QLatin1String("active")]              = s.quickMaskMode;
    quickMask[QLatin1String("activeLayerChecksum")] = static_cast<qint64>(s.activeLayerChecksum);
    quickMask[QLatin1String("quickMaskChecksum")]   = static_cast<qint64>(s.quickMaskChecksum);

    QJsonObject root;
    root[QLatin1String("tool")]      = s.tool;
    root[QLatin1String("aiSelect")]  = aiSelect;
    root[QLatin1String("selection")] = selection;
    root[QLatin1String("preview")]   = preview;
    root[QLatin1String("canvas")]    = canvas;
    root[QLatin1String("layers")]    = layers;
    root[QLatin1String("history")]   = history;
    root[QLatin1String("aiGen")]     = aiGen;
    root[QLatin1String("quickMask")] = quickMask;

    return okJson(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

// ── Endpoint: /debug/ai-state ────────────────────────────────────────────────
//
// AI workflow 観測状態を返す。
// workflow-analyze action で更新された detectedNodes と、
// doQueue() で捕捉した lastQueuedWorkflow を含む。

QByteArray DebugServer::endpointAiState() {
    const app::bridge::AppController::DebugState s = m_controller->debugState();

    QJsonObject root;
    root[QLatin1String("workflowPath")]       = s.aiWorkflowPath;
    root[QLatin1String("detectedNodes")]      = s.aiDetectedNodes;
    root[QLatin1String("lastQueuedWorkflow")] = s.aiLastQueuedWorkflow;
    root[QLatin1String("aiGenBusy")]          = s.aiGenBusy;
    root[QLatin1String("aiGenLastError")]     = s.aiGenLastError;
    // POST /prompt キャプチャ（Bad Request 原因特定用）
    root[QLatin1String("comfyHttpStatus")]    = s.aiLastComfyHttpStatus;
    root[QLatin1String("comfyResponseBody")]  = s.aiLastComfyResponseBody;
    // payload は巨大になりうるので先頭 8 KB のみ返す
    const QString payloadStr = QString::fromUtf8(s.aiLastComfyPayload.left(8192));
    root[QLatin1String("comfyPayloadHead")]   = payloadStr;

    return okJson(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

// ── Endpoint: /debug/pixels ──────────────────────────────────────────────────
//
// Renders the selection mask as a grayscale thumbnail scaled to size×size.
// Returns a flat RGBA array (size*size*4 values).
// If no selection, returns all-zero array.

QByteArray DebugServer::endpointPixels(int size) {
    const core::SelectionMask& sel = m_controller->documentSelection();
    const int W = sel.width();
    const int H = sel.height();

    QImage selImage;
    if (W > 0 && H > 0) {
        selImage = QImage(W, H, QImage::Format_Grayscale8);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                selImage.setPixel(x, y, qRgb(sel.maskValue(x, y), sel.maskValue(x, y), sel.maskValue(x, y)));
    } else {
        selImage = QImage(1, 1, QImage::Format_Grayscale8);
        selImage.fill(Qt::black);
    }

    const QImage thumb = selImage.scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                                 .convertToFormat(QImage::Format_RGBA8888);

    // Build flat RGBA number array
    QJsonArray arr;
    const uchar* bits = thumb.constBits();
    const int total = size * size * 4;
    for (int i = 0; i < total; ++i)
        arr.append(static_cast<int>(bits[i]));

    return okJson(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

// ── Endpoint: /debug/canvas-pixels ──────────────────────────────────────────
//
// Returns a sample of composited canvas pixels as a flat RGBA array.
// Used to verify red overlay presence when quickMask mode is active.

QByteArray DebugServer::endpointCanvasPixels(int size) {
    const core::PixelBuffer& buf = m_controller->compositedBuffer();
    const int W = buf.width();
    const int H = buf.height();

    QJsonArray arr;
    if (W <= 0 || H <= 0) {
        return okJson(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    }

    // Sample a grid of size×size pixels evenly spread across the canvas
    for (int gy = 0; gy < size; ++gy) {
        for (int gx = 0; gx < size; ++gx) {
            const int px = (gx * (W - 1)) / (size - 1 > 0 ? size - 1 : 1);
            const int py = (gy * (H - 1)) / (size - 1 > 0 ? size - 1 : 1);
            const core::Color c = buf.pixel(px, py);
            arr.append(c.r);
            arr.append(c.g);
            arr.append(c.b);
            arr.append(c.a);
        }
    }
    return okJson(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

// ── Endpoint: POST /debug/action ─────────────────────────────────────────────

QByteArray DebugServer::endpointAction(const QByteArray& body) {
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return errorJson(400, "Invalid JSON: " + err.errorString());
    }

    const QJsonObject req = doc.object();
    const QString type   = req.value(QLatin1String("type")).toString();
    const QString target = req.value(QLatin1String("target")).toString();

    const app::bridge::AppController::DebugActionResult result =
        m_controller->executeDebugAction(type, target, req);

    QJsonObject resp;
    resp[QLatin1String("success")] = result.success;
    resp[QLatin1String("message")] = result.message;
    if (!result.data.isEmpty())
        resp[QLatin1String("data")] = result.data;
    return okJson(QJsonDocument(resp).toJson(QJsonDocument::Compact));
}

// ── Endpoint: /debug/widget-tree ─────────────────────────────────────────────

static QJsonObject dumpWidget(QWidget* w, int depth = 0) {
    QJsonObject obj;
    obj[QLatin1String("class")]      = QLatin1String(w->metaObject()->className());
    obj[QLatin1String("objectName")] = w->objectName();
    obj[QLatin1String("visible")]    = w->isVisible();
    obj[QLatin1String("geometry")]   = QString("%1,%2 %3x%4")
        .arg(w->x()).arg(w->y()).arg(w->width()).arg(w->height());
    obj[QLatin1String("minimumSize")] = QString("%1x%2")
        .arg(w->minimumWidth()).arg(w->minimumHeight());
    obj[QLatin1String("maximumSize")] = QString("%1x%2")
        .arg(w->maximumWidth()).arg(w->maximumHeight());
    obj[QLatin1String("sizeHint")]    = QString("%1x%2")
        .arg(w->sizeHint().width()).arg(w->sizeHint().height());

    // Inline stylesheet (first 200 chars to keep output manageable)
    const QString ss = w->styleSheet();
    if (!ss.isEmpty())
        obj[QLatin1String("styleSheet")] = ss.left(200);

    // QPushButton: icon size, checkable state
    if (auto* btn = qobject_cast<QPushButton*>(w)) {
        obj[QLatin1String("iconSize")]  = QString("%1x%2")
            .arg(btn->iconSize().width()).arg(btn->iconSize().height());
        obj[QLatin1String("checkable")] = btn->isCheckable();
        obj[QLatin1String("text")]      = btn->text();
        obj[QLatin1String("toolTip")]   = btn->toolTip();
    }
    // QAbstractSlider: range, value, orientation
    if (auto* sl = qobject_cast<QAbstractSlider*>(w)) {
        obj[QLatin1String("sliderMin")]         = sl->minimum();
        obj[QLatin1String("sliderMax")]         = sl->maximum();
        obj[QLatin1String("sliderValue")]       = sl->value();
        obj[QLatin1String("sliderOrientation")] = (sl->orientation() == Qt::Horizontal) ? "H" : "V";
    }
    // QFrame: frame shape (catches separator lines)
    if (auto* fr = qobject_cast<QFrame*>(w)) {
        obj[QLatin1String("frameShape")] = static_cast<int>(fr->frameShape());
    }

    // Property snapshot for debugging tab bars / mdi
    QStringList props;
    if (w->property("dockTabBar").isValid())
        props << QString("dockTabBar=%1").arg(w->property("dockTabBar").toBool() ? "true" : "false");
    if (w->property("fullText").isValid())
        props << QString("fullText=%1").arg(w->property("fullText").toString());
    if (!props.isEmpty())
        obj[QLatin1String("props")] = props.join("; ");

    QJsonArray children;
    if (depth < 6) {  // cap depth to keep output manageable
        for (QObject* child : w->children()) {
            if (auto* cw = qobject_cast<QWidget*>(child))
                children.append(dumpWidget(cw, depth + 1));
        }
    }
    if (!children.isEmpty())
        obj[QLatin1String("children")] = children;

    return obj;
}

QByteArray DebugServer::endpointWidgetTree() {
    QJsonArray roots;
    for (QWidget* w : QApplication::topLevelWidgets())
        if (w->isVisible())
            roots.append(dumpWidget(w));
    return okJson(QJsonDocument(roots).toJson(QJsonDocument::Indented));
}

// ── HTTP response helpers ────────────────────────────────────────────────────

QByteArray DebugServer::okJson(const QByteArray& json) {
    QByteArray response;
    response  = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: application/json\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "Content-Length: " + QByteArray::number(json.size()) + "\r\n";
    response += "\r\n";
    response += json;
    return response;
}

QByteArray DebugServer::errorJson(int code, const QString& message) {
    const QByteArray json = QJsonDocument(QJsonObject{
        {QLatin1String("error"), message}
    }).toJson(QJsonDocument::Compact);

    QByteArray response;
    response  = "HTTP/1.1 " + QByteArray::number(code) + " Error\r\n";
    response += "Content-Type: application/json\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "Content-Length: " + QByteArray::number(json.size()) + "\r\n";
    response += "\r\n";
    response += json;
    return response;
}

#endif // PAINT_DEBUG_SERVER
