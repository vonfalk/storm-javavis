#!/bin/bash

LC_NUMERIC="en_US.UTF-8"
total=0

for i in ./*/.mymake
do
    dir=$(dirname $i)
    dirname=$(basename $dir)
    if [[ $dirname = "mps_tests" ]]
    then
	:
    elif [[ $dirname = "SoundLib" ]]
    then
	:
    else
	if [[ $dirname = "Core" ]]
	then
	    files=$(find $dir -name "*.cpp" -or -name "*.h")
	else
	    files=$(find $dir -name "*.cpp" -or -name "*.h" | grep -v "./.*/Gen/")
	fi

	size=$(echo $files | xargs ls -l | awk '{ total += $5 }; END { print total/1024 }')
	printf "%10s %7.1f kb\n" $dirname $size

	total=$(printf "%s\n%s" $size $total | awk '{ total += $1 }; END { print total }')
    fi
done

printf "Total size %7.1f kb\n" $total

