add_library(disassemble
        disassemble.cpp disassemble.h)

add_library(r4300i
        r4300i.c r4300i.h r4300i_register_access.h
        cache.c cache.h
        mips_instructions.c mips_instructions.h
        fpu_instructions.c fpu_instructions.h
        tlb_instructions.c tlb_instructions.h
        rsp_interface.c rsp_interface.h
        mips_instruction_decode.h
)


message("System processor: ${CMAKE_HOST_SYSTEM_PROCESSOR}")

add_library(rsp
        n64_rsp_bus.h
        rsp_types.h rsp_rom.h
        rsp.c rsp.h
        rsp_instructions.c rsp_instructions.h
        rsp_vector_instructions.c rsp_vector_instructions.h
        mips_instruction_decode.h)

TARGET_LINK_LIBRARIES(rsp    disassemble common jit)
TARGET_LINK_LIBRARIES(r4300i disassemble common jit)
if (NOT WIN32)
    TARGET_LINK_LIBRARIES(r4300i m)
endif()

if (N64_DYNAREC_ENABLED)
    target_sources(r4300i PRIVATE
        dynarec/dynarec.c dynarec/dynarec.h
        dynarec/dynarec_memory_management.c dynarec/dynarec_memory_management.h
        dynarec/v2/v2_compiler.c dynarec/v2/v2_compiler.h
    )
    target_sources(rsp PRIVATE
        dynarec/rsp_dynarec.c dynarec/rsp_dynarec.h
    )
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
