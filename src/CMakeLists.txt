SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
find_package(SDL2 REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM "contrib/include" "contrib/imgui" "contrib/cic_nus_6105" ${SDL2_INCLUDE_DIR} "common" "." "cpu")

add_subdirectory(common)
add_subdirectory(cpu)
add_subdirectory(rdp)
add_subdirectory(debugger)
add_subdirectory(contrib/parallel-rdp)
add_subdirectory(contrib/imgui)
add_subdirectory(contrib/cic_nus_6105)
set(NFD_PORTAL true)
add_subdirectory(contrib/nativefiledialog-extended)
add_subdirectory(tools)

find_package(Qt6 COMPONENTS Core Gui Widgets)
if (Qt6_FOUND)
    message("Qt6 found, building the Qt frontend")
    add_subdirectory(qt_frontend)
else()
    message("Qt6 NOT FOUND, NOT building the Qt frontend!")
endif()

add_library(core
        system/n64system.c system/n64system.h
        system/scheduler.c system/scheduler.h

        mem/mem_util.h
        mem/addresses.h
        mem/n64rom.c mem/n64rom.h
        mem/n64mem.c mem/n64mem.h
        mem/n64bus.c mem/n64bus.h
        mem/pif.c mem/pif.h
        mem/backup.c mem/backup.h

        interface/vi.c interface/vi.h interface/vi_reg.h
        interface/si.c interface/si.h
        interface/pi.c interface/pi.h
        interface/ai.c interface/ai.h

        frontend/render.c frontend/render.h frontend/render_internal.h
        frontend/frontend.c frontend/frontend.h
        frontend/device.c frontend/device.h
        frontend/tas_movie.c frontend/tas_movie.h
        frontend/audio.c frontend/audio.h
        frontend/gamepad.c frontend/gamepad.h
        frontend/game_db.c frontend/game_db.h)

target_link_libraries(core r4300i rsp rdp parallel-rdp debugger ${SDL2_LIBRARY} cic_nus_6105)

add_library(imgui-ui
        imgui/imgui_ui.cpp imgui/imgui_ui.h)

target_include_directories(imgui-ui PUBLIC
        contrib/parallel-rdp/parallel-rdp-standalone/volk
        contrib/parallel-rdp/parallel-rdp-standalone/vulkan
        contrib/parallel-rdp/parallel-rdp-standalone/util
        contrib/nativefiledialog-extended/src/include
        )

target_link_libraries(imgui-ui imgui nfd)

add_executable(${N64_TARGET} frontend/cli_frontend.c)
target_link_libraries(${N64_TARGET} common core imgui-ui)
