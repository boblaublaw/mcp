#include <stdio.h>      // printf
#include <pthread.h>    // pthread_*
#include <unistd.h>     // sleep
#include <string.h>     // strerrno
#include <errno.h>      // errno
#include <sys/stat.h>   // stat()
#include <assert.h>     // assert()
#include "mcp.h"

extern int verbosity;
extern int cancel;

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
    FILE                *stream = NULL;
    struct stat         sb;
    int                 i=0;

    assert (arg);
    bzero(&sb,sizeof(sb));
    mcp_writer_t *self = (mcp_writer_t *)arg;
    if (verbosity) {
        pthread_mutex_lock(&self->mr->debugLock);
        fprintf(stderr, "writer %d:  launching with target %s\n", 
           self->tid, self->filename);
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
                cancel = 1;
                goto thread_exit;
            }
        }
        else {
            // if we can stat the file, it exists and we should not overwrite it
            retval=-1;
            fprintf(stderr, "File exists (-f to force): %s\n", self->filename);
            cancel = 1;
            goto thread_exit;
        }
    }

    if (NULL == (stream = fopen (self->filename, "w+"))) {
        fprintf(stderr, "Could not open %s for writing: %s\n", self->filename, strerror(errno));
    }
   
    while (1) {
        if (verbosity)
            fprintf (stderr, "writer %d about to wait for BUF A barrier\n", self->tid);

        if (-1 == (retval = pthread_barrier_waitcancel(&self->mr->barrier[BUF_A], &cancel))) {
            fprintf(stderr, "writer %d: failed to wait for BUF A barrier\n", self->tid);
            cancel = 1;
            goto thread_exit;
        }

        if (verbosity) 
            fprintf (stderr, "writer %d done waiting for readBarrier\n", self->tid);

        // ========================= A BUFFER WRITE, B BUFFER READ START====================
        
        if ((self->mr->bufBytes[BUF_A] == 0 ) || cancel)
            break;

        if (self->mr->bufBytes[BUF_A] != fwrite(self->mr->buf[BUF_A], 1, self->mr->bufBytes[BUF_A], stream)) {
            retval=-1;
            cancel = 1;
            goto thread_exit;
        }

        if (verbosity) {
            pthread_mutex_lock(&self->mr->debugLock);
            fprintf( stderr, "writer %d wrote %ld bytes\n", self->tid, self->mr->bufBytes[BUF_A]);
            pthread_mutex_unlock(&self->mr->debugLock);
        }

        // ========================= A BUFFER WRITE, B BUFFER READ END======================

        if (-1 == (retval = pthread_barrier_waitcancel(&self->mr->barrier[BUF_B], &cancel))) {
            fprintf(stderr, "writer %d: failed to wait for BUF A barrier\n", self->tid);
            cancel = 1;
            goto thread_exit;
        }

        // ========================= A BUFFER READ, B BUFFER WRITE START====================
        
        if ((self->mr->bufBytes[BUF_B] == 0) || cancel )
            break;

        if (self->mr->bufBytes[BUF_B] != fwrite(self->mr->buf[BUF_B], 1, self->mr->bufBytes[BUF_B], stream)) {
            retval=-1;
            cancel = 1;
            goto thread_exit;
        }

        if (verbosity) {
            pthread_mutex_lock(&self->mr->debugLock);
            fprintf( stderr, "writer %d wrote %ld bytes\n", self->tid, self->mr->bufBytes[BUF_B]);
            pthread_mutex_unlock(&self->mr->debugLock);
        }

        // ========================= A BUFFER WRITE, B BUFFER READ END======================
    }

thread_exit:
    if (stream) {
        fclose(stream);
        stream=NULL;
    }
    pthread_exit((void*) retval);
}

/* vim: set noet sw=5 ts=4: */
