#include "mcp.h"
#include <assert.h>
#include <stdio.h>          // strerror()
#include <string.h>         // strerror(), bzero()
#include <errno.h>          // errno
#include <stdlib.h>         // free()

// globals
extern pthread_attr_t      attr;

typedef struct elem_t
{
    char *name;
    struct elem_t *next;
} elem_t;

typedef struct queue 
{
    pthread_mutex_t mtx;      
    pthread_cond_t  cond;   
    int             size;   // length of the list 
    elem_t          *head;  // head of the linked list of values
    int             drain;  // set this when you have no more elements to add
} queue_t;

typedef struct dir_mgr_t 
{
    int         argc;
    char        **argv;
    queue_t     q;
    int         exitFlag;
    pthread_t   t[MCP_FILE_READERS];
} dir_mgr_t;

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

void *directoryWorker(void *arg)
{
    long                retval = 0;
    char                *sourceFile;

    assert (arg);
    dir_mgr_t *self = (dir_mgr_t *)arg;

    while (!self->exitFlag) {
        if (queue_get(&self->q, &sourceFile)) {
            logInfo("about to copy source file %s\n", sourceFile);
            retval = copyFile(sourceFile, self->argc, self->argv);
            logInfo("copied source file %s with retval %ld\n", sourceFile, retval);
            free(sourceFile);
            sourceFile=NULL;
        }
        else
            break;
    }

    pthread_exit((void*) retval);
}

int copyDirectory(const char *source, int argc, char **argv)
{
    int         i;
    dir_mgr_t   d;
    long        retval=0;
    void        *thread_status;

    queue_init(&d.q);
    d.argc=argc;
    d.argv=argv;
    d.exitFlag=0;
    
    for (i=0; i<MCP_FILE_READERS; i++) {
        if (0 != (retval = pthread_create(&d.t[i], &attr, directoryWorker, (void *)&d))) {
            logFatal("ERROR; return code from pthread_create() is %ld\n", retval);
            return retval;
        }
    }

    // iterate over dir structure adding elements to q
    queue_add(&d.q, "poop");
    queue_add(&d.q, "pee");
    // mark the queue as read only now
    queue_drain(&d.q);

    for (i=0; i<MCP_FILE_READERS; i++) {
        if (-1 == pthread_join(d.t[i], &thread_status)) {
            logFatal("failed to join file reader:%s\n", strerror(errno));
            retval |= -1;
        }
    }

    return retval;
}
