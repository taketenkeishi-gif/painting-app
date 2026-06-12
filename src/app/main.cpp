#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QIcon>

#include "app/mainwindow/MainWindow.h"

#ifdef PAINT_DEBUG_SERVER
#  include "app/debug/DebugServer.h"
#endif

int main(int argc, char* argv[]) {
  // High DPI scaling
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

  QApplication app(argc, argv);
  qDebug() << "[STARTUP] Build:" << __DATE__ << __TIME__;
  qDebug() << "[STARTUP] Executable:" << argv[0];
  app.setWindowIcon(QIcon(":/icons/app_icon.ico"));

  // ── Command-line parsing ──────────────────────────────────────────────────
  QCommandLineParser parser;
  parser.setApplicationDescription(QLatin1String("LayeredPaint"));
  parser.addHelpOption();

#ifdef PAINT_DEBUG_SERVER
  QCommandLineOption debugServerOpt(
      QLatin1String("debug-server"),
      QLatin1String("Start Dev_Bridge HTTP debug server (port 9223)."));
  parser.addOption(debugServerOpt);

  QCommandLineOption debugPortOpt(
      QLatin1String("debug-port"),
      QLatin1String("Dev_Bridge debug server port (default: 9223)."),
      QLatin1String("port"),
      QLatin1String("9223"));
  parser.addOption(debugPortOpt);
#endif

  parser.process(app);

  // ── Main window ───────────────────────────────────────────────────────────
  app::mainwindow::MainWindow window;
  window.show();

#ifdef PAINT_DEBUG_SERVER
  DebugServer* debugServer = nullptr;
  if (parser.isSet(debugServerOpt)) {
    const int port = parser.value(debugPortOpt).toInt();
    debugServer = new DebugServer(window.controller(), port, &app);
    if (!debugServer->listen()) {
      qWarning() << "[STARTUP] Dev_Bridge debug server failed to start — continuing without it";
    } else {
      qDebug() << "[STARTUP] Dev_Bridge debug server running on port" << port;
    }
  }
#endif

  return QApplication::exec();
}
