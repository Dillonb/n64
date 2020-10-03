add_library(disassemble
        disassemble.c disassemble.h)

configure_file(../contrib/tools/dynasm/dynasm.lua dynasm/dynasm.lua COPYONLY)
configure_file(../contrib/tools/dynasm/dasm_arm.lua dynasm/dasm_arm.lua COPYONLY)
configure_file(../contrib/tools/dynasm/dasm_mips.lua dynasm/dasm_mips.lua COPYONLY)
configure_file(../contrib/tools/dynasm/dasm_ppc.lua dynasm/dasm_ppc.lua COPYONLY)
configure_file(../contrib/tools/dynasm/dasm_x64.lua dynasm/dasm_x64.lua COPYONLY)
configure_file(../contrib/tools/dynasm/dasm_x86.lua dynasm/dasm_x86.lua COPYONLY)

configure_file(dynarec.dasc dynarec.dasc COPYONLY)

add_executable(minilua ../contrib/tools/minilua.c)
target_link_libraries(minilua m)

add_custom_command(OUTPUT dynarec.c
        COMMAND ../../minilua ARGS dynasm/dynasm.lua -D X64 -o dynarec.c dynarec.dasc
        BYPRODUCTS dynarec.c
        DEPENDS minilua dynarec.dasc
        )

add_library(r4300i
        r4300i.c r4300i.h
        mips_instructions.c mips_instructions.h
        tlb_instructions.c tlb_instructions.h
        rsp_interface.c rsp_interface.h
        mips_instruction_decode.h
        dynarec.c dynarec.h)

add_library(rsp
        rsp_types.h rsp_rom.h
        rsp.c rsp.h
        rsp_instructions.c rsp_instructions.h
        rsp_vector_instructions.c rsp_vector_instructions.h
        mips_instruction_decode.h)

TARGET_LINK_LIBRARIES(rsp    disassemble)
TARGET_LINK_LIBRARIES(r4300i disassemble m)

find_package(Capstone)

IF(Capstone_FOUND)
    TARGET_LINK_LIBRARIES(disassemble Capstone::Capstone)
    TARGET_COMPILE_DEFINITIONS(disassemble PRIVATE -DHAVE_CAPSTONE)
ENDIF()
