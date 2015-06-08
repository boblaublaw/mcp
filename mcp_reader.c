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

void *startReader(void *arg) 

{
    assert(arg);
    mcp_reader_t *mr = (mcp_reader_t *)arg;
    assert(mr->source);
    
    long retval = 0;

    while(1) {

        logDebug2("reader reached loop begin\n");

        // ========================= A BUFFER READ, B BUFFER WRITE START====================

        logDebug("reader about to read into BUF A\n");

        if (-1 == (mr->bufBytes[BUF_A]  = fread(mr->buf[BUF_A], 1, PAGESIZE, mr->source))) {
            logError("failed to read from %s: %s\n", mr->filename, strerror(errno));
            retval = EXIT_FAILURE;
            exitFlag = 1;
            goto pthread_exit;
        }

        if (mr->hashFiles && mr->bufBytes[BUF_A])
            CC_MD5_Update(&mr->md5state, mr->buf[BUF_A], mr->bufBytes[BUF_A]);

        logDebug("reader read %ld bytes into BUF A\n", mr->bufBytes[BUF_A]);

        // ========================= A BUFFER READ, B BUFFER WRITE END======================

        //  make sure all the writers are done writing before we advance any further
        logDebug("reader about to wait for BUF A barrier\n");

        if (-1 == (retval = pthread_barrier_waitcancel(&mr->barrier[BUF_A], &exitFlag))) {
            if (!exitFlag) {
                logDebug("reader failed to wait for BUF A barrier\n");
                retval = EXIT_FAILURE;
            }
            exitFlag=1;
            goto pthread_exit;
        }

        if (retval == ETIMEDOUT) {
            if (!exitFlag) {
                logDebug("reader timed out on BUF A\n");
            }
            retval = EXIT_FAILURE;
            exitFlag=1;
            goto pthread_exit;
        }

        logDebug("reader done waiting for BUF A barrier\n");
        
        if (exitFlag) {
            logDebug("reader told to shut down\n");
            break;
        }

        if (mr->bufBytes[BUF_A] == 0) {
            logDebug("reader ran out of data in BUF A\n");
            break;
        }

        // ========================= B BUFFER READ, A BUFFER WRITE START====================
       
        logDebug("reader about to read into BUF B\n");

        if (-1 == (mr->bufBytes[BUF_B]  = fread(mr->buf[BUF_B], 1, PAGESIZE, mr->source))) {
            logError("failed to read from %s: %s\n", mr->filename, strerror(errno));
            retval = EXIT_FAILURE;
            exitFlag = 1;
            goto pthread_exit;
        }

        if (mr->hashFiles && mr->bufBytes[BUF_B])
            CC_MD5_Update(&mr->md5state, mr->buf[BUF_B], mr->bufBytes[BUF_B]);

        logDebug("reader read %ld bytes into BUF B\n", mr->bufBytes[BUF_A]);

        // ========================= B BUFFER READ, A BUFFER WRITE END======================
        
        logDebug("reader about to wait for BUF B barrier\n");
            
        if (-1 == (retval = pthread_barrier_waitcancel(&mr->barrier[BUF_B], &exitFlag))) {
            if (!exitFlag) {
                logDebug("reader failed to wait for BUF B barrier\n");
                retval = EXIT_FAILURE;
            }
            exitFlag=1;
            goto pthread_exit;
        }

        if (retval == ETIMEDOUT) {
            if (!exitFlag) {
                logDebug("reader timed out on BUF B\n");
            }
            retval = EXIT_FAILURE;
            exitFlag=1;
            goto pthread_exit;
        }

        logDebug("reader done waiting for BUF B barrier\n");

        if (exitFlag) {
            logDebug("reader told to shut down\n");
            break;
        }

        if (mr->bufBytes[BUF_B] == 0) {
            logDebug("reader ran out of data in BUF B\n");
            break;
        }

        logDebug2("reader reached loop end\n");
    }

    logDebug("reader closing file\n");

pthread_exit:
    if (mr->source) {
        fclose(mr->source);
        mr->source=NULL;
    }
    logDebug("reader closed file\n");

    if (!exitFlag && mr->hashFiles) {
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
