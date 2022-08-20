#include <QApplication>
#include <settings.h>

#include "mainwindow.h"

int main(int argc, char** argv) {
    n64_settings_init();
    QApplication app(argc, argv);
    MainWindow mw;
    mw.show();
    return app.exec();
}