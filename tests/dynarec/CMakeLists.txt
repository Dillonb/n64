add_executable(test_dynarec test_dynarec.cpp)
TARGET_LINK_LIBRARIES(test_dynarec disassemble r4300i common core jit)
file(GLOB cases ${CMAKE_CURRENT_LIST_DIR}/*.toml)
foreach(casefile ${cases})
    get_filename_component(casefile_name ${casefile} NAME)
    configure_file(${casefile} ${casefile_name} COPYONLY)
endforeach()

add_test(NAME test_dynarec COMMAND test_dynarec)
