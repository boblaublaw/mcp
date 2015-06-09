#define PTHREAD_BARRIER_NOT_LAST    0
#define PTHREAD_BARRIER_LAST        1

//int pthread_barrier_waitseconds(pthread_barrier_t *barrier, const int seconds);
//int pthread_barrier_timedwait(pthread_barrier_t *barrier, const struct timespec *restrict abstime);
//int pthread_barrier_waitcancel(pthread_barrier_t *barrier, int *exitFlag);
int pthread_barrier_waitcancel(pthread_barrier_t *barrier, int *exitFlag, const char *meta);
