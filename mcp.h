#include <stdio.h>

#define MCP_MAX_WRITERS 32

typedef struct mcp_writer_t {
    char            *filename;
    pthread_t       thread;
    unsigned        tid;
    int             forceOverwrite;
} mcp_writer_t;

void *startWriter(void *arg);
int startReader(FILE *);


/* vim: set noet sw=4 ts=4: */
