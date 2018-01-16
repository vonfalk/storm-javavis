#!/bin/bash

# Copy libraries required by an .so-file to the target path.
if [[ $# -lt 3 ]]
then
    echo "Not enough arguments supplied."
    echo "Expected <.so> <target> <library...>"
    exit 1
fi

so=$1
shift
outdir=$1
shift

# Clean up old files.
for i in $(find $outdir -name "lib*.so*")
do
    rm $i
done

all=$(ldd $so)

for i in $@
do
    lib=$(echo "$all" | grep $i | sed -E 's/^.*=> (.*) \(0x[0-9A-Fa-f]+\)$/\1/')
    cp $lib $outdir/
done
