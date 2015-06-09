#!/bin/bash

storm_regex="(Storm.*|Shared)/.*(\.h|\.cpp)"
code_regex="(OS|Code).*/.*(\.h|\.cpp)"
bs_regex="root/.*\.bs"

function find_files {
    local commit=$1
    local regex=$2
    echo $regex
    git ls-tree -r $commit | grep -E $regex
}

function extract_hashes {
    cut -d" " -f3 | cut -f1
}

function list_hashes {
    # git cat-file --batch=""
    git cat-file --batch | grep -v "^[0-9a-f]\{40\} blob [0-9]\+$"
}

function count_input {
    perl ../cloc.pl --stdin-name="foo.cpp" --quiet --csv-delimiter=";" - | tail -n1 | cut -d";" -f3-
}

function count {
    local commit=$1
    local regex=$2

    find_files $commit $regex | extract_hashes | list_hashes | count_input
    # git ls-tree -r $commit | grep -E $file_regex | \
    # 	cut -d" " -f3 | cut -f1 | \
    # 	git cat-file --batch="" | \
    # 	perl cloc.pl --stdin-name="foo.cpp" --quiet --csv - | \
    # 	tail -n1 | cut -d"," -f3-
}

function compute_sum {
    local other=$2
    echo $1 | while read -d";" a; do
	local b=`echo $other | cut -d";" -f1`
	echo -n $(($a + $b))";"
	other=`echo $other | cut -d";" -f2-`
    done
}

function compute_line {
    local commit=$1
    
    code=`count $commit $code_regex | tr -d "\n"`
    echo -n $code";"

    storm=`count $commit $storm_regex | tr -d "\n"`
    echo -n $storm";"

    bs=`count $commit $bs_regex | tr -d "\n"`
    echo -n $bs";"

    sum=`compute_sum $code";" $storm";"`
    compute_sum $sum $bs";"
    echo ""
}

echo "Date;Code: blank;Code: comment;Code: code;Storm: blank;Storm: comment;Storm: code;Basic Storm: blank;Basic Storm: comment;Basic Storm: code;Total: blank;Total: comment;Total: code"

git log --format="format:%H %ai" $1 | while read commit date time timezone; do
    echo -n $date $time";"

    cached=`grep $commit"->" loc_cache 2>/dev/null`
    if [[ $? -ne 0 ]]
    then
	cached=`compute_line $commit`
	echo $commit"->"$cached >> loc_cache
    else
	cached=${cached#$commit"->"}
    fi
    echo $cached

done
