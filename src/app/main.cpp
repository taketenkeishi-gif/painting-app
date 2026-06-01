#include <QApplication>
#include <QIcon>

#include "app/mainwindow/MainWindow.h"

int main(int argc, char* argv[]) {
  // High DPI scaling
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

  QApplication app(argc, argv);
  app.setWindowIcon(QIcon(":/icons/app_icon.ico"));
  app::mainwindow::MainWindow window;
  window.show();
  return QApplication::exec();
}
