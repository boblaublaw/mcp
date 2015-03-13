#include <stdio.h>
#include <pthread.h>

#ifdef __APPLE__
#include "pthread_barrier.h"
#endif

#define PAGESIZE                4096 
#define BUFSIZE                 100 * PAGESIZE
#define NUMBUF                  2

#define BUF_A                   0
#define BUF_B                   1


#define MCP_MAX_WRITERS         32

typedef struct mcp_reader_t {
    char                *filename;
    long                size;
    FILE                *source;

    // reader thread
    pthread_t           thread;

    // read buffer and associated thread vars
    unsigned char       buf[BUFSIZE][NUMBUF];
    size_t              bufBytes[NUMBUF];

    pthread_barrier_t   readBarrier;
    pthread_barrier_t   writeBarrier;

} mcp_reader_t;

typedef struct mcp_writer_t {
    char                *filename;
    pthread_t           thread;
    unsigned            tid;
    int                 forceOverwrite;
    mcp_reader_t        *mr;
    long                bytesWritten;
} mcp_writer_t;

// function prototypes:
void *startWriter(void *arg);
int initReader(mcp_reader_t *, char *, int);
int startReader(mcp_reader_t *);


/* vim: set noet sw=4 ts=4: */
