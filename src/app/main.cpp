#include <QApplication>

#include "app/mainwindow/MainWindow.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app::mainwindow::MainWindow window;
  window.show();
  return QApplication::exec();
}
