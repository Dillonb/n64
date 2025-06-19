#include "mprotect_utils.h"
#include <log.h>
#include <string.h>

#ifdef N64_MACOS
#include <pthread.h>
#endif

#ifndef N64_WIN
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#else
#include <windows.h>
#include <memoryapi.h>
#endif


void mprotect_error(const char* cache_name) {
#ifdef N64_WIN
    LPVOID lpMsgBuf;
    DWORD error = GetLastError();

    DWORD bufLen = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL );
    LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;

    if (bufLen) {
        logfatal("VirtualProtect %s failed! Code: dec %lu hex %lX Message: %s", cache_name, error, error, lpMsgStr);
    } else {
        logfatal("VirtualProtect %s failed! Code: %lu", cache_name, error);
    }
#else
    logfatal("mprotect %s failed! %s", cache_name, strerror(errno));
#endif
}

void mprotect_rwx(u8* cache, size_t size, const char* cache_name) {
#ifdef N64_WIN
    DWORD oldProtect = 0;
    if (!VirtualProtect(cache, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        mprotect_error(cache_name);
    }
#elif defined(N64_MACOS)
    // no-op
#else
    if (mprotect(cache, size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        mprotect_error(cache_name);
    }
#endif
}

