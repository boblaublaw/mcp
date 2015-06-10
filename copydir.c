#include "mcp.h"
#include <assert.h>

typedef struct queue 
{
    pthread_mutex_t mtx;
    pthread_cond_t  cond;
    int             size;
} queue_t;

typedef struct dir_mgr_t 
{
    int         argc;
    char        **argv;
    queue_t     q;
} dir_mgr_t;

void queue_init(queue_t *q)
{
    pthread_mutex_init(&q->mtx, NULL);
    pthread_cond_init(&q->cond, NULL);
    q->size=0;
}

void queue_add(queue_t *q, void *value)
{
    pthread_mutex_lock(&q->mtx);

    /* Add element normally. */

    q->size=q->size+1;
    pthread_mutex_unlock(&q->mtx);

    /* Signal waiting threads. */
    pthread_cond_signal(&q->cond);
}

void queue_get(queue_t *q, void **val_r)
{
    int rc;
    pthread_mutex_lock(&q->mtx);

    /* Wait for element to become available. */
    while (q->size == 0) {
        rc = pthread_cond_wait(&q->cond, &q->mtx);
    }
    /* We have an element. Pop it normally and return it in val_r. */
    q->size=q->size-1;

    pthread_mutex_unlock(&q->mtx);
}

void *directoryWorker(void *arg)
{
    long                retval = 0;

    assert (arg);
    dir_mgr_t *self = (dir_mgr_t *)arg;

    

    return NULL;
}



int copyDirectory(const char *source, int argc, char **argv)
{
    int         i;
    dir_mgr_t   d;

    queue_init(&d.q);
    d.argc=argc;
    d.argv=argv;
    
    for (i=0; i<MCP_FILE_READERS; i++) {
        // launch worker threads
    }

    // iterate over dir structure adding elements to q

    for (i=0; i<MCP_FILE_READERS; i++) {
        // join worker threads after they are done
    }

    return 0;
}
