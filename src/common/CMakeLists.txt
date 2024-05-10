add_library(common
        log.c log.h
        metrics.c metrics.h
        util.h
        fifo.h
        timing.c timing.h
        settings.c settings.h
        perf_map_file.c perf_map_file.h)

target_link_libraries(common inih)
