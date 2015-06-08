#include <stdio.h>          // printf, etc
#include <stdlib.h>         // exit()
#include <unistd.h>         // getopt() 
#include <getopt.h>         // getopt()
#include <string.h>         // strdup()
#include <pthread.h>        // pthread_*
#include <assert.h>         // assert
#include <string.h>         // strerror
#include <errno.h>          // errno
#include <sys/stat.h>       // stat
#include "mcp.h"            // mcp_writer_t

static struct option longopts[] = {
    { "create-hashfiles",       no_argument,            NULL,           'h' },
    { NULL,                     0,                      NULL,           0 }
};

// global vars
int verbosity,
    exitFlag,
    hashFiles, 
    createParents,
    forceOverwrite;

mcp_reader_t reader;

void usage(long retval)
{
    logInfo("usage()\n");
    logInfo("\t-f: force overwrite destination file\n");
    logInfo("\t-h: create hash files for every source file\n");
    logInfo("\t-p: create parent directories where needed\n");
    logInfo("\t-v: increase verbosity (vv, vvv, etc)\n");
    exit(retval);
}

// this function does not return
void copyDirectory(const char *source, int argc, char **argv)
{
    logFatal("copying directories not yet supported\n");
}

// this function does not return
void copyFile(const char *source, int argc, char **argv) 
{
    pthread_attr_t      attr;
    void                *thread_status;
    long                retval=0;
    unsigned            numWriters, writerIndex;
    mcp_writer_t        writers[MCP_MAX_WRITERS];

    numWriters = 0;
    bzero(writers, sizeof(writers));
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // at this point, argc is equal to the number of threads (writers plus readers)
    if (-1 == initReader(&reader, source, argc, hashFiles)) {
        exit(EXIT_FAILURE);
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
            exit(EXIT_FAILURE);
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

    pthread_attr_destroy(&attr);
    exit(retval);
}


int main(int argc, char **argv)
{
    char                *source;
    struct stat         sb;
    int                 streamCopy,
                        ch;
    long                retval, sz;

    forceOverwrite = 0;
    streamCopy=0;
    hashFiles = 0; 
    createParents = 0;
    verbosity=0;
    exitFlag = 0;
    retval = 0;

    logInit(stderr);

    // parse arguments
    while ((ch = getopt_long(argc, argv, "fhpv", longopts, NULL)) != -1)
        switch (ch) {
            case 'f':
                forceOverwrite = 1;
                break;
            case 'h':
                hashFiles = 1;
                break;
            case 'p':
                createParents = 1; 
                break;
            case 'v':
                verbosity++;
                break;
            default:
                usage(EXIT_SUCCESS);
    }

    logDebug("verbosity: %d\n", verbosity);
    
    argc -= optind;
    argv += optind;

    source = argv[0];

    argc -= 1;
    argv += 1;

    if (0 == strcmp("-",source)) {
        logDebug("source is stdin\n");
        copyFile(source, argc, argv);
    }
    else if( 0 != (retval = stat(source,&sb))) {
        logFatal("Couldn't determine file type for %s: %s\n", source, strerror(errno));
    }
    else if (sb.st_mode & S_IFREG) {
        logDebug("source is a file: %s\n", source);
        copyFile(source, argc, argv);
    }
    else if (sb.st_mode & S_IFDIR) {
        logDebug("source is a directory: %s\n", source);
        copyDirectory(source, argc, argv);
    }
    else {
        logFatal("source is an unknown type: %s mode %x\n", 
            source, sb.st_mode);
    }
}

/* vim: set noet sw=5 ts=4: */
