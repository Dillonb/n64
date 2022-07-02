#include <QMainWindow>
#include <QVulkanWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class VulkanPane : public QVulkanWindow {

};

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
    //auto vulkanWidget = QWidget::createWindowContainer();
};
