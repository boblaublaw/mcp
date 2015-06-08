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

extern int verbosity;
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
                fprintf(stderr,"Could not create directory %s: %s\n",
                    tmp, strerror(errno));
                fflush(stderr);
                return retval;
            }
            *p = '/';
        }
    }
    if (-1 == (retval = mkdir(tmp, S_IRWXU))) {
        fprintf(stderr,"Could not create directory %s: %s\n",
            tmp, strerror(errno));
        fflush(stderr);
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

void *startWriter(void *arg)
{
    long                retval = 0;
    char                *parentDir = NULL;
    FILE                *stream = NULL;
    int                 i=0;

    assert (arg);
    mcp_writer_t *self = (mcp_writer_t *)arg;

    if (verbosity) {
        pthread_mutex_lock(&self->mr->debugLock);
        fprintf(stderr, "\twriter %d:  launching with target %s\n", 
           self->tid, self->filename);
        fflush(stderr);
        pthread_mutex_unlock(&self->mr->debugLock);
    }

evaluate_destination:
    if (exists(self->filename)) {
        if (isDir(self->filename)) {
            // if the destination is a directory, rename the
            // destination file to be the destination directory
            // concatenated with the source file name.
            if (verbosity) {
                fprintf(stderr,"renaming dest file from dir: %s and dest file:%s\n",
                        self->filename, self->mr->filename);
                fflush(stderr);
            }
            if (-1 != (retval = asprintf(&self->filename, "%s/%s", 
                    self->filename, self->mr->filename))) {
                goto evaluate_destination;
            }
            retval = -1;
            fprintf(stderr, "Could not create a new destination filename\n");
            fflush(stderr);
            exitFlag = 1;
            goto thread_exit;
        }
        if (!self->forceOverwrite) {
            retval=-1;
            fprintf(stderr, "File exists (-f to force): %s\n", self->filename);
            fflush(stderr);
            exitFlag = 1;
            goto thread_exit;
        }
        if (verbosity) {
            pthread_mutex_lock(&self->mr->debugLock);
            fprintf(stderr, "\twriter %d: overwriting destination file %s\n",
               self->tid, self->filename);
            fflush(stderr);
            pthread_mutex_unlock(&self->mr->debugLock);
        }
    }
    else {
        if (verbosity) {
            pthread_mutex_lock(&self->mr->debugLock);
            fprintf(stderr, "\twriter %d: destination file %s does not exist\n",
               self->tid, self->filename);
            fflush(stderr);
            pthread_mutex_unlock(&self->mr->debugLock);
        }

        // file doesn't exist, let's check the parent dir
        // unfortunately, dirname is not reentrant
        // so lets make a local copy of the parent dirname
        pthread_mutex_lock(&self->mr->nonReentrantLock);
        parentDir = strdup(dirname(self->filename));
        pthread_mutex_unlock(&self->mr->nonReentrantLock);

        if (NULL == parentDir) {
            retval = -1;
            fprintf(stderr, "Failed to determine dirname for %s: %s\n", self->filename, strerror(errno));
            fflush(stderr);
            exitFlag = 1;
            goto thread_exit;
        }

        if (!exists(parentDir)) {
            if (createParents) {
                if (verbosity) {
                    pthread_mutex_lock(&self->mr->debugLock);
                    fprintf(stderr, "\twriter %d: creating missing parent dir: %s\n",
                       self->tid, parentDir);
                    fflush(stderr);
                    pthread_mutex_unlock(&self->mr->debugLock);
                }
                if (-1 == (retval = _mkdir(parentDir))) {
                    fprintf(stderr, "Failed to create parent directory %s: %s\n", 
                        parentDir, strerror(errno));
                    fflush(stderr);
                    exitFlag = 1;
                    goto thread_exit;
                }
                if (exists(parentDir)) {
                    if (verbosity) {
                        pthread_mutex_lock(&self->mr->debugLock);
                        fprintf(stderr, "\twriter %d:  created missing parent dir: %s\n",
                           self->tid, parentDir);
                        fflush(stderr);
                        pthread_mutex_unlock(&self->mr->debugLock);
                    }
                }
                else {
                    retval = -1;
                    fprintf(stderr, "failed to find new parent directory %s: %s\n", 
                        parentDir, strerror(errno));
                    fflush(stderr);
                    exitFlag = 1;
                    goto thread_exit;
                }
            }
            else {
                retval = -1;
                fprintf(stderr, "destination directory %s does not exist, -p to create parents\n",
                    parentDir);
                fflush(stderr);
                exitFlag = 1;
            }
        }
    }

    if (verbosity) {
        fprintf(stderr,"\twriter %d opening %s for writing\n", 
            self->tid, self->filename);
        fflush(stderr);
    }

    if (NULL == (stream = fopen (self->filename, "w+"))) {
        fprintf(stderr, "writer %d could not open %s for writing: %s\n", 
            self->tid, self->filename, strerror(errno));
        retval = -1;
        exitFlag = 1;
        goto thread_exit;
    }
   
    while (1) {
        if (verbosity) {
            pthread_mutex_lock(&self->mr->debugLock);
            fprintf (stderr, "\twriter %d about to wait for BUF A barrier\n", self->tid);
            fflush(stderr);
            pthread_mutex_unlock(&self->mr->debugLock);
        }

        if (-1 == (retval = pthread_barrier_waitcancel(&self->mr->barrier[BUF_A], &exitFlag))) {
            if (!exitFlag) {
                fprintf(stderr, "writer %d: failed to wait for BUF A barrier: %ld\n", 
                    self->tid, retval);
                fflush(stderr);
            }
            
            exitFlag = 1;
            goto thread_exit;
        }

        if (retval == ETIMEDOUT) {
            if (!exitFlag) {
                fprintf(stderr, "writer %d: timed out waiting for BUF A barrier\n", self->tid);
                fflush(stderr);
            }
            exitFlag = 1;
            goto thread_exit;
        }

        if (verbosity) {
            pthread_mutex_lock(&self->mr->debugLock);
            fprintf (stderr, "\twriter %d done waiting for BUF A barrier\n", self->tid);
            fflush(stderr);
            pthread_mutex_unlock(&self->mr->debugLock);
        }

        // ========================= A BUFFER WRITE, B BUFFER READ START====================
        
        if (exitFlag) {
            if (verbosity) {
                pthread_mutex_lock(&self->mr->debugLock);
                fprintf (stderr, "\twriter %d told to shut down\n", self->tid);
                fflush(stderr);
                pthread_mutex_unlock(&self->mr->debugLock);
            }
            break;
        }

        if (self->mr->bufBytes[BUF_A] == 0 ) {
            if (verbosity) {
                pthread_mutex_lock(&self->mr->debugLock);
                fprintf (stderr, "\twriter %d has no more data\n", self->tid);
                fflush(stderr);
                pthread_mutex_unlock(&self->mr->debugLock);
            }
            break;
        }

        if (self->mr->bufBytes[BUF_A] != fwrite(self->mr->buf[BUF_A], 1, self->mr->bufBytes[BUF_A], stream)) {
            retval=-1;
            exitFlag = 1;
            fprintf (stderr, "writer %d failed to write %ld bytes from BUF A\n", 
                self->tid, self->mr->bufBytes[BUF_A]);
            fflush(stderr);
            goto thread_exit;
        }

        if (verbosity) {
            pthread_mutex_lock(&self->mr->debugLock);
            fprintf( stderr, "\twriter %d wrote %ld bytes from BUF A\n", self->tid, self->mr->bufBytes[BUF_A]);
            fflush(stderr);
            pthread_mutex_unlock(&self->mr->debugLock);
        }

        // ========================= A BUFFER WRITE, B BUFFER READ END======================

        if (verbosity) {
            pthread_mutex_lock(&self->mr->debugLock);
            fprintf (stderr, "\twriter %d about to wait for BUF B barrier\n", self->tid);
            fflush(stderr);
            pthread_mutex_unlock(&self->mr->debugLock);
        }

        if (-1 == (retval = pthread_barrier_waitcancel(&self->mr->barrier[BUF_B], &exitFlag))) {
            if (!exitFlag) {
                fprintf(stderr, "writer %d: failed to wait for BUF B barrier: %ld\n", 
                    self->tid, retval);
                fflush(stderr);
            }
            
            exitFlag = 1;
            goto thread_exit;
        }

        if (retval == ETIMEDOUT) {
            fprintf(stderr, "writer %d: timed out waiting for BUF B barrier\n", self->tid);
            fflush(stderr);
            exitFlag = 1;
            goto thread_exit;
        }

        if (verbosity) {
            pthread_mutex_lock(&self->mr->debugLock);
            fprintf (stderr, "\twriter %d done waiting for BUF B barrier\n", self->tid);
            fflush(stderr);
            pthread_mutex_unlock(&self->mr->debugLock);
        }

        // ========================= A BUFFER READ, B BUFFER WRITE START====================
        
        if (exitFlag) {
            if (verbosity) {
                pthread_mutex_lock(&self->mr->debugLock);
                fprintf (stderr, "\twriter %d told to shut down\n", self->tid);
                fflush(stderr);
                pthread_mutex_unlock(&self->mr->debugLock);
            }
            break;
        }

        if (self->mr->bufBytes[BUF_B] == 0 ) {
            if (verbosity) {
                pthread_mutex_lock(&self->mr->debugLock);
                fprintf (stderr, "\twriter %d has no more data\n", self->tid);
                fflush(stderr);
                pthread_mutex_unlock(&self->mr->debugLock);
            }
            break;
        }

        if (self->mr->bufBytes[BUF_B] != fwrite(self->mr->buf[BUF_B], 1, self->mr->bufBytes[BUF_B], stream)) {
            fprintf (stderr, "writer %d failed to write %ld bytes from BUF B\n", 
                self->tid, self->mr->bufBytes[BUF_A]);
            fflush(stderr);
            retval = -1;
            exitFlag = 1;
            goto thread_exit;
        }

        if (verbosity) {
            pthread_mutex_lock(&self->mr->debugLock);
            fprintf( stderr, "\twriter %d wrote %ld bytes from BUF B\n", self->tid, self->mr->bufBytes[BUF_B]);
            fflush(stderr);
            pthread_mutex_unlock(&self->mr->debugLock);
        }

        // ========================= A BUFFER WRITE, B BUFFER READ END======================
    }

thread_exit:
    if (stream) {
        fclose(stream);
        stream=NULL;
    }
    if (parentDir) {
        free(parentDir);
        parentDir=NULL;
    }
    if (verbosity) {
        fprintf( stderr, "\twriter %d exiting with status %ld\n", 
            self->tid, retval);
        fflush(stderr);
    }
    pthread_exit((void*) retval);
}

/* vim: set noet sw=5 ts=4: */
