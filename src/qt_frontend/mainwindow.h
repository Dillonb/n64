#ifndef N64_MAIN_WINDOW
#define N64_MAIN_WINDOW

#include <QMainWindow>
#include <QVulkanWindow>

#include "vulkan_pane.h"

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    virtual ~MainWindow() {};

private:
    Ui::MainWindow *ui;
    VulkanPane* vkPane;
    QVulkanInstance vkInstance;
};


#endif // N64_MAIN_WINDOW