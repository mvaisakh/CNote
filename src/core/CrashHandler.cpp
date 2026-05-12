#include "CrashHandler.h"
#include <cstdio>
#include <cstdlib>
#include <csignal>
#ifndef ANDROID
#include <execinfo.h>
#endif
#include <unistd.h>

namespace Cerium {

static void handle_signal(int sig) {
    void *array[32];
    size_t size;

    fprintf(stderr, "\n--- CERIUMNOTES CRASH DUMP (Signal %d) ---\n", sig);
    
    // Get void*'s for all entries on the stack
#ifndef ANDROID
    size = backtrace(array, 32);

    // Print out all the frames to stderr
    backtrace_symbols_fd(array, size, STDERR_FILENO);
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
