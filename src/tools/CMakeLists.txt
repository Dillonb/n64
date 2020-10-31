add_executable(logtester logtester.c)
target_link_libraries(logtester common core)

add_executable(testcase_gen testcase_gen.c)
target_link_libraries(testcase_gen r4300i common core)
