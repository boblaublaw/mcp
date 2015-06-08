#include "mcp.h"
#include <pthread.h>
#include <strings.h>                // bzero
#include <sys/time.h>               // gettimeofday

extern int verbosity;

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


// this function will wait for a condition variable or a cancel variable
// to be set.
//  0 or 1 indicate the barrier was satisfied, -1 is a problem or cancellation
int pthread_barrier_waitcancel(pthread_barrier_t *barrier, int *cancel)
{
    long int retval = 0;

    if (verbosity > 2) {
        fprintf (stderr, "about to start wait loop\n");
        fflush(stderr);
    }

    do {
        if (verbosity > 2) {
            fprintf (stderr, "wait loop start\n");
            fflush(stderr);
        }
        // check on this once per second
        retval = pthread_barrier_waitseconds(barrier, 5);
        if (verbosity > 2) {
            fprintf (stderr, "wait over: %ld\n", retval);
            fflush(stderr);
        }
        switch (retval) {
            case PTHREAD_BARRIER_NOT_LAST:
            case PTHREAD_BARRIER_LAST:
                break;
                ;;
            case ETIMEDOUT:
                if (*cancel) {
                    if (verbosity > 2) {
                        fprintf (stderr, "thread already cancelled\n");
                        fflush(stderr);
                    }
                }
                else {
                    *cancel=1;
                    if (verbosity > 2) {
                        fprintf (stderr, "pthread barrier timed out. cancelling thread.\n");
                        fflush(stderr);
                    }
                }
                return retval;
                ;;
            default:
                fprintf (stderr, "unexpected pthread_barrier_wait error: %ld\n", retval);
                *cancel = 1;
                return -1;
                ;;
        }
        if (verbosity > 2) {
            fprintf (stderr, "wait loop end\n");
            fflush(stderr);
        }
    } while ( (retval != PTHREAD_BARRIER_NOT_LAST) && (retval != PTHREAD_BARRIER_LAST) );

    return retval;
}
