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
        parallel-rdp-standalone/vulkan/texture/texture_format.cpp
        parallel-rdp-standalone/vulkan/wsi.cpp
        parallel-rdp-standalone/vulkan/wsi.hpp
        parallel-rdp-standalone/vulkan/indirect_layout.cpp
        parallel-rdp-standalone/vulkan/indirect_layout.hpp
        parallel-rdp-standalone/util/arena_allocator.cpp
        parallel-rdp-standalone/util/arena_allocator.hpp
        parallel-rdp-standalone/util/aligned_alloc.cpp
        parallel-rdp-standalone/util/environment.cpp
        parallel-rdp-standalone/util/timer.cpp
        parallel-rdp-standalone/util/timeline_trace_file.cpp
        parallel-rdp-standalone/util/timeline_trace_file.hpp
        parallel-rdp-standalone/util/thread_name.cpp
        parallel-rdp-standalone/util/thread_name.hpp
        parallel-rdp-standalone/util/logging.cpp
        parallel-rdp-standalone/util/logging.hpp
        parallel-rdp-standalone/util/thread_id.cpp
        parallel-rdp-standalone/util/thread_id.hpp
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

if(WIN32)
    target_compile_definitions(parallel-rdp PUBLIC VK_USE_PLATFORM_WIN32_KHR)
else()
    target_link_libraries(parallel-rdp dl)
endif()
