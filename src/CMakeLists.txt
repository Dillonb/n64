SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
find_package(SDL2 REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM "contrib" ${SDL2_INCLUDE_DIR})

add_subdirectory(common)

add_library(core
        n64system.c n64system.h
        n64rom.c n64rom.h
        n64mem.c n64mem.h
        n64bus.c n64bus.h
        pif_rom.c pif_rom.h)

add_executable(${N64_TARGET} cli_frontend.c)
target_link_libraries(${N64_TARGET} common core)
