#include <stdio.h>      // fopen
#include <assert.h>     // assert
#include <string.h>     // bzero, strerrno
#include <errno.h>      // errno
#include <pthread.h>    // pthread*


#include "mcp.h"

int initReader(mcp_reader_t *mr, char *filename)
{
    assert(filename);
    assert(mr);

    bzero(mr, sizeof(mcp_reader_t));
    if (NULL == (mr->source = fopen(filename,"r"))) {
        fprintf(stderr, "Could not open %s: %s\n", filename, strerror(errno));
        return -1;
    }

    // determine the size of the input file
    fseek(mr->source, 0L, SEEK_END);
    mr->size = ftell(mr->source);
    fseek(mr->source, 0L, SEEK_SET);

    // initialize the condition variable and mutex
    pthread_mutex_init(&mr->data_mutex, NULL);
    pthread_cond_init (&mr->dataEmpty_cv, NULL);
    pthread_cond_init (&mr->dataFull_cv, NULL);

    return 0;
}

int startReader(mcp_reader_t *mr)
{
    assert(mr);
    assert(mr->source);

    while(!feof(mr->source)) {
        if (-1 == (mr->dataBytes = fread(mr->data, 1, PAGESIZE, mr->source))) {
            fprintf(stderr, "failed to read from %s: %s\n", mr->filename, strerror(errno));
            return -1;
        }
    }
#if 0
        pthread_mutex_lock(&mr->data_mutex);
        if (mr->writerStatus == 0) {
            pthread_mutex_unlock(&mr->data_mutex);
            pthread_mutex_lock(&mr->data_mutex);
            mr->writerStatus=1;
            pthread_cond_signal(&mr->data_cv);
            pthread_mutex_unlock(&mr->data_mutex);
        }
        else {
            pthread_mutex_unlock(&mr->data_mutex);
        }
    }
#endif

    fclose(mr->source);

    return 0;
}

/* vim: set noet sw=5 ts=4: */

