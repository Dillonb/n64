file(GLOB_RECURSE contrib_headers contrib/*.h)

add_library(rdp
        ${contrib_headers}
        rdp.c rdp.h
        mupen_interface.c mupen_interface.h)

add_library(parallel_rdp_wrapper
        parallel_rdp_wrapper.cpp parallel_rdp_wrapper.h)

target_include_directories(parallel_rdp_wrapper PUBLIC
        ../contrib/parallel-rdp-standalone/parallel-rdp
        ../contrib/parallel-rdp-standalone/volk
        ../contrib/parallel-rdp-standalone/spirv-cross
        ../contrib/parallel-rdp-standalone/vulkan
        ../contrib/parallel-rdp-standalone/vulkan-headers/include
        ../contrib/parallel-rdp-standalone/util)

target_compile_definitions(parallel_rdp_wrapper PUBLIC GRANITE_VULKAN_MT)

target_link_libraries(rdp parallel_rdp_wrapper)

target_link_libraries(rdp dl)