add_library(disassemble
        disassemble.cpp disassemble.h)

configure_file(../contrib/tools/dynasm/dynasm.lua dynasm/dynasm.lua COPYONLY)
configure_file(../contrib/tools/dynasm/dasm_arm.lua dynasm/dasm_arm.lua COPYONLY)
configure_file(../contrib/tools/dynasm/dasm_mips.lua dynasm/dasm_mips.lua COPYONLY)
configure_file(../contrib/tools/dynasm/dasm_ppc.lua dynasm/dasm_ppc.lua COPYONLY)
configure_file(../contrib/tools/dynasm/dasm_x64.lua dynasm/dasm_x64.lua COPYONLY)
configure_file(../contrib/tools/dynasm/dasm_x86.lua dynasm/dasm_x86.lua COPYONLY)

add_executable(minilua ../contrib/tools/minilua.c)

if (NOT WIN32)
    target_link_libraries(minilua m)
endif()

if (WIN32)
    set(DYNASM_EXTRA_FLAGS -D WIN)
endif()

add_custom_command(OUTPUT v1_emitter.c
        COMMAND $<TARGET_FILE:minilua> ARGS dynasm/dynasm.lua -D X64 ${DYNASM_EXTRA_FLAGS} -o v1_emitter.c ${CMAKE_CURRENT_SOURCE_DIR}/dynarec/v1/v1_emitter.dasc
        MAIN_DEPENDENCY dynarec/v1/v1_emitter.dasc
        DEPENDS minilua)

add_custom_command(OUTPUT v2_emitter.c
        COMMAND $<TARGET_FILE:minilua> ARGS dynasm/dynasm.lua -D X64 ${DYNASM_EXTRA_FLAGS} -o v2_emitter.c ${CMAKE_CURRENT_SOURCE_DIR}/dynarec/v2/v2_emitter.dasc
        MAIN_DEPENDENCY dynarec/v2/v2_emitter.dasc
        DEPENDS minilua)

add_library(r4300i
        r4300i.c r4300i.h r4300i_register_access.h
        mips_instructions.c mips_instructions.h
        fpu_instructions.c fpu_instructions.h
        tlb_instructions.c tlb_instructions.h
        rsp_interface.c rsp_interface.h
        mips_instruction_decode.h

        dynarec/dynasm_impl.c

        dynarec/dynarec.c dynarec/dynarec.h
        dynarec/dynarec_memory_management.c dynarec/dynarec_memory_management.h

        dynarec/v1/v1_compiler.c dynarec/v1/v1_compiler.h
        v1_emitter.c dynarec/v1/v1_emitter.h

        v2_emitter.c dynarec/v2/v2_emitter.h
        dynarec/v2/v2_compiler.c dynarec/v2/v2_compiler.h
        dynarec/v2/ir_context.c dynarec/v2/ir_context.h
        dynarec/v2/ir_emitter.c dynarec/v2/ir_emitter.h
        dynarec/v2/ir_emitter_fpu.c dynarec/v2/ir_emitter_fpu.h
        dynarec/v2/ir_optimizer.c dynarec/v2/ir_optimizer.h
        dynarec/v2/target_platform.h
        dynarec/v2/register_allocator.c dynarec/v2/register_allocator.h)

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
    target_sources(r4300i PRIVATE dynarec/v2/x86_64_target_platform.c)
else()
    message(FATAL_ERROR "Unsupported target CPU architecture ${CMAKE_SYSTEM_PROCESSOR}")
endif()

message("System processor: ${CMAKE_HOST_SYSTEM_PROCESSOR}")

add_library(rsp
        n64_rsp_bus.h
        rsp_types.h rsp_rom.h
        rsp.c rsp.h
        rsp_instructions.c rsp_instructions.h
        rsp_vector_instructions.c rsp_vector_instructions.h
        dynarec/rsp_dynarec.c dynarec/rsp_dynarec.h
        mips_instruction_decode.h)

TARGET_LINK_LIBRARIES(rsp    disassemble)
TARGET_LINK_LIBRARIES(r4300i disassemble common)
if (NOT WIN32)
    TARGET_LINK_LIBRARIES(r4300i m)
endif()

find_package(Capstone)

IF(Capstone_FOUND)
    if (Capstone_LIBRARIES MATCHES ".*\\.a$")
        message("Statically linking Capstone")
    else()
        message("Dynamically linking Capstone")
    endif()
    TARGET_LINK_LIBRARIES(disassemble ${Capstone_LIBRARIES})
    target_include_directories(disassemble PRIVATE ${Capstone_INCLUDE_DIRS})
    TARGET_COMPILE_DEFINITIONS(disassemble PRIVATE -DHAVE_CAPSTONE)
else()
    message("Capstone not found, disassembly will not be supported in the debugger")
ENDIF()
