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

function verify()
{
    pushd $1
    count=$2
    
    x=0
    while [ $x -lt $count ]; do
        x=$((x+1))
        cmp source $x
        if [ $? -ne 0 ]; then
            echo COMPARISON FAILURE!
            exit 1
        fi
    done
    popd
}

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
echo mcp1 time:
time ./mcp1 -f $testsrc $w
verify $testdir $num_writers

echo
echo mcp2 time:
time ./mcp2 -f $testsrc $w
exit 0
rm -rf "$testdir"
verify $testdir $num_writers

