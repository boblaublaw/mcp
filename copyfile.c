#include <errno.h>          // errno
#include <string.h>         // strerror
#include <stdlib.h>         // free
#include "mcp.h"            // mcp_writer_t

extern int  hashFiles,
            forceOverwrite;

extern pthread_attr_t      attr;

//  source is a filename
//  argc is the number of destination files
//  argv is an argument vector of destinations
//      destinations can be files or directories
//      if the destination is a directory, append
//      the source file name
int copyFile(const char *source, int argc, char **argv, const char *appendStr)
{
    void                *thread_status;
    long                retval=0;
    unsigned            numWriters, writerIndex;
    mcp_writer_t        writers[MCP_MAX_WRITERS];
    mcp_reader_t        *fileReader;

    numWriters = 0; 
    fileReader = NULL;
    bzero(writers, sizeof(writers));

    // at this point, argc is equal to the number of threads (writers plus file readers)
    if (NULL == (fileReader = initReader(source, argc, hashFiles))) {
        logError("could not create a fileReader\n");
        return -1;
    }

    // launch all writer threads
    while (argc) {
        mcp_writer_t *mw = &writers[numWriters];
        if (appendStr)
            asprintf(&mw->filename, "%s/%s", argv[0], appendStr);
        else
            mw->filename = strdup(argv[0]);
        logDebug("copying %s into %s\n", source, mw->filename);
        mw->tid = numWriters;
        mw->forceOverwrite = forceOverwrite;
        mw->mr = fileReader;
        retval = pthread_create(&mw->thread, &attr, startWriter, 
            (void *)mw);
        if (retval != 0 ) {
            logError("ERROR; return code from pthread_create() is %ld\n", retval);
            mw->mr->exitFlag=1;
            return retval;
        }
        argc -= 1;
        argv += 1;
        numWriters += 1;
    }

    // launch file reader thread
    if (0 != (retval = pthread_create(&fileReader->thread, &attr, startReader, (void *)fileReader))) {
        logFatal("return code from pthread_create() is %ld\n", retval);    
    }

    // wait for the file reader to exit
    if (-1 == (retval = pthread_join(fileReader->thread, &thread_status))) {
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
    if (hashFiles && !fileReader->exitFlag) {
        for (writerIndex=0; writerIndex < numWriters; writerIndex++) {
            mcp_writer_t *mw = &writers[writerIndex];
            writeHashFile(mw->filename, mw->mr->md5sum);
        }
    }
    free(fileReader);
    return retval;
}
