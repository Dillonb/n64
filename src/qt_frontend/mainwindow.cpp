#include <QFileDialog>
#include <QKeyEvent>
#include <fstream>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qt_wsi_platform.h"

#include <SDL_events.h>
#include <SDL_keyboard.h>

static SDL_Keycode qtKeyToSdlKeycode(int qtKey) {
    // ASCII letters: Qt uses uppercase, SDL uses lowercase
    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z) {
        return SDLK_a + (qtKey - Qt::Key_A);
    }
    // Digits and basic ASCII punctuation match directly
    if (qtKey >= Qt::Key_Space && qtKey <= Qt::Key_At) {
        return qtKey;
    }
    // Brackets and backslash range
    if (qtKey >= Qt::Key_BracketLeft && qtKey <= Qt::Key_AsciiTilde) {
        return qtKey;
    }
    switch (qtKey) {
        case Qt::Key_Escape:    return SDLK_ESCAPE;
        case Qt::Key_Tab:       return SDLK_TAB;
        case Qt::Key_Backspace: return SDLK_BACKSPACE;
        case Qt::Key_Return:    return SDLK_RETURN;
        case Qt::Key_Enter:     return SDLK_KP_ENTER;
        case Qt::Key_Delete:    return SDLK_DELETE;
        case Qt::Key_Up:        return SDLK_UP;
        case Qt::Key_Down:      return SDLK_DOWN;
        case Qt::Key_Left:      return SDLK_LEFT;
        case Qt::Key_Right:     return SDLK_RIGHT;
        case Qt::Key_Shift:     return SDLK_LSHIFT;
        case Qt::Key_Control:   return SDLK_LCTRL;
        case Qt::Key_Alt:       return SDLK_LALT;
        default:                return SDLK_UNKNOWN;
    }
}

static void pushSdlKeyEvent(int qtKey, bool pressed) {
    // Don't forward Escape to SDL. Quit is handled by Qt, not the CLI's ESC handler.
    if (qtKey == Qt::Key_Escape) {
        return;
    }

    SDL_Keycode sdlKey = qtKeyToSdlKeycode(qtKey);
    if (sdlKey == SDLK_UNKNOWN) {
        return;
    }

    SDL_Event event = {};
    event.type = pressed ? SDL_KEYDOWN : SDL_KEYUP;
    event.key.state = pressed ? SDL_PRESSED : SDL_RELEASED;
    event.key.keysym.sym = sdlKey;
    event.key.keysym.scancode = SDL_GetScancodeFromKey(sdlKey);
    SDL_PushEvent(&event);
}

MainWindow::MainWindow(const char* rom_path, bool debug, bool interpreter, const char* pif_rom_path, QWidget *parent)
        : QMainWindow(parent) {
    ui = new Ui::MainWindow();
    ui->setupUi(this);

    vkPane = new VulkanPane();
    setCentralWidget(vkPane);
    vkPane->hide();

    emulatorThread = std::make_unique<N64EmulatorThread>(vkPane->qtVkInstanceFactory.get(), vkPane->platform.get(), rom_path, debug, interpreter, pif_rom_path);
}

void MainWindow::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (!event->isAutoRepeat()) {
        pushSdlKeyEvent(event->key(), true);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    if (!event->isAutoRepeat()) {
        pushSdlKeyEvent(event->key(), false);
    }
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
