#!/bin/bash

# Make sure Skia is compiled.

if [[ $# -lt 1 ]]
then
    echo "Missing parameter: The name of the output file must be provided."
    exit 1
fi

if [[ $# -lt 2 ]]
then
    thread=""
else
    threads="-j $2"
fi

output=$(pwd)/$1

cd ../Linux/skia
make -s -f ../../Gui/make_skia.mk OUTPUT=$output $threads

