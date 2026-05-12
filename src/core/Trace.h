#ifndef CN_TRACE_H
#define CN_TRACE_H

#include <QDebug>
#include <cstdio>

/*
 * CNote Tracing API
 * 
 * Use CN_TRACE() for general debug messages.
 * These are compiled out in Release builds unless CN_ENABLE_TRACING is defined.
 */

#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG) || defined(CN_ENABLE_TRACING)
    #define CN_TRACE(fmt, ...) printf("CNote: " fmt "\n", ##__VA_ARGS__)
#else
    #define CN_TRACE(fmt, ...) do {} while (0)
#endif

#endif // CN_TRACE_H
