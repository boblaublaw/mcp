#include <stdio.h>
#include <pthread.h>

#define MCP_MAX_WRITERS 32

typedef struct mcp_reader_t {
    char            *filename;
    long            size;
    FILE            *source;
    pthread_t       thread;
} mcp_reader_t;

typedef struct mcp_writer_t {
    char            *filename;
    pthread_t       thread;
    unsigned        tid;
    int             forceOverwrite;
    long            finalSize;
    long            bytesWritten;
} mcp_writer_t;

// function prototypes:
void *startWriter(void *arg);
int initReader(mcp_reader_t *, char *);
int startReader(mcp_reader_t *);


/* vim: set noet sw=4 ts=4: */
