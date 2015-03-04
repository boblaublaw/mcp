#include <stdio.h>      // fopen
#include <assert.h>     // assert
#include <string.h>     // bzero, strerrno
#include <errno.h>      // errno
#include <stdlib.h>     // EXIT_FAILURE
#include <pthread.h>    // pthread*


#include "mcp.h"

int initReader(mcp_reader_t *mr, char *filename, int count)
{
    assert(filename);
    assert(mr);

    bzero(mr, sizeof(mcp_reader_t));

    //printf("initializing mcp to read from file %s and write to %d writers\n", filename, count);

    if (NULL == (mr->source = fopen(filename,"r"))) {
        fprintf(stderr, "Could not open %s: %s\n", filename, strerror(errno));
        return -1;
    }

    // determine the size of the input file
    fseek(mr->source, 0L, SEEK_END);
    mr->size = ftell(mr->source);
    fseek(mr->source, 0L, SEEK_SET);

    // sync structures
    if (0 != pthread_barrier_init(&mr->readBarrier, NULL, count))
        return -1;
    if (0 != pthread_barrier_init(&mr->writeBarrier, NULL, count))
        return -1;

    return 0;
}

int startReader(mcp_reader_t *mr)
{
    assert(mr);
    assert(mr->source);
    
    int retval = 0;

    while(!feof(mr->source)) {
        if (-1 == (mr->bufBytes = fread(mr->buf, 1, PAGESIZE, mr->source))) {
            fprintf(stderr, "failed to read from %s: %s\n", mr->filename, strerror(errno));
            return(EXIT_FAILURE);
        }

        if (mr->bufBytes == 0)
            break;

        //  wait for the writers here
        //printf("reader read %ld bytes. now waiting for read barrier\n", mr->bufBytes);
        //fflush(stdout);
        retval = pthread_barrier_wait(&mr->readBarrier);
        
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
            return(EXIT_FAILURE);
        }
           
        //  and here
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
            return(EXIT_FAILURE);
        }
    }
    fclose(mr->source);

    return 0;
}

/* vim: set noet sw=5 ts=4: */
