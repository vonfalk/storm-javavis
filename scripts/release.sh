#!/bin/bash

echo "Building release..."
scripts/compile -r

echo "Done! Running the test suite..."
Release/CodeTest.exe || { echo "Tests failed!"; exit 1; }
Release/StormTest.exe || { echo "Tests failed!"; exit 1; }

echo "Seems good"

echo "Creating zip-file..."
if [[ -e Release/storm.zip ]]
then
    rm Release/storm.zip
fi

7z a Release/storm.zip ./Release/StormMain.exe doc/ > /dev/null
find root/ -name "*.bs" -or -name "*.bnf" -or -name "*.txt" -or -name "*.png" \
  | xargs --delimiter='\n' 7z a Release/storm.zip > /dev/null

# Add dlls, we need to rename them first...
IFS=$'\n'
for j in `find root/ -name "*Release.dll"`
do
    name=`echo $j | sed 's/Release\.dll/.dll/g'`
    cp $j $name
    7z a Release/storm.zip $name > /dev/null
    rm $name
done

echo "Checking so that the release works..."
if [[ -e Release/storm ]]
then
    rm -r Release/storm/*
else
    mkdir Release/storm
fi

7z x Release/storm.zip -oRelease/storm/ > /dev/null
echo "exit" | Release/storm/StormMain.exe > /dev/null
if [[ $? -ne 0 ]]
then
    echo "Failed to launch the repl. Something is bad! Test in Release/storm"
    exit 1
fi

Release/storm/StormMain.exe -c 'test:bf:inlineBf' > Release/storm/output.txt
if [[ $? -ne 0 ]]
then
    echo "Failed to execute BS code. Test in Release/storm"
    exit 1
fi

grep "1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89" Release/storm/output.txt > /dev/null
if [[ $? -ne 0 ]]
then
    echo "Failed to execute BS code. Test in Release/storm"
    exit 1
fi

rm -r Release/storm

echo "All seems well, copying the zip to fprg.se..."
scp Release/storm.zip "filip-www@fprg.se:~/www/storm/storm.zip"
