add_executable(test_dynarec test_dynarec.cpp)
TARGET_LINK_LIBRARIES(test_dynarec disassemble r4300i common core jit)
configure_file(testcases.toml testcases.toml COPYONLY)
