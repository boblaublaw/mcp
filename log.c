#include "mcp.h"
#include <strings.h>                // bzero()
#include <sys/time.h>               // gettimeofday()
#include <stdarg.h>                 // va_list
#include <stdlib.h>                 // exit()

extern int verbosity;
static FILE *logOutput;

void _logMsg(int level, const char *fmt, va_list argp);

void logInit(FILE *stream)
{
    logOutput=stream;
}

void logError(const char *fmt, ...)
{
    va_list argp;
    va_start(argp,fmt);
    _logMsg(LOGERROR, fmt, argp);
    va_end(argp);
}

void logFatal(const char *fmt, ...)
{
    va_list argp;
    va_start(argp,fmt);
    _logMsg(LOGERROR, fmt, argp);
    va_end(argp);
    exit(EXIT_FAILURE);
}

void logInfo(const char *fmt, ...)
{
    va_list argp;
    va_start(argp,fmt);
    _logMsg(LOGINFO, fmt, argp);
    va_end(argp);
}

void logDebug(const char *fmt, ...) 
{
    va_list argp;
    va_start(argp,fmt);
    _logMsg(LOGDEBUG1, fmt, argp);
    va_end(argp);
}

void logDebug2(const char *fmt, ...)
{
    va_list argp;
    va_start(argp,fmt);
    _logMsg(LOGDEBUG2, fmt, argp);
    va_end(argp);
}

void logDebug3(const char *fmt, ...)
{
    va_list argp;
    va_start(argp,fmt);
    _logMsg(LOGDEBUG3, fmt, argp);
    va_end(argp);
}

void logMsg(int level, const char *fmt, ...)
{
    va_list argp;
    va_start(argp,fmt);
    _logMsg(level, fmt, argp);
    va_end(argp);
    return;
}

void _logMsg(int level, const char *fmt, va_list argp)
{
    if (verbosity >= level) {
        vfprintf(logOutput, fmt, argp);
        fflush(logOutput);
    }
}
