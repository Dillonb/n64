#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent) {
    ui = new Ui::MainWindow();
    ui->setupUi(this);

    vkPane = new VulkanPane();
    vkPane->setVulkanInstance(&vkInstance);

    QWidget* vkPaneWidget = QWidget::createWindowContainer(vkPane, this);
    setCentralWidget(vkPaneWidget);
}
