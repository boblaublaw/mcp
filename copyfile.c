#include <errno.h>          // errno
#include <string.h>         // strerror
#include "mcp.h"            // mcp_writer_t

mcp_reader_t        fileReader;

extern int  exitFlag,
            hashFiles,
            forceOverwrite;

extern pthread_attr_t      attr;

int copyFile(const char *source, int argc, char **argv) 
{
    void                *thread_status;
    long                retval=0;
    unsigned            numWriters, writerIndex;
    mcp_writer_t        writers[MCP_MAX_WRITERS];

    numWriters = 0;
    bzero(writers, sizeof(writers));

    // at this point, argc is equal to the number of threads (writers plus file readers)
    if (-1 == initReader(&fileReader, source, argc, hashFiles)) {
        return -1;
    }

    // launch all writer threads
    while (argc) {
        mcp_writer_t *mw = &writers[numWriters];
        mw->filename = strdup(argv[0]);
        mw->tid = numWriters;
        mw->forceOverwrite = forceOverwrite;
        mw->mr = &fileReader;
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

    // launch file reader thread
    if (0 != (retval = pthread_create(&fileReader.thread, &attr, startReader, (void *)&fileReader))) {
        logFatal("return code from pthread_create() is %ld\n", retval);    
    }

    // wait for the file reader to exit
    if (-1 == (retval = pthread_join(fileReader.thread, &thread_status))) {
        logFatal("failed to join file reader:%s\n", strerror(errno));
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
