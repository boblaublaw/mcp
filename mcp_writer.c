#include <stdio.h>      // printf
#include <pthread.h>    // pthread_*
#include <unistd.h>     // sleep
#include <string.h>     // strerrno
#include <errno.h>      // errno
#include "mcp.h"

void *startWriter(void *arg)
{
    long retval = 0;
    FILE *stream = NULL;

    mcp_writer_t *self;
    self = (mcp_writer_t *)arg;
    printf("writer thread %d launching with target %s\n", self->tid, self->filename);

    if (!self->forceOverwrite) {
        // XXX check if the file exists. if it does, BAIL
        retval = -1;
        pthread_exit((void*) retval);
    }

    if (NULL == (stream = fopen (self->filename, "w+"))) {
        fprintf(stderr, "Could not open %s for writing: %s\n", self->filename, strerror(errno));
    }

    fclose(stream);
    pthread_exit((void*) retval);
}
