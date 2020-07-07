file(GLOB_RECURSE contrib_headers contrib/*.h)

add_library(rdp
        ${contrib_headers}
        rdp.c rdp.h
        mupen_interface.c mupen_interface.h)

target_link_libraries(rdp dl)