#include <iostream>
#include <QApplication>
#include "src/gui/main_window.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    QMetaObject::invokeMethod(&w, "onOpenNetwork", Qt::QueuedConnection);
    // Actually this is an interactive GUI test. Let's just mock the UI logic
    return 0;
}
