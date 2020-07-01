add_executable(test_cpu test_cpu.c unit.h)
target_link_libraries(test_cpu common core)
add_test(test_cpu test_cpu)

configure_file(testcases/addi.testcase addi.testcase COPYONLY)
configure_file(testcases/bgezal.testcase bgezal.testcase COPYONLY)
configure_file(testcases/bltz.testcase bltz.testcase COPYONLY)
configure_file(testcases/nor.testcase nor.testcase COPYONLY)
configure_file(testcases/sltu.testcase sltu.testcase COPYONLY)
configure_file(testcases/addiu.testcase addiu.testcase COPYONLY)
configure_file(testcases/bgezl.testcase bgezl.testcase COPYONLY)
configure_file(testcases/bnel.testcase bnel.testcase COPYONLY)
configure_file(testcases/ori.testcase ori.testcase COPYONLY)
configure_file(testcases/sra.testcase sra.testcase COPYONLY)
configure_file(testcases/add.testcase add.testcase COPYONLY)
configure_file(testcases/bgez.testcase bgez.testcase COPYONLY)
configure_file(testcases/bne.testcase bne.testcase COPYONLY)
configure_file(testcases/or.testcase or.testcase COPYONLY)
configure_file(testcases/srav.testcase srav.testcase COPYONLY)
configure_file(testcases/addu.testcase addu.testcase COPYONLY)
configure_file(testcases/bgtzl.testcase bgtzl.testcase COPYONLY)
configure_file(testcases/div.testcase div.testcase COPYONLY)
configure_file(testcases/sll.testcase sll.testcase COPYONLY)
configure_file(testcases/srl.testcase srl.testcase COPYONLY)
configure_file(testcases/andi.testcase andi.testcase COPYONLY)
configure_file(testcases/bgtz.testcase bgtz.testcase COPYONLY)
configure_file(testcases/divu.testcase divu.testcase COPYONLY)
configure_file(testcases/sllv.testcase sllv.testcase COPYONLY)
configure_file(testcases/srlv.testcase srlv.testcase COPYONLY)
configure_file(testcases/and.testcase and.testcase COPYONLY)
configure_file(testcases/blezl.testcase blezl.testcase COPYONLY)
configure_file(testcases/dsll.testcase dsll.testcase COPYONLY)
configure_file(testcases/slti.testcase slti.testcase COPYONLY)
configure_file(testcases/subu.testcase subu.testcase COPYONLY)
configure_file(testcases/beql.testcase beql.testcase COPYONLY)
configure_file(testcases/blez.testcase blez.testcase COPYONLY)
configure_file(testcases/mult.testcase mult.testcase COPYONLY)
configure_file(testcases/sltiu.testcase sltiu.testcase COPYONLY)
configure_file(testcases/xori.testcase xori.testcase COPYONLY)
configure_file(testcases/beq.testcase beq.testcase COPYONLY)
configure_file(testcases/bltzl.testcase bltzl.testcase COPYONLY)
configure_file(testcases/multu.testcase multu.testcase COPYONLY)
configure_file(testcases/slt.testcase slt.testcase COPYONLY)
configure_file(testcases/xor.testcase xor.testcase COPYONLY)