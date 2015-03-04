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
    printf("writer %d:  launching with target %s (%ld)\n", 
       self->tid, self->filename, self->mr->size);

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
   
    bytesRemaining = (self->mr->size - self->bytesWritten);
    while (bytesRemaining) {

        printf("writer %d: waiting for read barrier\n", self->tid);
        fflush(stdout);
        retval = pthread_barrier_wait(&self->mr->readBarrier);
        if (retval == -1) {
            fprintf(stderr, "writer %d: failed to wait for read barrier\n", self->tid);
            pthread_exit((void*) retval);
        }

        printf("writer %d: %sLAST %ld bytes ready.  %ld bytes remaining\n",
            self->tid, 
            (retval ? "" : "NOT "), 
            self->mr->bufBytes, 
            bytesRemaining);
        fflush(stdout);

        if (self->mr->bufBytes != fwrite(self->mr->buf, 1, self->mr->bufBytes, stream)) {
            retval=-1;
            pthread_exit((void*) retval);
        }

        self->bytesWritten += self->mr->bufBytes;
        bytesRemaining = (self->mr->size - self->bytesWritten);

        printf("writer %d: wrote %ld bytes. %ld remaining, waiting for write barrier\n", 
            self->tid, self->mr->bufBytes, bytesRemaining);
        fflush(stdout);

        pthread_barrier_wait(&self->mr->writeBarrier);
        if (retval == -1) {
            fprintf(stderr, "writer %d: failed to wait for write barrier\n", self->tid);
            pthread_exit((void*) retval);
        }
        printf ("writer %d: %sLAST to write buffer. %s\n", 
            self->tid, 
            ( retval ? "" : "NOT "),
            ( bytesRemaining ? "" : "exiting" ));                
        fflush(stdout);
    }

    fclose(stream);
    pthread_exit((void*) retval);
}

/* vim: set noet sw=5 ts=4: */
