#include <stdio.h>
#include <pthread.h>
#define PAGESIZE                4096

#define MCP_MAX_WRITERS         32

typedef struct mcp_reader_t {
    char            *filename;
    long            size;
    FILE            *source;

    // reader thread
    pthread_t       thread;

    // read buffer and associated thread vars
    unsigned char   data[PAGESIZE];
    size_t          dataBytes;

    unsigned        writerStatus;
    pthread_mutex_t data_mutex;

    pthread_cond_t  dataFull_cv;
    pthread_cond_t  dataEmpty_cv;
} mcp_reader_t;

typedef struct mcp_writer_t {
    char            *filename;
    pthread_t       thread;
    unsigned        tid;
    int             forceOverwrite;
    mcp_reader_t    *mr;
    long            bytesWritten;
} mcp_writer_t;

// function prototypes:
void *startWriter(void *arg);
int initReader(mcp_reader_t *, char *);
int startReader(mcp_reader_t *);


/* vim: set noet sw=4 ts=4: */
