#!/bin/bash

function find_version {
    tag=`git describe --abbrev=0 master`
    desc=`git describe master`

    if [[ $tag != $desc ]]
    then
	# Format the extra information.
	tag=`echo $desc | sed -E 's/-([0-9]+)-([a-zA-Z0-9]+)$/+git.\1.\2/'`
    fi

    echo $tag | sed 's/^v//'
}

function create_version {
    tag=`git describe --abbrev=0 master`
    git describe master > release_message.txt
    echo "" >> release_message.txt
    echo "# Commits since previous release:" >> release_message.txt
    git log ${tag}..master | sed -e 's/^/# /' >> release_message.txt
    $EDITOR release_message.txt

    version=`head -n 1 release_message.txt`
    tail -n +2 release_message.txt | grep -v "# " | git tag -a -F - v${version}
    rm release_message.txt
}

branch=`git rev-parse --abbrev-ref HEAD`
if [[ $branch != "master" ]]
then
    echo "Error: You need to checkout the branch 'master' to perform a release."
    exit 1
fi

version=`find_version`
read -p "Current version is $version. Change it? [Y/n] " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]] || [[ -z $REPLY ]]
then
    create_version
    version=`find_version`
fi

echo "Current version: $version"

echo "core.info" > Compiler/COMPILER.version
echo "$version" >> Compiler/COMPILER.version

echo "Building release..."
mm release -ne || { echo "Compilation failed!"; exit 1; }

echo "Done! Running the test suite..."
release/Test --all || { echo "Tests failed!"; exit 1; }

echo "Seems good"

echo "Creating zip-file..."
if [[ -e release/storm.zip ]]
then
    rm release/storm.zip
fi

7z a release/storm.zip ./release/Storm.exe doc/ > /dev/null
find root/ -type f -not -name "*.dll" | grep -v "server-tests/" | xargs --delimiter='\n' 7z a release/storm.zip > /dev/null

# Add dlls, we need to rename them first...
IFS=$'\n'
for j in `find root/ -name "Release*.dll"`
do
    name=`echo $j | sed 's/Release//g'`
    cp $j $name
    7z a release/storm.zip $name > /dev/null
    rm $name
done

# Add the emacs plugin.
cp Plugin/emacs.el storm-mode.el
7z a release/storm.zip storm-mode.el > /dev/null
rm storm-mode.el

echo "Checking so that the release works..."
if [[ -e release/storm ]]
then
    rm -r release/storm/* 2> /dev/null
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
