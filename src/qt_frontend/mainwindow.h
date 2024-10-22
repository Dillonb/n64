#ifndef N64_MAIN_WINDOW
#define N64_MAIN_WINDOW

// From parallel-rdp. This must be included before any other vulkan-related headers, or parallel-rdp will complain
#include <vulkan_headers.hpp>

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
    explicit MainWindow(const char* rom_path = nullptr, bool debug = false, bool interpreter = false, QWidget *parent = nullptr);
    virtual ~MainWindow() {};

    void showEvent(QShowEvent* event) override;
    std::unique_ptr<N64EmulatorThread> emulatorThread;

public slots:
    void resetTriggered();
    void openFileTriggered();

private:
    Ui::MainWindow *ui;
    VulkanPane* vkPane;
};


#endif // N64_MAIN_WINDOW