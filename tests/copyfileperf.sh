#!/bin/sh

max_mb=100
max_writers=32
if [ $# -eq 2 ]; then
    size_in_mb=$1
    num_writers=$2
else
    size_in_mb=$((((RANDOM % max_mb)) + 1 ))
    num_writers=$((((RANDOM % max_writers)) + 1 ))
fi

size_in_kb=$((size_in_mb * 1024))
testdir=`mktemp -d ./testdir.XXXXXXX`
testsrc=$testdir/0
echo creating a $size_in_mb MB test file
time dd if=/dev/urandom count=$size_in_kb bs=1024 2> /dev/null | pv -s $((size_in_kb * 1024)) > $testsrc

echo copying test file to $num_writers destinations
x=1
w=""
echo script copy time:
time while [ $x -le $num_writers ]; do
    w="$w $testdir/$x"
    cp $testsrc $testdir/$x
    x=$((x+1))
done

echo
echo mcp time:
cat $testsrc | pv -s `stat -f %z $testsrc` | time ../mcp -hf - $w
rm -rf "$testdir"

