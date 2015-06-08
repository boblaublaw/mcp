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

    pthread_mutex_init(&mr->debugLock, NULL);
    pthread_mutex_init(&mr->nonReentrantLock, NULL);

    if (verbosity) {
        pthread_mutex_lock(&mr->debugLock);
        fprintf(stderr, "initializing mcp to read from file %s and write to %d writers\n", filename, writerCount);
        fprintf(stderr, "initializing mcp with %d total threads\n", threadCount);
        pthread_mutex_unlock(&mr->debugLock);
    }

    if (0 == strcmp("-",filename)) {
        mr->source=stdin;
    }
    else if (NULL == (mr->source = fopen(filename,"r"))) {
        fprintf(stderr, "Could not open %s: %s\n", filename, strerror(errno));
        return -1;
    }

    // initialize sync structures
    for (i=0; i < NUMBUF; i++) {
        if (verbosity) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf(stderr, "initializing BUF %c barrier for %d threads\n", 
                (i ? 'B' : 'A'), threadCount);
            pthread_mutex_unlock(&mr->debugLock);
        }
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

#if 0
    int flags;
    flags = fcntl(fileno(mr->source), F_GETFL, 0);
    fprintf(stderr,"flags are %x\n", flags);
    fcntl(fileno(mr->source), F_SETFL, flags | O_NONBLOCK);
    flags = fcntl(fileno(mr->source), F_GETFL, 0);
    fprintf(stderr,"flags are %x\n", flags);
#endif

    while(1) {

        if (verbosity) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf (stderr, "reader reached loop begin\n");
            fflush(stderr);
            pthread_mutex_unlock(&mr->debugLock);
        }

        // ========================= A BUFFER READ, B BUFFER WRITE START====================

        if (verbosity) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf(stderr, "reader about to read into BUF A\n");
            fflush(stderr);
            pthread_mutex_unlock(&mr->debugLock);
        }

        if (-1 == (mr->bufBytes[BUF_A]  = fread(mr->buf[BUF_A], 1, PAGESIZE, mr->source))) {
            fprintf(stderr, "failed to read from %s: %s\n", mr->filename, strerror(errno));
            fflush(stderr);
            retval = EXIT_FAILURE;
            exitFlag = 1;
            goto pthread_exit;
        }

        if (mr->hashFiles && mr->bufBytes[BUF_A])
            CC_MD5_Update(&mr->md5state, mr->buf[BUF_A], mr->bufBytes[BUF_A]);

        if (verbosity) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf(stderr, "reader read %ld bytes into BUF A\n", mr->bufBytes[BUF_A]);
            fflush(stderr);
            pthread_mutex_unlock(&mr->debugLock);
        }

        // ========================= A BUFFER READ, B BUFFER WRITE END======================

        //  make sure all the writers are done writing before we advance any further
        if (verbosity) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf (stderr, "reader about to wait for BUF A barrier\n");
            fflush(stderr);
            pthread_mutex_unlock(&mr->debugLock);
        }

        if (-1 == (retval = pthread_barrier_waitcancel(&mr->barrier[BUF_A], &exitFlag))) {
            if (!exitFlag) {
                fprintf (stderr, "reader failed to wait for BUF A barrier\n");
                fflush(stderr);
                retval = EXIT_FAILURE;
            }
            exitFlag=1;
            goto pthread_exit;
        }

        if (retval == ETIMEDOUT) {
            if (!exitFlag) {
                fprintf (stderr, "reader timed out on BUF A\n");
                fflush(stderr);
            }
            retval = EXIT_FAILURE;
            exitFlag=1;
            goto pthread_exit;
        }

        if (verbosity) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf (stderr, "reader done waiting for BUF A barrier\n");
            fflush(stderr);
            pthread_mutex_unlock(&mr->debugLock);
        }
        
        if (exitFlag) {
            if (verbosity) {
                pthread_mutex_lock(&mr->debugLock);
                fprintf (stderr, "reader told to shut down\n");
                fflush(stderr);
                pthread_mutex_unlock(&mr->debugLock);
            }
            break;
        }

        if (mr->bufBytes[BUF_A] == 0) {
            if (verbosity) {
                pthread_mutex_lock(&mr->debugLock);
                fprintf (stderr, "reader ran out of data in BUF A\n");
                fflush(stderr);
                pthread_mutex_unlock(&mr->debugLock);
            }
            break;
        }

        // ========================= B BUFFER READ, A BUFFER WRITE START====================
       
        if (verbosity) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf(stderr, "reader about to read into BUF B\n");
            fflush(stderr);
            pthread_mutex_unlock(&mr->debugLock);
        }

        if (-1 == (mr->bufBytes[BUF_B]  = fread(mr->buf[BUF_B], 1, PAGESIZE, mr->source))) {
            fprintf(stderr, "failed to read from %s: %s\n", mr->filename, strerror(errno));
            fflush(stderr);
            retval = EXIT_FAILURE;
            exitFlag = 1;
            goto pthread_exit;
        }

        if (mr->hashFiles && mr->bufBytes[BUF_B])
            CC_MD5_Update(&mr->md5state, mr->buf[BUF_B], mr->bufBytes[BUF_B]);

        if (verbosity) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf(stderr, "reader read %ld bytes into BUF B\n", mr->bufBytes[BUF_A]);
            fflush(stderr);
            pthread_mutex_unlock(&mr->debugLock);
        }

        // ========================= B BUFFER READ, A BUFFER WRITE END======================
        
        if (verbosity) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf (stderr, "reader about to wait for BUF B barrier\n");
            fflush(stderr);
            pthread_mutex_unlock(&mr->debugLock);
        }
        
        if (-1 == (retval = pthread_barrier_waitcancel(&mr->barrier[BUF_B], &exitFlag))) {
            if (!exitFlag) {
                fprintf (stderr, "reader failed to wait for BUF B barrier\n");
                fflush(stderr);
                retval = EXIT_FAILURE;
            }
            exitFlag=1;
            goto pthread_exit;
        }

        if (retval == ETIMEDOUT) {
            if (!exitFlag) {
                fprintf (stderr, "reader timed out on BUF B\n");
                fflush(stderr);
            }
            retval = EXIT_FAILURE;
            exitFlag=1;
            goto pthread_exit;
        }

        if (verbosity) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf (stderr, "reader done waiting for BUF B barrier\n");
            fflush(stderr);
            pthread_mutex_unlock(&mr->debugLock);
        }

        if (exitFlag) {
            if (verbosity) {
                pthread_mutex_lock(&mr->debugLock);
                fprintf (stderr, "reader told to shut down\n");
                fflush(stderr);
                pthread_mutex_unlock(&mr->debugLock);
            }
            break;
        }

        if (mr->bufBytes[BUF_B] == 0) {
            if (verbosity) {
                pthread_mutex_lock(&mr->debugLock);
                fprintf (stderr, "reader ran out of data in BUF B\n");
                fflush(stderr);
                pthread_mutex_unlock(&mr->debugLock);
            }
            break;
        }

        if (verbosity) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf (stderr, "reader reached loop end\n");
            fflush(stderr);
            pthread_mutex_unlock(&mr->debugLock);
        }
    }

    if (verbosity) {
        pthread_mutex_lock(&mr->debugLock);
        fprintf (stderr, "reader closing file\n");
        fflush(stderr);
        pthread_mutex_unlock(&mr->debugLock);
    }

pthread_exit:
    if (mr->source) {
        fclose(mr->source);
        mr->source=NULL;
    }
    if (verbosity) {
        pthread_mutex_lock(&mr->debugLock);
        fprintf (stderr, "reader closed file\n");
        fflush(stderr);
        pthread_mutex_unlock(&mr->debugLock);
    }

    if (!exitFlag && mr->hashFiles) {
        int i;
        CC_MD5_Final(mr->md5sum, &mr->md5state);
        if (verbosity) {
            pthread_mutex_lock(&mr->debugLock);
            fprintf(stderr, "md5 state is: ");
            for (i=0; i<CC_MD5_DIGEST_LENGTH; i++) 
                fprintf(stderr, "%02x", mr->md5sum[i]);
            fprintf(stderr, "\n");
            pthread_mutex_unlock(&mr->debugLock);
        }
    }
    if (verbosity) {
        pthread_mutex_lock(&mr->debugLock);
        fprintf (stderr, "reader exiting\n");
        fflush(stderr);
        pthread_mutex_unlock(&mr->debugLock);
    }
    pthread_exit((void*) retval);
}

/* vim: set noet sw=5 ts=4: */
