cmake_minimum_required (VERSION 3.10)

message("Detecting system: ${CMAKE_HOST_SYSTEM_NAME}")
if (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()

set(DEFAULT_C_COMP_OPTIONS -Werror=switch -Werror=format)
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
    find_program(mold_FOUND mold)
    message("mold found: ${mold_FOUND}")
    if (mold_FOUND)
        add_link_options("-fuse-ld=mold")
        message("mold WAS found, using it as the linker.")
    else()
        message("mold was not found, using default linker.")
    endif()
    message("Building on Linux")
endif()

INCLUDE(CheckCCompilerFlag)

# Uncomment me if building on a big endian system (good luck!)
# ADD_COMPILE_DEFINITIONS(N64_BIG_ENDIAN)

# Instant DMAs - useful for debugging issues when timing is not a problem
# ADD_COMPILE_DEFINITIONS(INSTANT_DMA)

# Enable the icache/dcache emulation (interpreter only, not fully accurate yet)
# ADD_COMPILE_DEFINITIONS(ENABLE_DCACHE)
ADD_COMPILE_DEFINITIONS(ENABLE_ICACHE)

project (N64)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(N64_TARGET n64)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
include(CTest)
message("CMAKE_C_COMPILER_ID: ${CMAKE_C_COMPILER_ID}")

find_program(GIT_EXECUTABLE git)

if (GIT_EXECUTABLE)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE _git_hash
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (NOT "${N64_GIT_COMMIT_HASH}" STREQUAL "${_git_hash}")
        file(REMOVE ${CMAKE_CURRENT_LIST_DIR}/src/generated/version.h)
        set(N64_GIT_COMMIT_HASH "${_git_hash}" CACHE STRING "" FORCE)
    endif()
else()
    # Otherwise, ensure N64_GIT_COMMIT_HASH is defined
    if (DEFINED N64_GIT_COMMIT_HASH)
        set(_git_hash "${N64_GIT_COMMIT_HASH}" CACHE STRING "" FORCE)
        file(REMOVE ${CMAKE_CURRENT_LIST_DIR}/src/generated/version.h)
        message("Git not found, but git commit hash was defined as ${N64_GIT_COMMIT_HASH}")
    else()
        message(FATAL_ERROR "Git not found, please define N64_GIT_COMMIT_HASH manually.")
    endif()
endif()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    set (X86 TRUE)
else ()
    set (X86 FALSE)
endif ()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "(aarch64)|(AARCH64)|(arm64)|(ARM64)")
    set (ARM64 TRUE)
    ADD_COMPILE_DEFINITIONS(N64_ARM64)
endif()

check_c_compiler_flag("-mssse3" HAS_SSSE3)
check_c_compiler_flag("-msse4.1" HAS_SSE4_1)

if (HAS_SSSE3 AND HAS_SSE4_1 AND X86)
    set(DEFAULT_C_COMP_OPTIONS ${DEFAULT_C_COMP_OPTIONS} -mssse3 -msse4.1)
    ADD_COMPILE_DEFINITIONS(N64_HAVE_SSE)
elseif(ARM64)
    ADD_COMPILE_DEFINITIONS(N64_HAVE_SSE N64_USE_NEON SSE2NEON_SUPPRESS_WARNINGS)
endif()

add_compile_options("$<$<COMPILE_LANGUAGE:C>:${DEFAULT_C_COMP_OPTIONS}>")
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:${DEFAULT_C_COMP_OPTIONS}>")
add_link_options("$<$<COMPILE_LANGUAGE:C>:${DEFAULT_C_LINK_OPTIONS}>")
add_link_options("$<$<COMPILE_LANGUAGE:CXX>:${DEFAULT_C_LINK_OPTIONS}>")

configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/version.h.in ${CMAKE_CURRENT_LIST_DIR}/src/generated/version.h)

set(N64_DYNAREC_ENABLED true)
set(N64_DYNAREC_V1_ENABLED false) # For the RSP, for now. Only works on x64.
if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
    set(N64_DYNAREC_ARCH "x86_64")
    set(N64_DYNAREC_V1_ENABLED true)
endif()

if (N64_DYNAREC_ENABLED)
    message("Dynarec enabled!")
    add_compile_definitions(N64_DYNAREC_ENABLED)
else()
    message("Dynarec not enabled.")
endif()

if (N64_DYNAREC_V1_ENABLED)
    message("Dynarec V1 enabled!")
    add_compile_definitions(N64_DYNAREC_V1_ENABLED)
else()
    message("Dynarec V1 not enabled.")
endif()

add_subdirectory(src)
add_subdirectory(tests)