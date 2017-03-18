#!/bin/bash

echo "Building release..."
mm release -ne || { echo "Compilation failed!"; exit 1; }

echo "Done! Running the test suite..."
release/CodeTest.exe || { echo "Tests failed!"; exit 1; }
release/StormTest.exe || { echo "Tests failed!"; exit 1; }

echo "Seems good"

echo "Creating zip-file..."
if [[ -e release/storm.zip ]]
then
    rm release/storm.zip
fi

7z a release/storm.zip ./release/Storm.exe doc/ > /dev/null
find root/ -name "*.bs" -or -name "*.bnf" -or -name "*.txt" -or -name "*.png" \
  | xargs --delimiter='\n' 7z a release/storm.zip > /dev/null

# Add dlls, we need to rename them first...
IFS=$'\n'
for j in `find root/ -name "Release*.dll"`
do
    name=`echo $j | sed 's/Release//g'`
    cp $j $name
    7z a Release/storm.zip $name > /dev/null
    rm $name
done

echo "Checking so that the release works..."
if [[ -e release/storm ]]
then
    rm -r release/storm/*
else
    mkdir release/storm
fi

7z x release/storm.zip -orelease/storm/ > /dev/null
echo "exit" | release/storm/Storm.exe > /dev/null
if [[ $? -ne 0 ]]
then
    echo "Failed to launch the repl. Something is bad! Test in release/storm"
    exit 1
fi

release/storm/Storm.exe -c 'test:bf:inlineBf' > release/storm/output.txt
if [[ $? -ne 0 ]]
then
    echo "Failed to execute BS code. Test in release/storm"
    exit 1
fi

grep "1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89" release/storm/output.txt > /dev/null
if [[ $? -ne 0 ]]
then
    echo "Failed to execute BS code. Test in release/storm"
    exit 1
fi

rm -r release/storm

echo "All seems well, copying the zip to fprg.se..."
cat release/storm.zip | ssh filip-www@fprg.se /home/filip-www/upload-storm.sh
