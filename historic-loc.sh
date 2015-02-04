#!/bin/bash

storm_regex="Storm.*/.*(\.h|\.cpp)"
code_regex="Code.*/.*(\.h|\.cpp)"

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
    git cat-file --batch=""
}

function count_input {
    perl cloc.pl --stdin-name="foo.cpp" --quiet --csv-delimiter=";" - | tail -n1 | cut -d";" -f3-
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
    echo $1";" | while read -d";" a; do
	local b=`echo $other | cut -d";" -f1`
	echo -n $(($a + $b))";"
	other=`echo $other | cut -d";" -f2-`
    done
}

echo "date,code blank,code comment,code code,storm blank,storm comment,storm code,sum blank,sum comment,sum code"

git log --format="format:%H %ci" master | while read commit date time timezone; do
    echo -n $date $time";"

    code=`count $commit $code_regex | tr -d "\n"`
    echo -n $code";"

    storm=`count $commit $storm_regex | tr -d "\n"`
    echo -n $storm";"

    compute_sum $code $storm
    echo ""
done
