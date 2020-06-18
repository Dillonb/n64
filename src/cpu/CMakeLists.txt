add_library(r4300i
        r4300i.c r4300i.h
        disassemble.c disassemble.h
        mips.c mips.h
        sign_extension.c sign_extension.h)

add_library(rsp
        rsp.c rsp.h rsp_status.h)

find_package(Capstone)

IF(Capstone_FOUND)
    TARGET_LINK_LIBRARIES(r4300i Capstone::Capstone)
    TARGET_COMPILE_DEFINITIONS(r4300i PRIVATE -DHAVE_CAPSTONE)
ENDIF()
