#!/bin/sh

[ "$cc" = "" ] && cc=cc
[ "$cflags" = "" ] && cflags="-ansi -Wall -Wextra -Wpedantic"
[ "$ldflags" = "" ] && ldflags=""
[ "$bdir" = "" ] && bdir=build

mkdir -p $bdir

l=""
for i in $(find ../src -type f -name "*.c"); do
    o=$bdir/$i.o

    mkdir -p $(dirname $o)

    $cc -c $i -o $o $cflags
    if [ $? -ne 0 ]; then
        echo "-- Build failed!"
        exit 1
    fi

    l="$l $o"
done

$cc $l -o ocr $ldflags
if [ $? -ne 0 ]; then
    echo "-- Build failed!"
    exit 1
fi
