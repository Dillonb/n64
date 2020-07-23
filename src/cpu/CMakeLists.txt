add_library(disassemble
        disassemble.c disassemble.h)

add_library(r4300i
        r4300i.c r4300i.h
        mips_instructions.c mips_instructions.h
        tlb_instructions.c tlb_instructions.h
        rsp_interface.c rsp_interface.h
        mips_instruction_decode.h)

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
