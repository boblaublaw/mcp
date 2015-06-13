#!/bin/sh

srcdir=mcp

rm -rf dst1dir dst2dir dst3dir

mkdir dst1dir dst2dir dst3dir

mkfifo tmp1
mkfifo tmp2

tar -xf tmp1 -C dst1dir &
tar -xf tmp2 -C dst2dir &

tar -C $srcdir -cf - . | tee tmp1 | tee tmp2 | tar xf - -C dst3dir
fg;fg
rm tmp1 tmp2

