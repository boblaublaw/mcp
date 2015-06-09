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
    # create a source file hash
    md5 $1/0 | cut -f2 -d\= | cut -c2- > $1/0.md5
    # check source and dest file hashes
    ./checkmd5.sh $1 && return
    echo COPYFILE FAILED
    exit 1
}

echo copying test file to $num_writers destinations
x=1
w=""
while [ $x -le $num_writers ]; do
    w="$w $testdir/$x"
    x=$((x+1))
done

echo
cat $testsrc | pv -s `stat -f %z $testsrc` | time ../mcp -hf - $w
verify $testdir $num_writers
rm -rf "$testdir"

