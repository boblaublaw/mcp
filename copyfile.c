#include <errno.h>          // errno
#include <string.h>         // strerror
#include "mcp.h"            // mcp_writer_t

extern int exitFlag,
    hashFiles,
    forceOverwrite;
#if 0
    createParents,
#endif

extern mcp_reader_t reader;
extern pthread_attr_t      attr;

// this function does not return
int copyFile(const char *source, int argc, char **argv) 
{
    void                *thread_status;
    long                retval=0;
    unsigned            numWriters, writerIndex;
    mcp_writer_t        writers[MCP_MAX_WRITERS];

    numWriters = 0;
    bzero(writers, sizeof(writers));

    // at this point, argc is equal to the number of threads (writers plus readers)
    if (-1 == initReader(&reader, source, argc, hashFiles)) {
        return -1;
    }

    // launch all writer threads
    while (argc) {
        mcp_writer_t *mw = &writers[numWriters];
        mw->filename = strdup(argv[0]);
        mw->tid = numWriters;
        mw->forceOverwrite = forceOverwrite;
        mw->mr = &reader;
        retval = pthread_create(&mw->thread, &attr, startWriter, 
            (void *)mw);
        if (retval != 0 ) {
            printf("ERROR; return code from pthread_create() is %ld\n", retval);
            return retval;
        }
        argc -= 1;
        argv += 1;
        numWriters += 1;
    }

    // launch reader thread
    if (0 != (retval = pthread_create(&reader.thread, &attr, startReader, (void *)&reader))) {
        logFatal("return code from pthread_create() is %ld\n", retval);    
    }

    // wait for the reader to exit
    if (-1 == (retval = pthread_join(reader.thread, &thread_status))) {
        logFatal("failed to join reader:%s\n", strerror(errno));
    }

    // wait for all writers to exit
    for (writerIndex=0; writerIndex < numWriters; writerIndex++) {
        if (-1 == (retval = pthread_join(writers[writerIndex].thread, &thread_status))) {
            logError("writer %d failed to join:%s\n", 
                writers[writerIndex].tid, strerror(errno));
        }
        else {
            logDebug("writer %d joined with status %ld\n", 
                writers[writerIndex].tid, (long)thread_status);
        }
        // even if the join is successful, we combine the returned
        // statuses together to check if any of them were failures
        retval |= (long)thread_status;
    }
    logDebug2("Main: completed joins with final return value of %ld\n",
        retval);

    // if hash files were requested, write them now
    if (hashFiles && !exitFlag) {
        for (writerIndex=0; writerIndex < numWriters; writerIndex++) {
            mcp_writer_t *mw = &writers[writerIndex];
            writeHashFile(mw->filename, mw->mr->md5sum);
        }
    }

    return retval;
}
