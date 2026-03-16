#ifndef N64_MAIN_WINDOW
#define N64_MAIN_WINDOW

// From parallel-rdp. This must be included before any other vulkan-related headers, or parallel-rdp will complain
#include <vulkan_headers.hpp>

#include <QMainWindow>
#include <QVulkanWindow>

#include "vulkan_pane.h"

class QMenu;

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
Q_OBJECT

public:
    explicit MainWindow(const char* rom_path = nullptr, bool debug = false, bool interpreter = false, const char* pif_rom_path = nullptr, QWidget *parent = nullptr);
    virtual ~MainWindow() {};

    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    std::unique_ptr<N64EmulatorThread> emulatorThread;

public slots:
    void resetTriggered();
    void openFileTriggered();

private slots:
    void openRecentFile();
    void clearRecentFiles();

private:
    void loadRomFile(const QString& filename);
    void addRecentFile(const QString& filename);
    void updateRecentFilesMenu();

    static constexpr int MaxRecentFiles = 10;

    Ui::MainWindow *ui;
    VulkanPane* vkPane;
    QMenu* recentFilesMenu;
};


#endif // N64_MAIN_WINDOW