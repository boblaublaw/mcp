#include <stdio.h>
#include <pthread.h>
#include <CommonCrypto/CommonDigest.h>

#ifdef __APPLE__
#include "pthread_barrier.h"
#endif

#define PAGESIZE                4096
#define BUFSIZE                 1024 * PAGESIZE
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
    unsigned char       buf[NUMBUF][BUFSIZE];
    size_t              bufBytes[NUMBUF];

    pthread_barrier_t   readBarrier;
    pthread_barrier_t   writeBarrier;
    pthread_mutex_t     debugLock;

    // MD5 
    unsigned char       md5sum[CC_MD5_DIGEST_LENGTH];
    CC_MD5_CTX          md5state;
    int                 hashFiles;
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
void *startWriter(void *);
int initReader(mcp_reader_t *, char *, int, int);
void *startReader(void *);


/* vim: set noet sw=4 ts=4: */
