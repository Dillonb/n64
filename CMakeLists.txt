cmake_minimum_required (VERSION 3.10)

message("Detecting system: ${CMAKE_HOST_SYSTEM_NAME}")
if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()

set(DEFAULT_C_COMP_OPTIONS -mssse3 -msse4.1 -Werror=switch)
set(DEFAULT_C_LINK_OPTIONS "")

# Asan
#set(DEFAULT_C_COMP_OPTIONS ${DEFAULT_C_COMP_OPTIONS} -fsanitize=address)
#set(DEFAULT_C_LINK_OPTIONS ${DEFAULT_C_LINK_OPTIONS} -fsanitize=address)

# Ubsan
#set(DEFAULT_C_COMP_OPTIONS ${DEFAULT_C_COMP_OPTIONS} -fsanitize=undefined)
#set(DEFAULT_C_LINK_OPTIONS ${DEFAULT_C_LINK_OPTIONS} -fsanitize=undefined)

# Thread sanitizer
#set(DEFAULT_C_COMP_OPTIONS ${DEFAULT_C_COMP_OPTIONS} -fsanitize=thread)
#set(DEFAULT_C_LINK_OPTIONS ${DEFAULT_C_LINK_OPTIONS} -fsanitize=thread)

add_compile_options("$<$<COMPILE_LANGUAGE:C>:${DEFAULT_C_COMP_OPTIONS}>")
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:${DEFAULT_C_COMP_OPTIONS}>")
add_link_options("$<$<COMPILE_LANGUAGE:C>:${DEFAULT_C_LINK_OPTIONS}>")
add_link_options("$<$<COMPILE_LANGUAGE:CXX>:${DEFAULT_C_LINK_OPTIONS}>")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DN64_DEBUG_MODE")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DN64_DEBUG_MODE")

if (MACOSX)
    message("Building on MacOS")
    ADD_COMPILE_DEFINITIONS(N64_MACOS)
elseif(WIN32)
    message("Building on Windows")
    ADD_COMPILE_DEFINITIONS(N64_WIN NOMINMAX _CRT_SECURE_NO_WARNINGS)

    ADD_COMPILE_OPTIONS(/EHa)

else()
    message("Building on Linux")
endif()
# Uncomment me if building on a big endian system (good luck!)
# ADD_COMPILE_DEFINITIONS(N64_BIG_ENDIAN)

ADD_COMPILE_DEFINITIONS(N64_USE_SIMD)
ADD_COMPILE_DEFINITIONS(INSTANT_DMA)

project (N64)
set(CMAKE_CXX_STANDARD 17)
set(N64_TARGET n64)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
include(CTest)
message("CMAKE_C_COMPILER_ID: ${CMAKE_C_COMPILER_ID}")

if (CMAKE_C_COMPILER_ID MATCHES "Clang")
    #add_compile_options(-fsanitize=memory -fsanitize-recover=memory)
    #add_link_options(-fsanitize=memory)
endif()

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    ADD_LINK_OPTIONS(-export-dynamic) # Allows the angrylion shared library to be loaded
endif()
add_subdirectory(src)
add_subdirectory(tests)