#!/bin/bash

function do_run {
    mkdir -p benchmark

    echo "Compiling..."
    nim compile -d:release root/benchmark/tak.nim
    nim compile -d:release root/benchmark/reverse.nim

    echo "== Tak =="
    echo "Nim..."
    root/benchmark/tak > benchmark/tak_nim
    echo "Racket..."
    racket root/benchmark/tak.rkt > benchmark/tak_rkt
    echo "Python..."
    python3 root/benchmark/tak.py > benchmark/tak_py
    echo "Storm..."
    ./Storm -f benchmark.fullTak > benchmark/tak_storm

    echo "== Reverse =="
    echo "Nim..."
    root/benchmark/reverse > benchmark/reverse_nim
    echo "Racket..."
    racket root/benchmark/reverse.rkt > benchmark/reverse_rkt
    echo "Python..."
    python3 root/benchmark/reverse.py > benchmark/reverse_py
    echo "Storm..."
    ./Storm -f benchmark.fullReverse > benchmark/reverse_storm
}

function process_tail {
    input=$1
    multiplier=$2

    read -d '' minmaxavg << 'EOF'
BEGIN {
    sum=0;
    count=0;
    min=9999999;
    max=0;
}
{
    count += 1;
    sum += $1;
    if ($1 > max) max = $1;
    if ($1 < min) min = $1;
}
END{
    print "Avg: ", sum/count;
    print "Min: ", min;
    print "Max: ", max;
}
EOF

    tail -n +11 | awk -v multiplier=$multiplier '{ print $1 * multiplier; }' > $input.out
    awk "$minmaxavg" $input.out > $input.avg
    #awk 'BEGIN { sum=0; count=0; } { count+=1; sum+=$1; } END { print sum/count; }' $input.out > $input.avg
    cat $input.avg
}

function process_file {
    input=$1
    sed -rn 's/Total time: ([0-9.]+)/\1/p' $input | process_tail $1 $2
}

function process_rkt {
    input=$1
    sed -rn 's/cpu time: ([0-9]+)/\1/p' $input | process_tail $1 1
}

function do_process {
    echo "== Tak =="
    echo "Nim: "; process_file benchmark/tak_nim 1000
    echo "Python: "; process_file benchmark/tak_py 1000
    echo "Storm: "; process_file benchmark/tak_storm 1
    echo "Racket: "; process_rkt benchmark/tak_rkt

    echo "== Reverse =="
    echo "Nim: "; process_file benchmark/reverse_nim 1000
    echo "Python: "; process_file benchmark/reverse_py 1000
    echo "Storm: "; process_file benchmark/reverse_storm 1
    echo "Racket: "; process_rkt benchmark/reverse_rkt
}

case $1 in
    run)
	do_run
	;;
    process)
	do_process
	;;	
    *)
	echo "Please run with either 'run' or 'process'."
	exit 1
	;;
esac
