#include <assert.h>
#include <stdio.h>          // strerror()
#include <string.h>         // strerror(), bzero()
#include <errno.h>          // errno
#include <stdlib.h>         // free()
#include "mcp.h"

// globals
extern pthread_attr_t      attr;

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
            retval = -1;
        }
        retval |= (long)thread_status;
    }

    return retval;
}
