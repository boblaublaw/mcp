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

function verify()
{
    pushd $1 > /dev/null 2>&1
    count=$2
    
    x=1
    while [ $x -le $count ]; do
        md5 $x
        if [ -f "$x.md5" ] ; then 
            cat $x.md5  
        fi
        cmp 0 $x
        if [ $? -ne 0 ]; then
            echo COMPARISON FAILURE!
            exit 1
        fi
        x=$((x+1))
    done
    echo test successful.
    popd > /dev/null 2>&1
}

echo copying test file to $num_writers destinations
x=1
w=""
echo
echo script hash time:
time md5 $testsrc 
echo script copy time:
time while [ $x -le $num_writers ]; do
    w="$w $testdir/$x"
    cp $testsrc $testdir/$x
    x=$((x+1))
done

echo
echo mcp time:
cat $testsrc | pv -s `stat -f %z $testsrc` | time ./mcp -hf - $w
verify $testdir $num_writers
rm -rf "$testdir"

