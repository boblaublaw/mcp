#define LOGERROR    0
#define LOGINFO     0
#define LOGDEBUG1   1
#define LOGDEBUG2   2
#define LOGDEBUG3   3

void logInit(FILE *stream);
void logMsg(int level, const char *fmt, ...);
void logIncrementVerbosity(void);

void logError(const char *fmt, ...);
void logFatal(const char *fmt, ...);
void logInfo(const char *fmt, ...);
void logFatal(const char *fmt, ...);
void logDebug(const char *fmt, ...);
void logDebug2(const char *fmt, ...);
void logDebug3(const char *fmt, ...);
