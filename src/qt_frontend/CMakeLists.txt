qt_standard_project_setup()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

add_executable(${N64_TARGET}-qt
        qt_frontend.cpp
        mainwindow.cpp  mainwindow.h mainwindow.ui
        vulkan_pane.cpp vulkan_pane.h
        qt_wsi_platform.cpp qt_wsi_platform.h
        n64_emulator_thread.cpp n64_emulator_thread.h)
target_link_libraries(${N64_TARGET}-qt Qt6::Widgets Qt6::Gui Qt6::Core common core parallel_rdp_wrapper parallel-rdp)