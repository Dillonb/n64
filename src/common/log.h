#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <stdlib.h>

extern unsigned int n64_log_verbosity;

#define COLOR_RED       "\033[0;31m"
#define COLOR_GREEN     "\033[0;32m"
#define COLOR_YELLOW    "\033[0;33m"
#define COLOR_CYAN      "\033[0;36m"
#define COLOR_END       "\033[0;m"

#define LOG_VERBOSITY_WARN 1
#define LOG_VERBOSITY_INFO 2
#define LOG_VERBOSITY_DEBUG 3
#define LOG_VERBOSITY_TRACE 4


#define log_set_verbosity(new_verbosity) do {n64_log_verbosity = new_verbosity;} while(0);

#define logfatal(message,...) if (1) { \
    fprintf(stderr, COLOR_RED "[FATAL] at %s:%d ", __FILE__, __LINE__);\
    fprintf(stderr, message "\n" COLOR_END, ##__VA_ARGS__);\
    exit(EXIT_FAILURE);}

#define logdie(message,...) if (1) { \
    fprintf(stderr, COLOR_RED "[FATAL] ");\
    fprintf(stderr, message "\n" COLOR_END, ##__VA_ARGS__);\
    exit(EXIT_FAILURE);}

#define logwarn(message,...) if (n64_log_verbosity >= LOG_VERBOSITY_WARN) {printf(COLOR_YELLOW "[WARN]  " message "\n" COLOR_END, ##__VA_ARGS__);}
#define loginfo(message,...) if (n64_log_verbosity >= LOG_VERBOSITY_INFO) {printf(COLOR_CYAN "[INFO]  " message "\n" COLOR_END, ##__VA_ARGS__);}
#define loginfo_nonewline(message,...) if (n64_log_verbosity >= LOG_VERBOSITY_INFO) {printf(COLOR_CYAN "[INFO]  " message COLOR_END, ##__VA_ARGS__);}
#define logdebug(message,...) if (n64_log_verbosity >= LOG_VERBOSITY_DEBUG) {printf(COLOR_GREEN "[DEBUG] " message "\n" COLOR_END, ##__VA_ARGS__);}
#define logtrace(message,...) if (n64_log_verbosity >= LOG_VERBOSITY_TRACE) {printf("[TRACE] " message "\n", ##__VA_ARGS__);}

#define unimplemented(condition, message) if (condition) { logfatal("UNIMPLEMENTED CASE DETECTED: %s", message) }
#endif
