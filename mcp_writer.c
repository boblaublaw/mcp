#include <stdio.h>      // printf
#include <pthread.h>    // pthread_*
#include <unistd.h>     // sleep
#include <string.h>     // strerrno
#include <errno.h>      // errno
#include <sys/stat.h>   // stat()
#include <assert.h>     // assert()
#include "mcp.h"

extern int debugLevel;

int
writeHashFile(char *basename, unsigned char *md5sum)
{
    char *hashFileName;
    FILE *hashFile;
    int i;

    if (-1 == asprintf(&hashFileName, "%s.md5", basename)) {
        return -1;
    }
    if (NULL == (hashFile = fopen (hashFileName, "w+"))) {
        return -1;
    }
    for (i=0; i<CC_MD5_DIGEST_LENGTH; i++)
        fprintf(hashFile, "%02x", md5sum[i]);
        fprintf(hashFile, "\n");
        fclose(hashFile);
    return 0;
}

void *startWriter(void *arg)
{
    long                retval = 0;
    long                bytesRemaining = 0;
    FILE                *stream = NULL;
    struct stat         sb;
    int                 i=0;

    assert (arg);
    bzero(&sb,sizeof(sb));
    mcp_writer_t *self = (mcp_writer_t *)arg;
    if (debugLevel) {
        pthread_mutex_lock(&self->mr->debugLock);
        fprintf(stderr, "writer %d:  launching with target %s (size %ld)\n", 
           self->tid, self->filename, self->mr->size);
        pthread_mutex_unlock(&self->mr->debugLock);
    }

    if (!self->forceOverwrite) {
        if (-1 == stat(self->filename,&sb)) {
            if (errno == ENOENT) {
                // ENOENT is acceptable.
                // destination file doesn't exist yet.
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
        if (debugLevel)
            fprintf (stderr, "writer %d about to wait for readBarrier\n", self->tid);
        if (-1 == (retval = pthread_barrier_wait(&self->mr->readBarrier))) {
            fprintf(stderr, "writer %d: failed to wait for read barrier\n", self->tid);
            pthread_exit((void*) retval);
        }
        if (debugLevel) 
            fprintf (stderr, "writer %d done waiting for readBarrier\n", self->tid);

        // ========================= A BUFFER WRITE, B BUFFER READ START====================
        
        if (self->mr->bufBytes[BUF_A] != fwrite(self->mr->buf[BUF_A], 1, self->mr->bufBytes[BUF_A], stream)) {
            retval=-1;
            pthread_exit((void*) retval);
        }

        if (debugLevel) {
            pthread_mutex_lock(&self->mr->debugLock);
            fprintf( stderr, "writer %d wrote %ld bytes\n", self->tid, self->mr->bufBytes[BUF_A]);
            pthread_mutex_unlock(&self->mr->debugLock);
        }

        self->bytesWritten += self->mr->bufBytes[BUF_A];
        bytesRemaining = (self->mr->size - self->bytesWritten);

        if (bytesRemaining == 0)
            break;

        // ========================= A BUFFER WRITE, B BUFFER READ END======================

        if (-1 == (retval == pthread_barrier_wait(&self->mr->writeBarrier))) {
            fprintf(stderr, "writer %d: failed to wait for write barrier\n", self->tid);
            pthread_exit((void*) retval);
        }

        // ========================= A BUFFER READ, B BUFFER WRITE START====================
        
        if (self->mr->bufBytes[BUF_B] != fwrite(self->mr->buf[BUF_B], 1, self->mr->bufBytes[BUF_B], stream)) {
            retval=-1;
            pthread_exit((void*) retval);
        }

        if (debugLevel) {
            pthread_mutex_lock(&self->mr->debugLock);
            fprintf( stderr, "writer %d wrote %ld bytes\n", self->tid, self->mr->bufBytes[BUF_B]);
            pthread_mutex_unlock(&self->mr->debugLock);
        }

        self->bytesWritten += self->mr->bufBytes[BUF_B];
        bytesRemaining = (self->mr->size - self->bytesWritten);
        
        // ========================= A BUFFER WRITE, B BUFFER READ END======================
    }

    fclose(stream);
    pthread_exit((void*) retval);
}

/* vim: set noet sw=5 ts=4: */
