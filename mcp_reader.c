#include <stdio.h>          // fopen
#include <assert.h>         // assert
#include <string.h>         // bzero, strerrno
#include <errno.h>          // errno
#include <fcntl.h>          // fcntl()
#include <stdlib.h>         // EXIT_FAILURE
#include <pthread.h>        // pthread*

#include "mcp.h"

extern int verbosity;
extern int exitFlag;

int initReader(mcp_reader_t *mr, const char *filename, int writerCount, int hashFiles)
{
    assert(filename);
    assert(mr);
    int i;
    int threadCount = writerCount + 1;

    bzero(mr, sizeof(mcp_reader_t));
    mr->filename=filename;

    pthread_mutex_init(&mr->nonReentrantLock, NULL);

    logDebug("initializing mcp to read from file %s and write to %d writers\n", filename, writerCount);
    logDebug("initializing mcp with %d total threads\n", threadCount);

    if (0 == strcmp("-",filename)) {
        mr->source=stdin;
    }
    else if (NULL == (mr->source = fopen(filename,"r"))) {
        logError("Could not open %s: %s\n", filename, strerror(errno));
        return -1;
    }

    // initialize sync structures
    for (i=0; i < NUMBUF; i++) {
        logDebug("initializing BUF %c barrier for %d threads\n", 
            (i ? 'B' : 'A'), threadCount);
        if (0 != pthread_barrier_init(&mr->barrier[i], NULL, threadCount))
            return -1;
    }

    // initialize hashing state structure 
    mr->hashFiles=hashFiles;
    if (hashFiles) 
        if (1 != CC_MD5_Init(&mr->md5state))
            return -1;

    return 0;
}

//  return codes:
//                  -1 = something went wrong, exit before done
//                  0  = read all the data, exit normally
//                  1  = more data to read, keep going
int readIntoBuf(mcp_reader_t *mr, int bufId) 
{
    assert(mr);
    assert(mr->source);
    int retval;

    logDebug("reader about to read into BUF %d\n", bufId);

    if (-1 == (mr->bufBytes[bufId]  = fread(mr->buf[bufId], 1, PAGESIZE, mr->source))) {
        logError("failed to read from %s: %s\n", mr->filename, strerror(errno));
        exitFlag = 1;
        return -1;
    }

    if (mr->hashFiles && mr->bufBytes[bufId])
        CC_MD5_Update(&mr->md5state, mr->buf[bufId], mr->bufBytes[bufId]);

    logDebug("reader read %ld bytes into BUF %d\n", mr->bufBytes[bufId], bufId);
    logDebug("reader about to wait for BUF %d barrier\n", bufId);

    if (-1 == (retval = pthread_barrier_waitcancel(&mr->barrier[bufId], &exitFlag))) {
        if (!exitFlag) {
            logDebug("reader failed to wait for BUF %d barrier\n", bufId);
        }
        exitFlag=1;
        return -1;
    }

    if (retval == ETIMEDOUT) {
        if (!exitFlag) {
            logDebug("reader timed out on BUF %d\n", bufId);
        }
        exitFlag=1;
        return -1;
    }

    logDebug("reader done waiting for BUF %d barrier\n", bufId);
        
    if (exitFlag) {
        logDebug("reader told to shut down by another thread\n");
        return 0;
    }

    if (mr->bufBytes[bufId] == 0) {
        logDebug("reader ran out of data reading into BUF %d\n", bufId);
        return 0;
    }
    return 1;
}

void *startReader(void *arg) 

{
    assert(arg);
    mcp_reader_t *mr = (mcp_reader_t *)arg;
    assert(mr->source);
    
    long retval = 0;
    int bufId = 0;

    while(1) {
        if (1 != (retval = readIntoBuf(mr, bufId))) {
                goto pthread_exit;
        }
        bufId = !bufId;
        logDebug("setting new bufId to %d\n", bufId);
    }

    logDebug("reader closing file\n");

pthread_exit:
    if (mr->source) {
        fclose(mr->source);
        mr->source=NULL;
    }
    logDebug("reader closed file\n");

    if (!exitFlag && mr->hashFiles) {
        logDebug("reader hashing file\n");
        int i;
        CC_MD5_Final(mr->md5sum, &mr->md5state);
        logDebug("md5 state is: ");
        for (i=0; i<CC_MD5_DIGEST_LENGTH; i++) 
            logDebug("%02x", mr->md5sum[i]);
    }
    logDebug("reader exiting\n");
    pthread_exit((void*) retval);
}

/* vim: set noet sw=5 ts=4: */
