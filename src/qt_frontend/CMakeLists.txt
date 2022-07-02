qt_standard_project_setup()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

add_executable(${N64_TARGET}-qt mainwindow.cpp qt_frontend.cpp mainwindow.ui)
target_link_libraries(${N64_TARGET}-qt Qt6::Widgets Qt6::Gui Qt6::Core)
