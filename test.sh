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
testsrc=$testdir/source
echo creating a $size_in_mb MB test file
time dd if=/dev/urandom of=$testsrc count=$size_in_kb bs=1024 2> /dev/null

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
echo script hash time:
time md5 $testsrc 
echo script copy time:
time while [ $x -lt $num_writers ]; do
    w="$w $testdir/$x"
    x=$((x+1))
    cp $testsrc $testdir/$x
done

echo
echo mcp time:
echo time ./mcp -h -f $testsrc $w
time ./mcp -f $testsrc $w
verify $testdir $num_writers
rm -rf "$testdir"

