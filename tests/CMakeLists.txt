add_executable(test_cpu test_cpu.c unit.h)
target_link_libraries(test_cpu common core)
add_test(test_cpu test_cpu)