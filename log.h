#define LOGERROR    0
#define LOGINFO     1
#define LOGDEBUG1   2
#define LOGDEBUG2   3
#define LOGDEBUG3   3

void logInit(FILE *stream);
void logMsg(int level, const char *fmt, ...);

void logFatal(const char *fmt, ...);
void logDebug(const char *fmt, ...);
void logInfo(const char *fmt, ...);
void logFatal(const char *fmt, ...);
