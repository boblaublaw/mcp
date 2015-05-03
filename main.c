#include <stdio.h>          // printf, etc
#include <stdlib.h>         // exit()
#include <unistd.h>         // getopt() 
#include <getopt.h>         // getopt()
#include <string.h>         // strdup()
#include <pthread.h>        // pthread_*
#include <assert.h>         // assert
#include <string.h>         // strerror
#include <errno.h>          // errno
#include "mcp.h"            // mcp_writer_t

static struct option longopts[] = {
    { "create-hashfiles",       no_argument,            NULL,           'h' },
    { NULL,                     0,                      NULL,           0 }
};

static int debugLevel;

mcp_reader_t reader;

int
writeHashFile(char *basename, unsigned char *md5sum)
{
    char *hashFileName;
    FILE *hashFile;
    int i;
    
    if (-1 == asprintf(&hashFileName, "%s.md5", basename)) {
        return -1;
    }
    // XXX should i check for existing hash files?
    if (NULL == (hashFile = fopen (hashFileName, "w+"))) {
        return -1;
    }
    for (i=0; i<CC_MD5_DIGEST_LENGTH; i++)
        fprintf(hashFile, "%02x", md5sum[i]);
    fprintf(hashFile, "\n");
    fclose(hashFile);
    return 0;
}


void usage(long retval)
{
    fprintf(stderr,"usage()\n");
    exit(retval);
}

int main(int argc, char **argv)
{
    int                 hashFiles, 
                        createParents,
                        forceOverwrite,
                        ch;
    long                retval, sz;
    mcp_writer_t        writers[MCP_MAX_WRITERS];
    unsigned            numWriters, writerIndex;
    pthread_attr_t      attr;
    void                *thread_status;

    forceOverwrite = 0;
    hashFiles = 0; 
    createParents = 0;
    numWriters = 0;
    debugLevel=0;
    retval = 0;
    bzero(writers, sizeof(writers));
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // parse arguments
    while ((ch = getopt_long(argc, argv, "dfh", longopts, NULL)) != -1)
        switch (ch) {
            case 'd':
                debugLevel = 1;
                break;
            case 'f':
                forceOverwrite = 1;
                break;
            case 'h':
                hashFiles = 1; // TODO
                break;
            case 'p':
                createParents = 1; // TODO
                break;
            default:
                usage(EXIT_SUCCESS);
    }
    argc -= optind;
    argv += optind;

    if ((argc < 2) || (argc > 33))
        usage(EXIT_FAILURE);
     
    // third argument is number of writers plus readers
    // (or writers + 1 )
    if (-1 == initReader(&reader, argv[0], argc, hashFiles)) {
        exit(EXIT_FAILURE);
    }
    
    argc -= 1;
    argv += 1;

    // launch all writers
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
            goto exit;
        }
        argc -= 1;
        argv += 1;
        numWriters += 1;
    }

    if (0 != (retval = pthread_create(&reader.thread, &attr, startReader, (void *)&reader))) {
            printf("ERROR; return code from pthread_create() is %ld\n", retval);
            goto exit;
    }

    // wait for the reader to exit
    if (-1 == pthread_join(reader.thread, &thread_status)) {
        printf("ERROR: pthread_join(): %d (%s)", errno, strerror(errno));
        retval = -1;
        goto exit;
    }

    // wait for all writers to exit
    for (writerIndex=0; writerIndex < numWriters; writerIndex++) {
        if (-1 == pthread_join(writers[writerIndex].thread, &thread_status)) {
            printf("ERROR: pthread_join(): %d (%s)", errno, strerror(errno));
            retval = -1;
        }
        if (retval == 0) {
            //printf("Main: completed join with thread %d having a status of %ld\n",
            //    writers[writerIndex].tid,(long)thread_status);
        }
        retval = (long)thread_status;
    }

    // write all the hash files, if requested
    if (hashFiles) {
        for (writerIndex=0; writerIndex < numWriters; writerIndex++) {
            mcp_writer_t *mw = &writers[writerIndex];
            writeHashFile(mw->filename, mw->mr->md5sum);
        }
    }

exit:
    pthread_attr_destroy(&attr);
    exit(retval);
}

/* vim: set noet sw=5 ts=4: */
