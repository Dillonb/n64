add_library(common
        log.c log.h
        metrics.c metrics.h
        util.h
        fifo.h
        timing.c timing.h
        settings.c settings.h)

target_link_libraries(common inih)
