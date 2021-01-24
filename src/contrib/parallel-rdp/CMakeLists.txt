file(GLOB_RECURSE parallel-rdp-cpp parallel-rdp-standalone/parallel-rdp/*.cpp)

add_library(parallel-rdp
        # C++
        ${parallel-rdp-cpp}
        parallel-rdp-standalone/vulkan/buffer.cpp
        parallel-rdp-standalone/vulkan/buffer_pool.cpp
        parallel-rdp-standalone/vulkan/command_buffer.cpp
        parallel-rdp-standalone/vulkan/command_pool.cpp
        parallel-rdp-standalone/vulkan/context.cpp
        parallel-rdp-standalone/vulkan/cookie.cpp
        parallel-rdp-standalone/vulkan/descriptor_set.cpp
        parallel-rdp-standalone/vulkan/device.cpp
        parallel-rdp-standalone/vulkan/event_manager.cpp
        parallel-rdp-standalone/vulkan/fence.cpp
        parallel-rdp-standalone/vulkan/fence_manager.cpp
        parallel-rdp-standalone/vulkan/image.cpp
        parallel-rdp-standalone/vulkan/memory_allocator.cpp
        parallel-rdp-standalone/vulkan/pipeline_event.cpp
        parallel-rdp-standalone/vulkan/query_pool.cpp
        parallel-rdp-standalone/vulkan/render_pass.cpp
        parallel-rdp-standalone/vulkan/sampler.cpp
        parallel-rdp-standalone/vulkan/semaphore.cpp
        parallel-rdp-standalone/vulkan/semaphore_manager.cpp
        parallel-rdp-standalone/vulkan/shader.cpp
        parallel-rdp-standalone/vulkan/texture_format.cpp
        parallel-rdp-standalone/vulkan/thread_id.cpp
        parallel-rdp-standalone/vulkan/wsi.cpp
        parallel-rdp-standalone/vulkan/wsi.hpp
        parallel-rdp-standalone/vulkan/wsi_timing.cpp
        parallel-rdp-standalone/vulkan/wsi_timing.hpp
        parallel-rdp-standalone/util/aligned_alloc.cpp
        parallel-rdp-standalone/util/timer.cpp
        parallel-rdp-standalone/util/timeline_trace_file.cpp
        parallel-rdp-standalone/util/timeline_trace_file.hpp
        parallel-rdp-standalone/util/thread_name.cpp
        parallel-rdp-standalone/util/thread_name.hpp
        parallel-rdp-standalone/spirv-cross/spirv_cross.cpp
        parallel-rdp-standalone/spirv-cross/spirv_cfg.cpp
        parallel-rdp-standalone/spirv-cross/spirv_parser.cpp
        parallel-rdp-standalone/spirv-cross/spirv_cross_parsed_ir.cpp
        # C
        parallel-rdp-standalone/volk/volk.c
)

target_compile_definitions(parallel-rdp PUBLIC GRANITE_VULKAN_MT)

target_include_directories(parallel-rdp PUBLIC
        parallel-rdp-standalone/parallel-rdp
        parallel-rdp-standalone/volk
        parallel-rdp-standalone/spirv-cross
        parallel-rdp-standalone/vulkan
        parallel-rdp-standalone/vulkan-headers/include
        parallel-rdp-standalone/util)

if(NOT WIN32)
target_link_libraries(parallel-rdp dl)
endif()