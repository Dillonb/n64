SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
find_package(SDL2 REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM "contrib/include" "contrib/imgui" ${SDL2_INCLUDE_DIR} "common" "." "cpu")

add_subdirectory(common)
add_subdirectory(cpu)
add_subdirectory(rdp)
add_subdirectory(debugger)
add_subdirectory(contrib/parallel-rdp)
add_subdirectory(contrib/imgui)
add_subdirectory(tools)

add_library(core
        mem/mem_util.h
        system/n64system.c system/n64system.h
        mem/n64rom.c mem/n64rom.h
        mem/n64mem.c mem/n64mem.h
        mem/n64bus.c mem/n64bus.h
        mem/pif.c mem/pif.h
        interface/vi.c interface/vi.h interface/vi_reg.h
        frontend/render.c frontend/render.h frontend/render_internal.h
        interface/ai.c interface/ai.h
        mem/addresses.h
        frontend/tas_movie.c frontend/tas_movie.h
        frontend/audio.c frontend/audio.h
        frontend/gamepad.c frontend/gamepad.h
        mem/backup.c mem/backup.h
        frontend/game_db.c frontend/game_db.h)

target_link_libraries(core r4300i rsp rdp parallel-rdp debugger ${SDL2_LIBRARY} z)

add_library(imgui-ui
        imgui/imgui_ui.cpp imgui/imgui_ui.h)

target_include_directories(imgui-ui PUBLIC
        contrib/parallel-rdp/parallel-rdp-standalone/volk
        contrib/parallel-rdp/parallel-rdp-standalone/vulkan
        contrib/parallel-rdp/parallel-rdp-standalone/util
        )

target_link_libraries(imgui-ui imgui)

add_executable(${N64_TARGET} frontend/cli_frontend.c)
target_link_libraries(${N64_TARGET} common core imgui-ui)
