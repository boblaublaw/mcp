#include "mcp.h"
#include <sys/time.h>               //gettimeofday


// wait until barrier satisfied or an absolute time has passed
int pthread_barrier_timedwait(pthread_barrier_t *barrier, const struct timespec *restrict abstime)
{
    int rv = 0;

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
        if (0 != (rv =  pthread_cond_timedwait(&barrier->cond, &(barrier->mutex), abstime))) {
            return rv;
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
        gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec + seconds;
        ts.tv_nsec = 0;

        return (pthread_barrier_timedwait(barrier, &ts));
}


// this function will wait for a condition variable or a cancel variable
// to be set.
//  0 or 1 indicate the barrier was satisfied, -1 is a problem or cancellation
int pthread_barrier_waitcancel(pthread_barrier_t *barrier, int *cancel)
{
    long int retval = 0;
    int rewait=0;

    do {
        // currently set to 5 seconds because there is some race condition 
        // here I do not understand.
        retval = pthread_barrier_waitseconds(barrier, 5);
        switch (retval) {
            case PTHREAD_BARRIER_NOT_LAST:
            case PTHREAD_BARRIER_LAST:
                break;
                ;;
            case ETIMEDOUT:
                if (*cancel) {
                    fprintf (stderr, "reader thread cancelled\n");
                    return -1;
                }
                rewait++;
                break;
                ;;
            default:
                fprintf (stderr, "unexpected pthread_barrier_wait error: %ld\n", retval);
                *cancel = 1;
                return -1;
                ;;
        }
    } while ( (retval != PTHREAD_BARRIER_NOT_LAST) && (retval != PTHREAD_BARRIER_LAST) );

    return retval;
}
