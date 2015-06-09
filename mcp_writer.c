#include <stdio.h>      // printf
#include <pthread.h>    // pthread_*
#include <unistd.h>     // sleep()
#include <stdlib.h>     // free()
#include <string.h>     // strerrno()
#include <errno.h>      // errno
#include <sys/stat.h>   // stat()
#include <assert.h>     // assert()
#include <limits.h>     // PATH_MAX
#include <libgen.h>     // dirname()
#include "mcp.h"

extern int exitFlag;
extern int createParents;

int _mkdir(const char *dir) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;
    int retval;

    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if(tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    for(p = tmp + 1; *p; p++) {
        if(*p == '/') {
            *p = 0;
            if (-1 == (retval = mkdir(tmp, S_IRWXU))) {
                logError("Could not create directory %s: %s\n",
                    tmp, strerror(errno));
                return retval;
            }
            *p = '/';
        }
    }
    if (-1 == (retval = mkdir(tmp, S_IRWXU))) {
        logError("Could not create directory %s: %s\n",
            tmp, strerror(errno));
    }
    return 0;
}

int
writeHashFile(const char *basename, unsigned char *md5sum)
{
    char *hashFileName;
    FILE *hashFile;
    int i;

    if (-1 == asprintf(&hashFileName, "%s.md5", basename)) {
        return -1;
    }
    if (NULL == (hashFile = fopen (hashFileName, "w+"))) {
        return -1;
    }
    for (i=0; i<CC_MD5_DIGEST_LENGTH; i++)
        fprintf(hashFile, "%02x", md5sum[i]);
        fprintf(hashFile, "\n");
        fclose(hashFile);
    return 0;
}

int isDir(const char *filename)
{
    struct stat         sb;

    bzero(&sb,sizeof(sb));
    if ((0 == stat(filename,&sb)) && (sb.st_mode & S_IFDIR)) 
        return 1;
    return 0;
}

int exists(const char *filename)
{
    struct stat         sb;

    if (0 == stat(filename,&sb)) 
        return 1;
    return 0;
}
       
int writeFromBuf(mcp_writer_t *self, int bufId) 
{
    int retval;
    logDebug("%s about to wait for BUF %d barrier\n", self->desc, bufId);

    if (-1 == pthread_barrier_waitcancel(&self->mr->barrier[bufId], &exitFlag, self->desc)) {
        logError("%s: something went horribly wrong with pthread_barrier_waitcancel\n", 
            self->desc);
        exitFlag = 1;
        return(-1);
    }

    logDebug("%s done waiting for BUF %d barrier\n", self->desc, bufId);

    if (exitFlag) {
        logDebug("%s told to shut down\n", self->desc);
        return(0);
    }

    logDebug("%s has %d more bytes to write\n", self->desc, self->mr->bufBytes[bufId]);
    if (self->mr->bufBytes[bufId] == 0 ) {
        logDebug("%s has no more data\n", self->desc);
        return(0);
    }

    if (self->mr->bufBytes[bufId] != fwrite(self->mr->buf[bufId], 1, self->mr->bufBytes[bufId], self->stream)) {
        exitFlag = 1;
        logError("%s failed to write %ld bytes from BUF %d\n", 
            self->desc, self->mr->bufBytes[bufId], bufId);
        return(-1);
    }
    logDebug("%s wrote %ld bytes from BUF %d\n", self->desc, self->mr->bufBytes[bufId], bufId);
    return(1);
}

void *startWriter(void *arg)
{
    long                retval = 0;
    char                *parentDir = NULL;
    int                 i=0;
    int                 bufId = 0;

    assert (arg);
    mcp_writer_t *self = (mcp_writer_t *)arg;
    
    // craft a little barnacle to stick onto the front of our log messages
    asprintf(&self->desc, "\t\t\t\t\t\twriter %d", self->tid);

    logDebug("%s launching with target %s\n", self->desc, self->filename);

evaluate_destination:
    if (exists(self->filename)) {
        if (isDir(self->filename)) {
            // if the destination is a directory, rename the
            // destination file to be the destination directory
            // concatenated with the source file name.
            logDebug("%s renaming dest file from dir: %s and dest file:%s\n", self->desc,
                    self->filename, self->mr->filename);
            if (-1 != (retval = asprintf(&self->filename, "%s/%s", 
                    self->filename, self->mr->filename))) {
                goto evaluate_destination;
            }
            retval = -1;
            logError("Could not create a new destination filename\n");
            exitFlag = 1;
            goto thread_exit;
        }
        if (!self->forceOverwrite) {
            retval=-1;
            logError("File exists (-f to force): %s\n", self->filename);
            exitFlag = 1;
            goto thread_exit;
        }
        logDebug("%s overwriting destination file %s\n", self->desc, self->filename);
    }
    else {
        logDebug("%s destination file %s does not exist\n", self->desc, self->filename);

        // file doesn't exist, let's check the parent dir
        // unfortunately, dirname is not reentrant
        // so lets make a local copy of the parent dirname
        pthread_mutex_lock(&self->mr->nonReentrantLock);
        parentDir = strdup(dirname(self->filename));
        pthread_mutex_unlock(&self->mr->nonReentrantLock);

        if (NULL == parentDir) {
            retval = -1;
            logError("Failed to determine dirname for %s: %s\n", self->filename, strerror(errno));
            exitFlag = 1;
            goto thread_exit;
        }

        if (!exists(parentDir)) {
            if (createParents) {
                logDebug("%s creating missing parent dir: %s\n", self->desc, parentDir);
                if (-1 == (retval = _mkdir(parentDir))) {
                    logError("Failed to create parent directory %s: %s\n", 
                        parentDir, strerror(errno));
                    exitFlag = 1;
                    goto thread_exit;
                }
                if (exists(parentDir)) {
                    logDebug("%s created missing parent dir: %s\n", self->desc, parentDir);
                }
                else {
                    retval = -1;
                    logError("failed to find new parent directory %s: %s\n", 
                        parentDir, strerror(errno));
                    exitFlag = 1;
                    goto thread_exit;
                }
            }
            else {
                retval = -1;
                logError("destination directory %s does not exist, -p to create parents\n",
                    parentDir);
                exitFlag = 1;
            }
        }
    }

    logDebug("%s opening %s for writing\n",  self->desc, self->filename);

    if (NULL == (self->stream = fopen (self->filename, "w+"))) {
        logError("%s could not open %s for writing: %s\n", 
            self->desc, self->filename, strerror(errno));
        retval = -1;
        exitFlag = 1;
        goto thread_exit;
    }

    // so long as we believe there is more data, keep 
    // writing from the next buffer to the stream
    while (1 == (retval = writeFromBuf(self, bufId))) 
        bufId = !bufId;

thread_exit:
    if (self->stream) {
        fclose(self->stream);
        self->stream=NULL;
    }
    if (parentDir) {
        free(parentDir);
        parentDir=NULL;
    }
    logDebug("%s exiting with status %ld\n", self->desc, retval);
    pthread_exit((void*) retval);
}

/* vim: set noet sw=5 ts=4: */
