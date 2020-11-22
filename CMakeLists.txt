cmake_minimum_required (VERSION 3.10)

message("Detecting system: ${CMAKE_HOST_SYSTEM_NAME}")
if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()

ADD_COMPILE_OPTIONS(-Wall -O3 -mssse3)

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
project (N64)
set(N64_TARGET n64)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
include(CTest)
add_subdirectory(src)
add_subdirectory(tests)