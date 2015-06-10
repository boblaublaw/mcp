# mcp
mcp is a cp variant specifically for copying a single source to multiple destinations.  This program was created for use on film sets, to copy huge camera video files from the camera media onto multiple harddrives simultaneously.  

Where copying to multiple destinations is required, mcp will outperform sequential invocations of cp by only reading the source files once.  The overall copy speed will be that of the slowest writer or reader.

For small files, cp will probably outperform mcp.

Additionally, with the "-h flag", md5 hashes can be generated as the files are copied based on the first (and only) reading of the source file.  The hash files are placed adjacent to each copied file by appending the suffix ".md5" in the desination directories.
