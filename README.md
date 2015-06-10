# mcp
mcp is a cp variant specifically for copying a single source to multiple destinations.  This program was originally created for use on film sets, to copy huge camera video files from the camera media onto multiple hard drives simultaneously.  

Where copying to multiple destinations is required, mcp will outperform sequential invocations of cp by only reading the source files once.  The overall copy speed will be that of the slowest writer or reader.

For small files, cp will probably outperform mcp.

Additionally, with the "-h flag", md5 hashes can be generated as the files are copied based on the first (and only) reading of the source file.  The hash files are placed adjacent to each copied file by appending the suffix ".md5" in the desination directories.

```
mcp usage:

mcp <options> Source Destination1 [ DestinationN ] ...

options:
    -f: force overwrite destination file
    -h: create hash files for every source file
    -p: create parent directories where needed
    -v: increase verbosity (vv, vvv, etc)
    -?: help (add -v for more help)

Guidance:
    The Source can be a file or directory.

    If the Source is a file, the Destinations may also be
        files or directories.

    If the Source is a file and the Destination is a directory,
         the source file name will be created in the 
        Destination directory

    If the Source is a directory, the Destinations must be 
        directories.

Examples:
    mcp sourceFile DestFile
        file to file copy. (same as 'cp')

    mcp -h sourceFile DestFile
        file to file copy with hashing

    mcp sourceFile DestDir           
        create sourceFile in DestDir

    mcp -p sourceFile DestDir           
        create sourceFile in DestDir, create DestDir if needed

    mcp sourceFile DestFile DestDir  
        both at the same time
```
 
Build has only been tested on OS 10.9.5
