include <stdio.h>  
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

void usage(long retval)
{
    fprintf(stderr,"usage()\n");
    exit(retval);
}

int startReader(FILE *source) 
{
    assert(source);

    fclose(source);

    return 0;
}

int main(int argc, char **argv)
{
    int                 hashFiles, 
                        forceOverwrite,
                        ch;
    long                retval;
    mcp_writer_t        writers[MCP_MAX_WRITERS];
    unsigned            numWriters, writerIndex;
    pthread_attr_t      attr;
    void                *thread_status;
    FILE                *source;

    forceOverwrite = 0;
    source=NULL;
    hashFiles = 0;
    numWriters = 0;
    retval = 0;
    bzero(writers, sizeof(writers));
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // parse arguments
    while ((ch = getopt_long(argc, argv, "fh", longopts, NULL)) != -1)
        switch (ch) {
            case 'f':
                forceOverwrite = 1;
                break;
            case 'h':
                hashFiles = 1;
                break;
            // TODO -p arg to create parent directories on targets
            default:
                usage(EXIT_SUCCESS);
    }
    argc -= optind;
    argv += optind;

    if ((argc < 2) || (argc > 33))
        usage(EXIT_FAILURE);
     
    if (NULL == (source = fopen(argv[0],"r"))) {
        fprintf(stderr, "Could not open %s: %s\n", argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    }
    argc -= 1;
    argv += 1;

    // launch all writers
    while (argc) {
        writers[numWriters].filename = strdup(argv[0]);
        writers[numWriters].tid = numWriters;
        writers[numWriters].forceOverwrite = forceOverwrite;
        retval = pthread_create(&writers[numWriters].thread, &attr, startWriter, 
            (void *)&writers[numWriters]);
        if (retval) {
            printf("ERROR; return code from pthread_create() is %ld\n", retval);
            exit(EXIT_FAILURE);
        }
        argc -= 1;
        argv += 1;
        numWriters += 1;
    }

    retval = startReader(source);

    // wait for all writers to exit
    for (writerIndex=0; writerIndex < numWriters; writerIndex++) {
        retval |= pthread_join(writers[writerIndex].thread, &thread_status);
        if (retval) {
            printf("ERROR; return code from pthread_join() is %ld\n", retval);
            exit(EXIT_FAILURE);
        }
        printf("Main: completed join with thread %d having a status of %ld\n",
            writers[writerIndex].tid,(long)thread_status);
        retval |= (long)thread_status;
    }

    if (retval)
        exit(EXIT_FAILURE);

    exit(EXIT_SUCCESS);
}

/* vim: set noet sw=4 ts=4: */
