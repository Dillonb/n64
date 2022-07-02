#include <log.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent) {
    ui = new Ui::MainWindow();
    ui->setupUi(this);

    if (!vkInstance.create()) {
        logfatal("Failed to create vulkan instance! %d", vkInstance.errorCode());
    }

    vkPane = new VulkanPane();
    vkPane->setVulkanInstance(&vkInstance);

    setCentralWidget(QWidget::createWindowContainer(vkPane, this));
}
