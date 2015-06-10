# mcp
mcp is a cp variant specifically for copying a single source to multiple destinations.  This program was originally created for use on film sets, to copy huge camera video files from the camera media onto multiple hard drives simultaneously.  

Where copying to multiple destinations is required, mcp will outperform sequential invocations of cp by only reading the source files once.  The overall copy speed will be that of the slowest writer or reader.

For small files, cp will probably outperform mcp.

Additionally, with the "-h flag", md5 hashes can be generated as the files are copied based on the first (and only) reading of the source file.  The hash files are placed adjacent to each copied file by appending the suffix ".md5" in the desination directories.

```
mcp usage:

mcp <options> Source Destination1 [ DestinationN ] ...
	The Source can be a file or directory.
	If the Source is a file, the Destinations may also be files or directories.
	If the Source is a directory, the Destinations must be directories.

options:
	-f: force overwrite destination file
	-h: create hash files for every source file
	-p: create parent directories where needed
	-v: increase verbosity (vv, vvv, etc)

Examples:
	mcp sourceFile DestFile1 DestFile2
	mcp sourceFile DestFile1 DestFile2
	mcp sourceFile DestFile1 DestFile2
```
 
Build has only been tested on OS 10.9.5
