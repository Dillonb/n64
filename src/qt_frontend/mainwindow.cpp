#include <QFileDialog>
#include <fstream>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qt_wsi_platform.h"

MainWindow::MainWindow(const char* rom_path, bool debug, bool interpreter, QWidget *parent)
        : QMainWindow(parent) {
    ui = new Ui::MainWindow();
    ui->setupUi(this);

    vkPane = new VulkanPane();
    setCentralWidget(vkPane);
    vkPane->hide();

    emulatorThread = std::make_unique<N64EmulatorThread>(vkPane->qtVkInstanceFactory.get(), vkPane->platform.get(), rom_path, debug, interpreter);
}

void MainWindow::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
}

void MainWindow::resetTriggered() {
    emulatorThread->reset();
}

void MainWindow::openFileTriggered() {
    auto filename = QFileDialog::getOpenFileName(this, "Load ROM", QString(), "N64 ROM files (*.z64 *.n64 *.v64)");
    if (!filename.isEmpty()) {
        vkPane->show();
        emulatorThread->loadRom(filename.toStdString());
        emulatorThread->start();
    }
}
