#include <log.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent) {
    ui = new Ui::MainWindow();
    ui->setupUi(this);

    if (volkInitialize() != VK_SUCCESS) {
        logfatal("Failed to load Volk");
    }

    if (!vkInstance.create()) {
        logfatal("Failed to create vulkan instance! %d", vkInstance.errorCode());
    }

    vkPane = new VulkanPane(&vkInstance);

    setCentralWidget(QWidget::createWindowContainer(vkPane, this));
}
