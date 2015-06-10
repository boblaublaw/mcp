#define PTHREAD_BARRIER_NOT_LAST    0
#define PTHREAD_BARRIER_LAST        1

#define PTHREAD_POLL_MSEC           250

int pthread_barrier_waitcancel(pthread_barrier_t *barrier, int *exitFlag, const char *desc);
