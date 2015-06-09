#include <stdio.h>
#include <pthread.h>
#include <CommonCrypto/CommonDigest.h>

#include "log.h"
#include "copyfile.h"

#ifdef __APPLE__
#include "pthread_barrier.h"
#endif
#include "pthread_barrier_ext.h"

#define PAGESIZE                4096 
#define BUFSIZE                 64 * PAGESIZE
#define NUMBUF                  2

#define BUF_A                   0
#define BUF_B                   1


#define MCP_MAX_WRITERS         32

typedef struct mcp_reader_t {
    const char          *filename;
    FILE                *source;

    // reader thread
    pthread_t           thread;

    // read buffer and associated thread vars
    unsigned char       buf[NUMBUF][BUFSIZE];
    size_t              bufBytes[NUMBUF];

    // thread stuff
    pthread_barrier_t   barrier[2];
    // for functions that are not reentrant
    pthread_mutex_t     nonReentrantLock; 

    // MD5 
    unsigned char       md5sum[CC_MD5_DIGEST_LENGTH];
    CC_MD5_CTX          md5state;
    int                 hashFiles;
} mcp_reader_t;

typedef struct mcp_writer_t {
    char                *filename;
    char                *desc;
    pthread_t           thread;
    unsigned            tid;
    int                 forceOverwrite;
    mcp_reader_t        *mr;
    FILE                *stream;
} mcp_writer_t;

// function prototypes:
void *startWriter(void *);
int initReader(mcp_reader_t *mr, const char *filename, int count, int hashFiles);
void *startReader(void *);
int writeHashFile(const char *basename, unsigned char *md5sum);

/* vim: set noet sw=4 ts=4: */
