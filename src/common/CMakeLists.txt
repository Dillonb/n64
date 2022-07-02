SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

add_library(common
        log.c log.h
        metrics.c metrics.h
        util.h)

