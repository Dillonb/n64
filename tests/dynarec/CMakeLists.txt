configure_file(${CMAKE_CURRENT_LIST_DIR}/testcase_path.h.in ${CMAKE_CURRENT_LIST_DIR}/generated/testcase_path.h)

add_executable(test_dynarec test_dynarec.cpp)
TARGET_LINK_LIBRARIES(test_dynarec disassemble r4300i common core jit)
add_test(NAME test_dynarec COMMAND test_dynarec ${CMAKE_CURRENT_LIST_DIR})
