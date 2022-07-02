#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <stdlib.h>

extern unsigned int n64_log_verbosity;
extern unsigned int next_n64_log_verbosity;

#ifdef N64_WIN
#define COLOR_RED       ""
#define COLOR_GREEN     ""
#define COLOR_YELLOW    ""
#define COLOR_CYAN      ""
#define COLOR_END       ""
#else
#define COLOR_RED       "\033[0;31m"
#define COLOR_GREEN     "\033[0;32m"
#define COLOR_YELLOW    "\033[0;33m"
#define COLOR_CYAN      "\033[0;36m"
#define COLOR_END       "\033[0;m"
#endif

#define LOG_VERBOSITY_WARN 1
#define LOG_VERBOSITY_INFO 2
#define LOG_VERBOSITY_DEBUG 3
#define LOG_VERBOSITY_TRACE 4

#ifdef N64_DEBUG_MODE
#define LOG_ENABLED
#endif


#define log_set_verbosity(new_verbosity) do {n64_log_verbosity = new_verbosity; next_n64_log_verbosity = new_verbosity;} while(0)
#define delayed_log_set_verbosity(new_verbosity) do {next_n64_log_verbosity = new_verbosity;} while(0)
#define update_delayed_log_verbosity() do {n64_log_verbosity = next_n64_log_verbosity;} while(0)
#define log_get_verbosity() n64_log_verbosity

#define logfatal(message,...) do { \
    fprintf(stderr, COLOR_RED "[FATAL] at %s:%d ", __FILE__, __LINE__);\
    fprintf(stderr, message "\n" COLOR_END, ##__VA_ARGS__);\
    exit(EXIT_FAILURE);} while(0)

#define logdie(message,...) do { \
    fprintf(stderr, COLOR_RED "[FATAL] ");\
    fprintf(stderr, message "\n" COLOR_END, ##__VA_ARGS__);\
    exit(EXIT_FAILURE);} while(0)

#define logwarn(message,...) do { if (n64_log_verbosity >= LOG_VERBOSITY_WARN) {printf(COLOR_YELLOW "[WARN]  " message "\n" COLOR_END, ##__VA_ARGS__);} } while(0)
#define logalways(message,...) do { printf(COLOR_CYAN "[LOG]  " message "\n" COLOR_END, ##__VA_ARGS__); } while(0)
#define unimplemented(condition, message, ...) do { if (condition) { logfatal("UNIMPLEMENTED CASE DETECTED: " message, ##__VA_ARGS__); } } while(0)

#ifdef LOG_ENABLED
#define loginfo(message,...) do { if (n64_log_verbosity >= LOG_VERBOSITY_INFO) {printf(COLOR_CYAN "[INFO]  " message "\n" COLOR_END, ##__VA_ARGS__);} } while(0)
#define loginfo_nonewline(message,...) do { if (n64_log_verbosity >= LOG_VERBOSITY_INFO) {printf(COLOR_CYAN "[INFO]  " message COLOR_END, ##__VA_ARGS__);} } while(0)
#define logdebug(message,...) do { if (n64_log_verbosity >= LOG_VERBOSITY_DEBUG) {printf(COLOR_GREEN "[DEBUG] " message "\n" COLOR_END, ##__VA_ARGS__);} } while(0)
#define logtrace(message,...) do { if (n64_log_verbosity >= LOG_VERBOSITY_TRACE) {printf("[TRACE] " message "\n", ##__VA_ARGS__);} } while(0)


#else

#define loginfo(message,...) do {} while(0)
#define loginfo_nonewline(message,...) do {} while(0)
#define logdebug(message,...) do {} while(0)
#define logtrace(message,...) do {} while(0)

#endif
#endif
