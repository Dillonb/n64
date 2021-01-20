cmake_minimum_required (VERSION 3.10)

message("Detecting system: ${CMAKE_HOST_SYSTEM_NAME}")
if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()

ADD_COMPILE_OPTIONS(-Wall -mssse3 -msse4.1)
#ADD_COMPILE_OPTIONS(-DVULKAN_DEBUG)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DN64_DEBUG_MODE")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DN64_DEBUG_MODE")

if (MACOSX)
    message("Building on MacOS")
    ADD_COMPILE_DEFINITIONS(N64_MACOS)
else()
    message("Building on Linux? TODO: detect windows")
    ADD_LINK_OPTIONS(-export-dynamic) # TODO this works on GCC, need to figure out something for clang on Linux
endif()
# Uncomment me if building on a big endian system (good luck!)
# ADD_COMPILE_DEFINITIONS(N64_BIG_ENDIAN)

#add_compile_options(-fsanitize=undefined)
#add_link_options(-fsanitize=undefined)

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

#add_compile_options(-fsanitize=memory)
#add_link_options(-fsanitize=memory)

#add_compile_options(-fsanitize=thread)
#add_link_options(-fsanitize=thread)

ADD_COMPILE_DEFINITIONS(N64_USE_SIMD)

project (N64)
set(CMAKE_CXX_STANDARD 17)
set(N64_TARGET n64)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
include(CTest)
add_subdirectory(src)
add_subdirectory(tests)