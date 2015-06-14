#!/bin/sh

function copyfile()
{
    srcfile=$1
    shift

    finalfile=$1
    shift

    cmdstr="cat $srcfile "
    while [ ! -z $1 ]; do
        cmdstr=" $cmdstr | tee $1 "
        shift
    done
    cmdstr="$cmdstr > $finalfile"

    eval $cmdstr
}

function copydir() 
{
    srcdir=$1
    shift

    mkdir -p $@
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
}

# main
if [ -d "$1" ]; then
    copydir $@
elif [ -f "$1" ]; then
    copyfile $@
else
    echo I dont know what \"$1\" is.
fi

