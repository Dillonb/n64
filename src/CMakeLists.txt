SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
find_package(SDL2 REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM "contrib" ${SDL2_INCLUDE_DIR})

add_subdirectory(common)
add_subdirectory(cpu)

add_library(core
        system/n64system.c system/n64system.h
        mem/n64rom.c mem/n64rom.h
        mem/n64mem.c mem/n64mem.h
        mem/n64bus.c mem/n64bus.h
        mem/pif_rom.c mem/pif_rom.h
        mem/dma.c mem/dma.h
        vi.c vi.h vi_reg.h
        render.c render.h interface/ai.c interface/ai.h mem/addresses.h)

target_link_libraries(core r4300i rsp ${SDL2_LIBRARY})

add_executable(${N64_TARGET} cli_frontend.c)
target_link_libraries(${N64_TARGET} common core)

add_executable(logtester logtester.c)
target_link_libraries(logtester common core)