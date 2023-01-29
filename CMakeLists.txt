cmake_minimum_required (VERSION 3.10)

message("Detecting system: ${CMAKE_HOST_SYSTEM_NAME}")
if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()

ADD_COMPILE_OPTIONS(-mssse3 -msse4.1 -Werror=switch)
#ADD_COMPILE_OPTIONS(-DVULKAN_DEBUG)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DN64_DEBUG_MODE -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DN64_DEBUG_MODE -g")

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

#add_compile_options(-fsanitize=undefined)
#add_link_options(-fsanitize=undefined)

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

#add_compile_options(-fsanitize=thread)
#add_link_options(-fsanitize=thread)

ADD_COMPILE_DEFINITIONS(N64_USE_SIMD)

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