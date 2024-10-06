#include <QFileDialog>
#include <fstream>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qt_wsi_platform.h"

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent) {
    ui = new Ui::MainWindow();
    ui->setupUi(this);

    vkPane = new VulkanPane();
    setCentralWidget(vkPane);
    vkPane->hide();

    emulatorThread = std::make_unique<N64EmulatorThread>(vkPane->qtVkInstanceFactory.get(), vkPane->platform.get());

    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFileTriggered);
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
        emulatorThread->start();
        emulatorThread->loadRom(filename.toStdString());
    }
}
