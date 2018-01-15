#!/bin/bash

# Make sure Cairo is compiled.

if [[ $# -lt 1 ]]
then
    echo "Missing parameter: The name of the output file shall be provided."
    exit 1
fi

if [[ $# -lt 2 ]]
then
    threads=""
else
    threads="-j $2"
fi

output=$(pwd)/$1
if [[ -f $output ]]
then
    # Nothing to do. If Cairo shall be rebuilt, we currently require the user to delete the output file.
    exit 0
fi

echo "Building Cairo to ${output}..."
cd ../Linux/cairo

# Configure TODO: It would be nice to not build the test suite...
echo "Configuring Cairo..."
./autogen.sh --enable-gl --enable-dynamic --disable-static 2>/dev/null || { echo "Configure failed."; exit 1; }

# Make
echo "Compiling Cairo..."
make $threads 2>/dev/null || { echo "Make failed."; exit 1; }
echo "Done!"

# Copy the output.
mkdir -p $(dirname $output)
cp src/.libs/libcairo.so $output || { echo "Copy failed."; exit 1; }

