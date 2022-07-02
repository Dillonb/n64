qt_standard_project_setup()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

add_executable(${N64_TARGET}-qt
        qt_frontend.cpp
        mainwindow.cpp  mainwindow.h mainwindow.ui
        vulkan_renderer.cpp vulkan_renderer.h
        vulkan_pane.cpp vulkan_pane.h)
target_link_libraries(${N64_TARGET}-qt Qt6::Widgets Qt6::Gui Qt6::Core)
