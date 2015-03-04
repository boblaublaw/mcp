#include <stdio.h>      // printf
#include <pthread.h>    // pthread_*
#include <unistd.h>     // sleep
#include <string.h>     // strerrno
#include <errno.h>      // errno
#include <sys/stat.h>   // stat()
#include "mcp.h"



void *startWriter(void *arg)
{
    long                retval = 0;
    long                bytesRemaining = 0;
    FILE                *stream = NULL;
    struct stat         sb;

    bzero(&sb,sizeof(sb));
    mcp_writer_t *self = (mcp_writer_t *)arg;
    printf("writer thread %d launching with target %s (%ld)\n", 
       self->tid, self->filename, self->finalSize);

    if (!self->forceOverwrite) {
        if (-1 == stat(self->filename,&sb)) {
            if (errno == ENOENT) {
                // ENOENT is acceptable
            }
            else {
                // other errors are unacceptable
                retval = -1;
                fprintf(stderr, "Failed to stat %s: %s\n", self->filename, strerror(errno));
                pthread_exit((void*) retval);
            }
        }
        else {
            // if we can stat the file, it exists and we should not overwrite it
            retval=-1;
            fprintf(stderr, "File exists (-f to force): %s\n", self->filename);
            pthread_exit((void*) retval);
        }
    }

    if (NULL == (stream = fopen (self->filename, "w+"))) {
        fprintf(stderr, "Could not open %s for writing: %s\n", self->filename, strerror(errno));
    }

    while (0 < (bytesRemaining = (self->finalSize - self->bytesWritten))) {
        printf ("there are %ld bytes remaining\n", bytesRemaining);
    
        // check a condition
        // check some global variable
        // read some data

    }

    fclose(stream);
    pthread_exit((void*) retval);
}

/* vim: set noet sw=5 ts=4: */
