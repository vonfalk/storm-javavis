#!/bin/bash
mkdir -p log/
drmemory -logdir log/ -no_check_leaks -no_possible_leaks -report_leak_max 1 -no_fetch_symbols -batch -- "$@"
