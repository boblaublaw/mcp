#include <errno.h>          // errno
#include <string.h>         // strerror
#include <sys/stat.h>       // stat
#include <stdlib.h>         // exit()
#include <unistd.h>         // getopt() 
#include <getopt.h>         // getopt()
#include "mcp.h"            // mcp_writer_t

static struct option longopts[] = {
    { "create-hashfiles",       no_argument,            NULL,           'h' },
    { NULL,                     0,                      NULL,           0 }
};

// global vars
int hashFiles, 
    createParents,
    forceOverwrite;

pthread_attr_t      attr;

void usage(long retval)
{
    logInfo("mcp usage:\n");
    logInfo("\n");
    logInfo("mcp <options> Source Destination1 [ DestinationN ] ...\n");
    logInfo("\n");
    logInfo("options:\n");
    logInfo("\t-f: force overwrite destination file\n");
    logInfo("\t-h: create hash files for every source file\n");
    logInfo("\t-p: create parent directories where needed\n");
    logInfo("\t-v: increase verbosity (vv, vvv, etc)\n");
    logInfo("\t-?: help (add -v for more help)\n");
    logInfo("\n");
    logDebug("Guidance:\n");
    logDebug("\tThe Source can be a file or directory.\n");
    logDebug("\n");
    logDebug("\tIf the Source is a file, the Destinations may also be\n\t\tfiles or directories.\n");
    logDebug("\n");
    logDebug("\tIf the Source is a file and the Destination is a directory,\n\t\t the source file name will be created in the \n\t\tDestination directory\n");
    logDebug("\n");
    logDebug("\tIf the Source is a directory, the Destinations must be \n\t\tdirectories.\n");
    logDebug("\n");
    logDebug("Examples:\n");
    logDebug("\tmcp sourceFile DestFile\n\t\tfile to file copy. (same as 'cp')\n");
    logDebug("\n");
    logDebug("\tmcp -h sourceFile DestFile\n\t\tfile to file copy with hashing\n");
    logDebug("\n");
    logDebug("\tmcp sourceFile DestDir           \n\t\tcreate sourceFile in DestDir\n");
    logDebug("\n");
    logDebug("\tmcp -p sourceFile DestDir           \n\t\tcreate sourceFile in DestDir, create DestDir if needed\n");
    logDebug("\n");
    logDebug("\tmcp sourceFile DestFile DestDir  \n\t\tboth at the same time\n");
    logDebug("\n");
    exit(retval);
}

int main(int argc, char **argv)
{
    char                *source;
    struct stat         sb;
    int                 streamCopy,
                        ch;
    long                retval, sz;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    forceOverwrite = 0;
    streamCopy=0;
    hashFiles = 0; 
    createParents = 0;
    retval = 0;
    int dousage=0;

    logInit(stderr);

    // parse arguments
    while ((ch = getopt_long(argc, argv, "fhpv?", longopts, NULL)) != -1)
        switch (ch) {
            case 'f':
                forceOverwrite = 1;
                break;
            case 'h':
                hashFiles = 1;
                break;
            case 'p':
                createParents = 1; 
                break;
            case 'v':
                logIncrementVerbosity();
                break;
            case '?':
                dousage=1; // do this later in case -v flags follow
                break;
            default:
                usage(EXIT_SUCCESS);
    }
    if (dousage)
        usage(0);

    argc -= optind;
    argv += optind;

    source = argv[0];

    argc -= 1;
    argv += 1;

    if ((argc > 32) || (argc < 1)) {
        logError("Wrong number of arguments\n");
        usage(-1);
    }

    if (0 == strcmp("-",source)) {
        logDebug("source is stdin\n");
        retval = copyFile(source, argc, argv, NULL);
    }
    else if( 0 != (retval = stat(source,&sb))) {
        logFatal("Couldn't determine file type for %s: %s\n", source, strerror(errno));
    }
    else if (sb.st_mode & S_IFREG) {
        logDebug("source is a file: %s\n", source);
        retval = copyFile(source, argc, argv, NULL);
    }
    else if (sb.st_mode & S_IFDIR) {
        logDebug("source is a directory: %s\n", source);
        retval = copyDirectory(source, argc, argv);
    }
    else {
        logFatal("source is an unknown type: %s mode %x\n", 
            source, sb.st_mode);
    }

    pthread_attr_destroy(&attr);
    exit(retval);
}

/* vim: set noet sw=5 ts=4: */
