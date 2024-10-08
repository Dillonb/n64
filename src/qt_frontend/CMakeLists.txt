set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

set(N64_QT_TARGET ${N64_TARGET}-qt)

add_executable(${N64_QT_TARGET}
        qt_frontend.cpp
        mainwindow.cpp  mainwindow.h mainwindow.ui
        vulkan_pane.cpp vulkan_pane.h
        qt_wsi_platform.cpp qt_wsi_platform.h
        n64_emulator_thread.cpp n64_emulator_thread.h
        ../frontend/tas_movie.c
        ../common/perf_map_file.c)
target_link_libraries(${N64_TARGET}-qt Qt6::Widgets Qt6::Gui Qt6::Core common core parallel_rdp_wrapper parallel-rdp)

set_target_properties(${N64_QT_TARGET}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${N64_EXECUTABLE_OUTPUT_PATH}"
)