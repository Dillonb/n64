enable_testing()

INCLUDE_DIRECTORIES(SYSTEM "../src/contrib/include" "../src/common" "../src")

if (NOT WIN32)
add_executable(test_cpu test_cpu.c unit.h)
target_link_libraries(test_cpu r4300i common core)
add_test(test_cpu test_cpu)
endif()

add_executable(test_vmadm_overflow test_vmadm_overflow.c unit.h)
target_link_libraries(test_vmadm_overflow rsp common core)
add_test(test_vmadm_overflow test_vmadm_overflow)

find_program(BASS_FOUND bass)
find_program(CHKSUM64_FOUND chksum64)

if (BASS_FOUND)
    # Test roms
    if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/n64-tests)
        if (CHKSUM64_FOUND)
            add_subdirectory(n64-tests/src)
            add_executable(test_rom test_rom.c unit.h)
            target_link_libraries(test_rom r4300i common core)

            file(GLOB asmfiles ${CMAKE_CURRENT_LIST_DIR}/n64-tests/src/*.asm)
            foreach(asmfile ${asmfiles})
                get_filename_component(name ${asmfile} NAME_WLE)

                add_test(NAME test_${name}_recomp COMMAND test_rom ./n64-tests/src/${name}.z64 recomp)
                add_test(NAME test_${name}_interp COMMAND test_rom ./n64-tests/src/${name}.z64 interp)
                add_dependencies(test_rom assemble_test_rom_${name})
            endforeach()
        else()
            message("chksum64 not found, not building/running test ROMs. Install it from here: https://raw.githubusercontent.com/DragonMinded/libdragon/trunk/tools/chksum64.c")
        endif()
    endif()

    # Dynarec tests
    add_executable(test_dynarec_v2 test_dynarec_v2.c unit.h)
    target_link_libraries(test_dynarec_v2 r4300i common core)
    add_test(test_dynarec_v2 test_dynarec_v2)

    file(GLOB asmfiles ${CMAKE_CURRENT_LIST_DIR}/dynarec_v2_tests/*.asm)
    foreach(asmfile ${asmfiles})
        get_filename_component(name ${asmfile} NAME_WLE)
        set(output_bin dynarec_v2_tests/${name}.bin)

        add_custom_command(OUTPUT ${output_bin}
                COMMAND mkdir -p dynarec_v2_tests
                COMMAND bass ${asmfile} -o ${output_bin}
                DEPENDS ${asmfile})

        add_custom_target(assemble_dynarec_v2_test_${name} ALL DEPENDS ${output_bin})

        add_dependencies(test_dynarec_v2 assemble_dynarec_v2_test_${name})
    endforeach()

else()
    message("Bass not found, not building/running test ROMs. Install it from here: https://github.com/ARM9/bass")
endif()

if (NOT WIN32)
add_executable(test_rsp test_rsp.c unit.h)
target_link_libraries(test_rsp rsp r4300i core common)

add_subdirectory(testcases/rsp)

configure_file(testcases/cpu/addi.testcase addi.testcase COPYONLY)
configure_file(testcases/cpu/bgezal.testcase bgezal.testcase COPYONLY)
configure_file(testcases/cpu/bltz.testcase bltz.testcase COPYONLY)
configure_file(testcases/cpu/nor.testcase nor.testcase COPYONLY)
configure_file(testcases/cpu/sltu.testcase sltu.testcase COPYONLY)
configure_file(testcases/cpu/addiu.testcase addiu.testcase COPYONLY)
configure_file(testcases/cpu/bgezl.testcase bgezl.testcase COPYONLY)
configure_file(testcases/cpu/bnel.testcase bnel.testcase COPYONLY)
configure_file(testcases/cpu/ori.testcase ori.testcase COPYONLY)
configure_file(testcases/cpu/sra.testcase sra.testcase COPYONLY)
configure_file(testcases/cpu/add.testcase add.testcase COPYONLY)
configure_file(testcases/cpu/bgez.testcase bgez.testcase COPYONLY)
configure_file(testcases/cpu/bne.testcase bne.testcase COPYONLY)
configure_file(testcases/cpu/or.testcase or.testcase COPYONLY)
configure_file(testcases/cpu/srav.testcase srav.testcase COPYONLY)
configure_file(testcases/cpu/addu.testcase addu.testcase COPYONLY)
configure_file(testcases/cpu/bgtzl.testcase bgtzl.testcase COPYONLY)
configure_file(testcases/cpu/div.testcase div.testcase COPYONLY)
configure_file(testcases/cpu/sll.testcase sll.testcase COPYONLY)
configure_file(testcases/cpu/srl.testcase srl.testcase COPYONLY)
configure_file(testcases/cpu/andi.testcase andi.testcase COPYONLY)
configure_file(testcases/cpu/bgtz.testcase bgtz.testcase COPYONLY)
configure_file(testcases/cpu/divu.testcase divu.testcase COPYONLY)
configure_file(testcases/cpu/sllv.testcase sllv.testcase COPYONLY)
configure_file(testcases/cpu/srlv.testcase srlv.testcase COPYONLY)
configure_file(testcases/cpu/and.testcase and.testcase COPYONLY)
configure_file(testcases/cpu/blezl.testcase blezl.testcase COPYONLY)
configure_file(testcases/cpu/dsll.testcase dsll.testcase COPYONLY)
configure_file(testcases/cpu/slti.testcase slti.testcase COPYONLY)
configure_file(testcases/cpu/subu.testcase subu.testcase COPYONLY)
configure_file(testcases/cpu/beql.testcase beql.testcase COPYONLY)
configure_file(testcases/cpu/blez.testcase blez.testcase COPYONLY)
configure_file(testcases/cpu/mult.testcase mult.testcase COPYONLY)
configure_file(testcases/cpu/sltiu.testcase sltiu.testcase COPYONLY)
configure_file(testcases/cpu/xori.testcase xori.testcase COPYONLY)
configure_file(testcases/cpu/beq.testcase beq.testcase COPYONLY)
configure_file(testcases/cpu/bltzl.testcase bltzl.testcase COPYONLY)
configure_file(testcases/cpu/multu.testcase multu.testcase COPYONLY)
configure_file(testcases/cpu/slt.testcase slt.testcase COPYONLY)
configure_file(testcases/cpu/xor.testcase xor.testcase COPYONLY)
endif()