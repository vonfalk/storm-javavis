#!/bin/bash

echo "Tak --------"
echo "Nim:"
nim compile -d:release root/benchmark/tak.nim
time root/benchmark/tak > /dev/null

echo "Racket:"
time racket root/benchmark/tak.rkt > /dev/null

echo "Storm:"
time echo "benchmark:testTak" | wine StormMain.exe > /dev/null

echo "Python:"
time python root/benchmark/tak.py > /dev/null


echo "Reverse ------"
echo "Nim:"
nim compile -d:release root/benchmark/reverse.nim
time root/benchmark/reverse > /dev/null

echo "Racket:"
time racket root/benchmark/reverse.rkt > /dev/null

echo "Storm:"
time echo "benchmark:testReverse" | wine StormMain.exe > /dev/null

echo "Python:"
time python root/benchmark/reverse.py > /dev/null
