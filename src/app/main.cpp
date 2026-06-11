#include <QApplication>
#include <QDebug>
#include <QIcon>

#include "app/mainwindow/MainWindow.h"

int main(int argc, char* argv[]) {
  // High DPI scaling
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

  QApplication app(argc, argv);
  qDebug() << "[STARTUP] Build:" << __DATE__ << __TIME__;
  qDebug() << "[STARTUP] Executable:" << argv[0];
  app.setWindowIcon(QIcon(":/icons/app_icon.ico"));
  app::mainwindow::MainWindow window;
  window.show();
  return QApplication::exec();
}
