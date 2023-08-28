if (NOT WIN32)
    add_executable(logtester logtester.c)
    target_link_libraries(logtester common core)

    add_executable(testcase_gen testcase_gen.c)
    target_link_libraries(testcase_gen r4300i common core)

    if (N64_DYNAREC_ENABLED)
        add_executable(dynarec_compare dynarec_compare.c)
        target_link_libraries(dynarec_compare r4300i common core)
    endif()
endif()

#add_executable(rsp_fuzzer rsp_fuzzer.c)
#target_link_libraries(rsp_fuzzer ftd2xx rsp common core)