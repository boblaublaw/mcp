#include <stdio.h>          // fopen
#include <assert.h>         // assert
#include <string.h>         // bzero, strerrno
#include <errno.h>          // errno
#include <stdlib.h>         // EXIT_FAILURE
#include <pthread.h>        // pthread*

#include "mcp.h"

extern int debugLevel;

int initReader(mcp_reader_t *mr, char *filename, int count, int hashFiles)
{
    assert(filename);
    assert(mr);

    bzero(mr, sizeof(mcp_reader_t));

    pthread_mutex_init(&mr->debugLock, NULL);

    //printf("initializing mcp to read from file %s and write to %d writers\n", filename, count);

    if (NULL == (mr->source = fopen(filename,"r"))) {
        fprintf(stderr, "Could not open %s: %s\n", filename, strerror(errno));
        return -1;
    }

    // determine the size of the input file
    fseek(mr->source, 0L, SEEK_END);
    mr->size = ftell(mr->source);
    fseek(mr->source, 0L, SEEK_SET);

    // initialize sync structures
    if (0 != pthread_barrier_init(&mr->readBarrier, NULL, count))
        return -1;
    if (0 != pthread_barrier_init(&mr->writeBarrier, NULL, count))
        return -1;

    // initialize hashing state structure 
    mr->hashFiles=hashFiles;
    if (hashFiles) 
        if (1 != CC_MD5_Init(&mr->md5state))
            return -1;

    return 0;
}

void *startReader(void *arg) 

{
    assert(arg);
    mcp_reader_t *mr = (mcp_reader_t *)arg;
    assert(mr->source);
    
    long retval = 0;

    while(!feof(mr->source)) {

        // ========================= A BUFFER READ, B BUFFER WRITE START====================

        if (-1 == (mr->bufBytes[BUF_A]  = fread(mr->buf[BUF_A], 1, PAGESIZE, mr->source))) {
            fprintf(stderr, "failed to read from %s: %s\n", mr->filename, strerror(errno));
            retval = EXIT_FAILURE;
            pthread_exit((void*) retval);
        }

        if (mr->hashFiles && mr->bufBytes[BUF_A])
            CC_MD5_Update(&mr->md5state, mr->buf[BUF_A], mr->bufBytes[BUF_A]);

        if (debugLevel) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf(stderr, "%ld bytes read into buf A\n", mr->bufBytes[BUF_A]);
            pthread_mutex_unlock(&mr->debugLock);
        }

        if (mr->bufBytes[BUF_A] == 0)
            break;

        // ========================= A BUFFER READ, B BUFFER WRITE END======================

        //  make sure all the writers are done writing before we advance any further
        if (debugLevel) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf (stderr, "reader about to wait for readBarrier\n");
            pthread_mutex_unlock(&mr->debugLock);
        }
        retval = pthread_barrier_wait(&mr->readBarrier);
        if (debugLevel) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf (stderr, "reader done waiting for readBarrier\n");
            pthread_mutex_unlock(&mr->debugLock);
        }
        
        if(retval == 0) {
            //printf("reader had to wait for others\n");
            //fflush(stdout);
        }
        else if (retval == 1) {
            //printf("reader was the last to the party\n");
            //fflush(stdout);
        }
        else {
            fprintf(stderr, "Could not wait on read barrier\n");
            retval = EXIT_FAILURE;
            pthread_exit((void*) retval);
        }

        // ========================= B BUFFER READ, A BUFFER WRITE START====================
       
        if (-1 == (mr->bufBytes[BUF_B]  = fread(mr->buf[BUF_B], 1, PAGESIZE, mr->source))) {
            fprintf(stderr, "failed to read from %s: %s\n", mr->filename, strerror(errno));
            retval = EXIT_FAILURE;
            pthread_exit((void*) retval);
        }

        if (mr->hashFiles && mr->bufBytes[BUF_B])
            CC_MD5_Update(&mr->md5state, mr->buf[BUF_B], mr->bufBytes[BUF_B]);

        if (debugLevel) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf(stderr, "%ld bytes read into buf B\n", mr->bufBytes[BUF_B]);
            pthread_mutex_unlock(&mr->debugLock);
        }

        if (mr->bufBytes[BUF_B] == 0)
            break;

        // ========================= B BUFFER READ, A BUFFER WRITE END======================

        // here we wait for ther 
        //printf("reader is waiting for write barrier\n");
        //fflush(stdout);
        retval = pthread_barrier_wait(&mr->writeBarrier);
        if(retval == 0) {
            //printf("reader had to wait for others\n");
            //fflush(stdout);
        }
        else if (retval == 1) {
            //printf("reader was the last to the party\n");
            //fflush(stdout);
        }
        else
        {
            fprintf(stderr, "Could not wait on barrier\n");
            retval = EXIT_FAILURE;
            pthread_exit((void*) retval);
        }
    }
    fclose(mr->source);
    if (mr->hashFiles) {
        int i;
        CC_MD5_Final(mr->md5sum, &mr->md5state);
        if (debugLevel) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf(stderr, "md5 state is: ");
            for (i=0; i<CC_MD5_DIGEST_LENGTH; i++) 
                fprintf(stderr, "%02x", mr->md5sum[i]);
            fprintf(stderr, "\n");
            pthread_mutex_unlock(&mr->debugLock);
        }
    }

    pthread_exit((void*) retval);
}



/* vim: set noet sw=5 ts=4: */
