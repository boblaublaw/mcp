#!/bin/sh

max_mb=100
max_writers=32

if [ $# -eq 2 ]; then
    size_in_mb=$1
    num_writers=$2
else
    size_in_mb=$((RANDOM % max_mb))
    num_writers=$((RANDOM % max_writers))
fi

size_in_kb=$((size_in_mb * 1024))
testdir=`mktemp -d ./testdir.XXXXXXX`
testsrc=$testdir/source
echo creating a $size_in_mb MB test file
dd if=/dev/zero of=$testsrc count=$size_in_kb bs=1024 2> /dev/null

echo copying test file to $num_writers destinations
x=0
w=""
echo
echo script time:
time while [ $x -lt $num_writers ]; do
    w="$w $testdir/$x"
    x=$((x+1))
    cp $testsrc $testdir/$x
done

echo
echo mcp time:
time ./mcp -f $testsrc $w

rm -rf "$testdir"

