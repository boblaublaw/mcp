#include <stdio.h>      // fopen
#include <assert.h>     // assert
#include <string.h>     // bzero, strerrno
#include <errno.h>      // errno


#include "mcp.h"

int initReader(mcp_reader_t *mr, char *filename)
{
    assert(filename);
    assert(mr);

    bzero(mr, sizeof(mcp_reader_t));
    if (NULL == (mr->source = fopen(filename,"r"))) {
        fprintf(stderr, "Could not open %s: %s\n", filename, strerror(errno));
        return -1;
    }

    // determine the size of the input file
    fseek(mr->source, 0L, SEEK_END);
    mr->size = ftell(mr->source);
    fseek(mr->source, 0L, SEEK_SET);

    return 0;
}

int startReader(mcp_reader_t *mr)
{
    assert(mr);
    assert(mr->source);



    fclose(mr->source);

    return 0;
}

/* vim: set noet sw=5 ts=4: */

