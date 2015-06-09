#include "mcp.h"
#include <strings.h>                // bzero
#include <sys/time.h>               // gettimeofday

// These functions extend the pthread_barrier_t calls to 
// provide timeout and cancellation functionality.

// wait until barrier satisfied or an absolute time has passed
// returns:
//      PTHREAD_BARRIER_LAST: barrier is satisfied, this
//          thread was the last to reach the barrier.
//      PTHREAD_BARRIER_NOT_LAST: barrier is satisfied, this
//          thread was not the last to reach the barrier.
//      ETIMEDOUT: abstime was reached
//      ...: others?
int pthread_barrier_timedwait(pthread_barrier_t *barrier, const struct timespec *restrict abstime)
{
    int retval = 0;

    pthread_mutex_lock(&barrier->mutex);
    ++(barrier->count);
    if(barrier->count >= barrier->tripCount)
    {
        barrier->count = 0;
        pthread_cond_broadcast(&barrier->cond);
        pthread_mutex_unlock(&barrier->mutex);
        return PTHREAD_BARRIER_LAST;
    }
    else
    {
        if (0 != (retval =  pthread_cond_timedwait(&barrier->cond, &(barrier->mutex), abstime))) {
            pthread_cond_broadcast(&barrier->cond);
            pthread_mutex_unlock(&barrier->mutex);
            return retval;
        }
        pthread_mutex_unlock(&barrier->mutex);
        return PTHREAD_BARRIER_NOT_LAST;
    }
}

// wait until barrier satisfied or X seconds have passed
int pthread_barrier_waitseconds(pthread_barrier_t *barrier, const int seconds)
{
        struct timeval tv;
        struct timespec ts;

        bzero(&tv, sizeof(tv));
        bzero(&ts, sizeof(ts));

        gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec + seconds;
        ts.tv_nsec = 0;

        return (pthread_barrier_timedwait(barrier, &ts));
}


// this function will wait for a condition variable or a exitFlag variable
// to be set.
// return codes:
//      1  indicates the barrier was satisfied
//      0   indicates the barrier was asked to cancel
//      -1  indicates some other problem
int pthread_barrier_waitcancel(pthread_barrier_t *barrier, int *exitFlag)
{
    long int retval = 0;

    do {
        // check on this once per second
        retval = pthread_barrier_waitseconds(barrier, 5);
        logDebug2("wait over: %ld\n", retval);
        switch (retval) {
            case PTHREAD_BARRIER_NOT_LAST:
            case PTHREAD_BARRIER_LAST:
                return 0;
                ;;
            case ETIMEDOUT:
                if (*exitFlag) {
                    logDebug2("thread being asked to cancel\n");
                    return 0;
                }
                else {
                    logDebug2("timeout reached but no need to cancel");
                    retval=0;
                }
                return retval;
                ;;
            default:
                logError("unexpected pthread_barrier_wait error: %ld\n", retval);
                *exitFlag = 1;
                return -1;
                ;;
        }
    } while ( (retval != PTHREAD_BARRIER_NOT_LAST) && (retval != PTHREAD_BARRIER_LAST) );
}
