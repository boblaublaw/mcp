#include "mcp.h"
#include <strings.h>                // bzero
#include <errno.h>                  // ETIMEDOUT
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
int pthread_barrier_timedwait(
            pthread_barrier_t *barrier, 
            const struct timespec *restrict abstime, 
            const char *desc)
{
    int retval = 0;

    pthread_mutex_lock(&barrier->mutex);

    ++(barrier->count);

    if (barrier->count > barrier->tripCount) {
        logError("WTF. this should never happen.\n");
        retval=-1;
    }
    else if(barrier->count == barrier->tripCount)
    {
        logDebug2("%s this thread is the last thread to arrive\n", desc);
        // start the counter over
        barrier->count = 0;
        // wake up the others
        pthread_cond_broadcast(&barrier->cond);
        retval=PTHREAD_BARRIER_LAST;
    }
    else
    {
        logDebug2("%s not everyone is here yet, let's wait\n", desc);
        retval =  pthread_cond_timedwait(&barrier->cond, &barrier->mutex, abstime);
        if (retval == ETIMEDOUT) {
            logDebug2("%s wait timer exceeded. putting count back and giving up\n", desc);
            // put the count back the way we found it
            --(barrier->count);
        }
        else if (retval == PTHREAD_BARRIER_NOT_LAST) {
            logDebug("%s someone else was last\n", desc);
        }
        else {
            logError("%s unexpected result from pthread_cond_timedwait: %d\n", desc, retval);
        }
    }

    pthread_mutex_unlock(&barrier->mutex);
    return retval;
}

// wait until barrier satisfied or X seconds have passed
int pthread_barrier_waitseconds(pthread_barrier_t *barrier, const int msec, const char *desc)
{
        struct timeval tv;
        struct timespec ts;

        bzero(&tv, sizeof(tv));
        bzero(&ts, sizeof(ts));

        gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec;
        ts.tv_nsec = msec * 1000000;

        return (pthread_barrier_timedwait(barrier, &ts, desc));
}

// this function will wait for a condition variable or a exitFlag variable
// to be set.
// return codes:
//      1  indicates the barrier was satisfied
//      0   indicates the barrier was asked to cancel
//      -1  indicates some other problem
int pthread_barrier_waitcancel(pthread_barrier_t *barrier, int *exitFlag, const char *desc)
{
    long int retval = 0;

    while (1) {
        retval = pthread_barrier_waitseconds(barrier, PTHREAD_POLL_MSEC, desc);
        switch (retval) {
            case PTHREAD_BARRIER_NOT_LAST:
            case PTHREAD_BARRIER_LAST:
                logDebug2("%s thread barrier reached\n", desc);
                return 0;
            case ETIMEDOUT:
                if (*exitFlag) {
                    logDebug2("%s thread being asked to cancel\n", desc);
                    return 0;
                }
                else {
                    logDebug2("%s timeout reached but no need to cancel\n", desc);
                }
                break;
            default:
                logError("%s unexpected pthread_barrier_wait error: %ld\n", desc, retval);
                *exitFlag = 1;
                return -1;
        }
    } 
}

int pthread_cond_timedwaitseconds(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex, int msec)
{
    struct timeval tv;
    struct timespec ts;

    bzero(&tv, sizeof(tv));
    bzero(&ts, sizeof(ts));

    gettimeofday(&tv, NULL);
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = msec * 1000000;

    return(pthread_cond_timedwait(cond, mutex, &ts));
}
