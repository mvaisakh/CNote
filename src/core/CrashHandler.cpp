#include "CrashHandler.h"
#include <cstdio>
#include <cstdlib>
#include <csignal>
#if !defined(ANDROID) && !defined(_WIN32)
#include <execinfo.h>
#include <unistd.h>
#endif

namespace Cerium {

static void handle_signal(int sig) {
    void *array[32];
    size_t size;

    fprintf(stderr, "\n--- CERIUMNOTES CRASH DUMP (Signal %d) ---\n", sig);
    
    // Get void*'s for all entries on the stack
#if !defined(ANDROID) && !defined(_WIN32)
    size = backtrace(array, 32);

    // Print out all the frames to stderr
    backtrace_symbols_fd(array, size, 2); // 2 is stderr
#else
    fprintf(stderr, "Backtrace not available on this platform\n");
#endif
    
    fprintf(stderr, "------------------------------------\n");
    
    // Call default handler or exit
    exit(1);
}

void setupCrashHandler() {
    signal(SIGSEGV, handle_signal);
    signal(SIGABRT, handle_signal);
    signal(SIGFPE, handle_signal);
    signal(SIGILL, handle_signal);
    printf("CeriumNotes: Crash handler initialized\n");
}

} // namespace Cerium
