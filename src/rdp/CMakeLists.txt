find_program(glslc_PROGRAM glslc
        HINTS $ENV{VULKAN_SDK}/Bin
        REQUIRED)

add_custom_command(OUTPUT fullscreen_quad.frag.inc
        COMMAND ${glslc_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fullscreen_quad.frag -mfmt=c -o fullscreen_quad.frag.inc
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fullscreen_quad.frag)

add_custom_command(OUTPUT fullscreen_quad.vert.inc
        COMMAND ${glslc_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fullscreen_quad.vert -mfmt=c -o fullscreen_quad.vert.inc
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fullscreen_quad.vert)

file(GLOB_RECURSE contrib_headers contrib/*.h)

add_library(rdp
        ${contrib_headers}
        rdp.c rdp.h
        softrdp.cpp softrdp.h
        mupen_interface.c mupen_interface.h)

add_library(parallel_rdp_wrapper
        parallel_rdp_wrapper.cpp parallel_rdp_wrapper.h
        fullscreen_quad.frag.inc
        fullscreen_quad.vert.inc
        )

target_link_libraries(parallel_rdp_wrapper
        imgui-ui)

target_include_directories(parallel_rdp_wrapper PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ../contrib/parallel-rdp/parallel-rdp-standalone/parallel-rdp
        ../contrib/parallel-rdp/parallel-rdp-standalone/volk
        ../contrib/parallel-rdp/parallel-rdp-standalone/spirv-cross
        ../contrib/parallel-rdp/parallel-rdp-standalone/vulkan
        ../contrib/parallel-rdp/parallel-rdp-standalone/vulkan-headers/include
        ../contrib/parallel-rdp/parallel-rdp-standalone/util)

target_compile_definitions(parallel_rdp_wrapper PUBLIC GRANITE_VULKAN_MT)

target_link_libraries(rdp parallel_rdp_wrapper)

if (NOT WIN32)
    target_link_libraries(rdp dl)
endif()