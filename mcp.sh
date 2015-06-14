#!/bin/sh

srcdir=$1
shift

mkdir -v $@
finaldstdir=$1
shift

fifodir="/tmp/mcp/$$"
mkdir -p $fifodir

while [ ! -z $1 ]; do
    fifo=$fifodir/$1
    mkfifo $fifo
    tar -xf $fifo -C $1 &
    fifos="$fifo $fifos"
    shift
done

cmdstr="tar -C $srcdir -cf - . "
for fifo in $fifos; do
    cmdstr="$cmdstr | tee $fifo "
done
cmdstr="$cmdstr | tar xf - -C $finaldstdir"

eval $cmdstr

rm -rf $fifodir

