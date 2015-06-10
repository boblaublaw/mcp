#include "mcp.h"
#include <assert.h>
#include <stdio.h>          // strerror()
#include <string.h>         // strerror(), bzero()
#include <errno.h>          // errno
#include <stdlib.h>         // free()
#include <sys/time.h>       // gettimeofday()

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

void queue_drain(queue_t *q)
{
    assert(q);
    pthread_mutex_lock(&q->mtx);
    q->drain=1;
    pthread_mutex_unlock(&q->mtx);
}

void queue_init(queue_t *q)
{
    assert(q);
    bzero(q, sizeof(queue_t));
    pthread_mutex_init(&q->mtx, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void queue_add(queue_t *q, char *value)
{
    assert(q);
    assert(value);
    elem_t *curr = NULL;

    pthread_mutex_lock(&q->mtx);

    curr=q->head;

    if (curr) {
        while (curr->next) 
            curr=curr->next;
        curr->next=malloc(sizeof(elem_t)); 
        curr=curr->next;
    }
    else {
        curr=malloc(sizeof(elem_t));
        bzero(curr, sizeof(elem_t));
        q->head=curr;
    }
   
    curr->name=strdup(value);
    curr->next=NULL;

    logDebug("PUT: queue size is %d, head is %p\n", q->size, q->head);
    q->size=q->size+1;
    pthread_mutex_unlock(&q->mtx);

    /* Signal waiting threads. */
    pthread_cond_signal(&q->cond);
}

int queue_get(queue_t *q, char **val_r)
{
    assert(q);
    int rc;
    elem_t  *curr;

    pthread_mutex_lock(&q->mtx);

    if (q->size==0 && q->drain) {
        pthread_mutex_unlock(&q->mtx);
        return 0;
    }

    /* Wait for element to become available. */
    while (q->size == 0) {
        rc = pthread_cond_timedwaitseconds(&q->cond, &q->mtx, PTHREAD_POLL_MSEC);
        if (rc == ETIMEDOUT) {
            if (q->drain) {
                pthread_mutex_unlock(&q->mtx);
                return 0;
            }
        }
        if (rc == -1) {
            logError("unexpected return from pthread_cond_timedwaitseconds %d\n", rc);
            pthread_mutex_unlock(&q->mtx);
            return 0;
        }
    }
    logDebug("GET: queue size is %d, head is %p\n", q->size, q->head);
    curr=q->head->next;
    *val_r = q->head->name;
    free(q->head);
    q->head=curr;

    q->size=q->size-1;
    pthread_mutex_unlock(&q->mtx);
    return 1;
}
