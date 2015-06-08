#include "mcp.h"
#include <pthread.h>
#include <strings.h>                // bzero
#include <sys/time.h>               // gettimeofday

// wait until barrier satisfied or an absolute time has passed
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
        return 1;
    }
    else
    {
        if (0 != (retval =  pthread_cond_timedwait(&barrier->cond, &(barrier->mutex), abstime))) {
            pthread_cond_broadcast(&barrier->cond);
            pthread_mutex_unlock(&barrier->mutex);
            return retval;
        }
        pthread_mutex_unlock(&barrier->mutex);
        return 0;
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
//  0 or 1 indicate the barrier was satisfied, -1 is a problem or exitFlaglation
int pthread_barrier_waitcancel(pthread_barrier_t *barrier, int *exitFlag)
{
    long int retval = 0;

    logDebug2("about to start wait loop\n");

    do {
        logDebug2("wait loop start\n");
        // check on this once per second
        retval = pthread_barrier_waitseconds(barrier, 5);
        logDebug2("wait over: %ld\n", retval);
        switch (retval) {
            case PTHREAD_BARRIER_NOT_LAST:
            case PTHREAD_BARRIER_LAST:
                break;
                ;;
            case ETIMEDOUT:
                if (*exitFlag) {
                    logDebug2("thread already exitFlagled\n");
                }
                else {
                    *exitFlag=1;
                    logDebug2("pthread barrier timed out. exitFlagling thread.\n");
                }
                return retval;
                ;;
            default:
                logError("unexpected pthread_barrier_wait error: %ld\n", retval);
                *exitFlag = 1;
                return -1;
                ;;
        }
        logDebug2("wait loop end\n");
    } while ( (retval != PTHREAD_BARRIER_NOT_LAST) && (retval != PTHREAD_BARRIER_LAST) );

    // outside of this thread, we dont need to know if this was the last 
    // thread to reach the barrier so we discard PTHREAD_BARRIER_NOT_LAST
    // and PTHREAD_BARRIER_LAST.
    return 0;
}
