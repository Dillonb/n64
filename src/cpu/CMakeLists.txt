add_library(disassemble
        disassemble.c disassemble.h)

configure_file(../contrib/tools/dynasm/dynasm.lua dynasm/dynasm.lua COPYONLY)
configure_file(../contrib/tools/dynasm/dasm_arm.lua dynasm/dasm_arm.lua COPYONLY)
configure_file(../contrib/tools/dynasm/dasm_mips.lua dynasm/dasm_mips.lua COPYONLY)
configure_file(../contrib/tools/dynasm/dasm_ppc.lua dynasm/dasm_ppc.lua COPYONLY)
configure_file(../contrib/tools/dynasm/dasm_x64.lua dynasm/dasm_x64.lua COPYONLY)
configure_file(../contrib/tools/dynasm/dasm_x86.lua dynasm/dasm_x86.lua COPYONLY)

add_executable(minilua ../contrib/tools/minilua.c)
target_link_libraries(minilua m)

add_custom_command(OUTPUT asm_emitter.c
        COMMAND ../../minilua ARGS dynasm/dynasm.lua -D X64 -o asm_emitter.c ${CMAKE_CURRENT_SOURCE_DIR}/dynarec/asm_emitter.dasc
        MAIN_DEPENDENCY dynarec/asm_emitter.dasc
        DEPENDS minilua)

add_library(r4300i
        r4300i.c r4300i.h r4300i_register_access.h
        mips_instructions.c mips_instructions.h
        tlb_instructions.c tlb_instructions.h
        rsp_interface.c rsp_interface.h
        mips_instruction_decode.h
        dynarec/dynarec.c dynarec/dynarec.h
        asm_emitter.c dynarec/asm_emitter.h
        dynarec/dynarec_memory_management.c dynarec/dynarec_memory_management.h)

add_library(rsp
        n64_rsp_bus.c n64_rsp_bus.h
        rsp_types.h rsp_rom.h
        rsp.c rsp.h
        rsp_instructions.c rsp_instructions.h
        rsp_vector_instructions.c rsp_vector_instructions.h
        mips_instruction_decode.h)

TARGET_LINK_LIBRARIES(rsp    disassemble)
TARGET_LINK_LIBRARIES(r4300i disassemble m common)

find_package(Capstone)

IF(Capstone_FOUND)
    TARGET_LINK_LIBRARIES(disassemble debug Capstone::Capstone)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DHAVE_CAPSTONE")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DHAVE_CAPSTONE")
ENDIF()
