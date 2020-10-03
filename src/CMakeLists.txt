SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
find_package(SDL2 REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM "contrib/include" ${SDL2_INCLUDE_DIR} "common" "." "cpu")

add_subdirectory(common)
add_subdirectory(cpu)
add_subdirectory(rdp)
add_subdirectory(debugger)

add_library(core
        system/n64system.c system/n64system.h
        mem/n64rom.c mem/n64rom.h
        mem/n64mem.c mem/n64mem.h
        mem/n64bus.c mem/n64bus.h
        mem/pif.c mem/pif.h
        mem/dma.c mem/dma.h
        interface/vi.c interface/vi.h interface/vi_reg.h
        frontend/render.c frontend/render.h
        interface/ai.c interface/ai.h
        mem/addresses.h
        mem/n64_rsp_bus.c mem/n64_rsp_bus.h
        mem/mem_util.h)

add_library(glad
        contrib/src/glad.c)

target_link_libraries(glad dl)

target_link_libraries(core r4300i rsp rdp debugger glad ${SDL2_LIBRARY})

add_executable(${N64_TARGET} frontend/cli_frontend.c)
target_link_libraries(${N64_TARGET} common core)

add_executable(logtester logtester.c)
target_link_libraries(logtester common core)