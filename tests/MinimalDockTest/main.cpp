#include <QApplication>
#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QTextEdit>

// MinimalDockTest: Verifies that Qt standard QDockWidget drag/detach/redock works.
// Build independently from the main app.
// Expected: title bar visible, draggable, floatable, re-dockable.
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QMainWindow win;
    win.setWindowTitle("MinimalDockTest - drag/detach/redock");
    win.resize(900, 600);

    // Central widget
    auto* central = new QTextEdit(&win);
    central->setPlainText("Central widget.\nDrag the dock panels by their title bars.\nTest: detach, move, re-dock.");
    win.setCentralWidget(central);

    win.setDockNestingEnabled(true);

    // Dock A - left
    auto* dockA = new QDockWidget("Panel A", &win);
    dockA->setObjectName("DockA");
    auto* labelA = new QLabel("Content of Panel A", dockA);
    labelA->setAlignment(Qt::AlignCenter);
    dockA->setWidget(labelA);
    // Standard features: movable + floatable (no custom title bar widget)
    dockA->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    win.addDockWidget(Qt::LeftDockWidgetArea, dockA);

    // Dock B - right
    auto* dockB = new QDockWidget("Panel B", &win);
    dockB->setObjectName("DockB");
    auto* labelB = new QLabel("Content of Panel B", dockB);
    labelB->setAlignment(Qt::AlignCenter);
    dockB->setWidget(labelB);
    dockB->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    win.addDockWidget(Qt::RightDockWidgetArea, dockB);

    // Dock C - tabbed with B
    auto* dockC = new QDockWidget("Panel C", &win);
    dockC->setObjectName("DockC");
    auto* labelC = new QLabel("Content of Panel C\n(tabbed with B)", dockC);
    labelC->setAlignment(Qt::AlignCenter);
    dockC->setWidget(labelC);
    dockC->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    win.addDockWidget(Qt::RightDockWidgetArea, dockC);
    win.tabifyDockWidget(dockB, dockC);

    win.show();
    return app.exec();
}
