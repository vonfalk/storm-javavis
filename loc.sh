#!/bin/bash
echo "----- Compiler--------------------------------------------"
find Compiler/ CppTypes/ Core/ Test/ -name "*.cpp" -or -name "*.h" | xargs perl cloc.pl
echo "----- OS -------------------------------------------------"
find OS/ -name "*.cpp" -or -name "*.h" | xargs perl cloc.pl
echo "----- Code  ----------------------------------------------"
find Code/ CodeTest/ -name "*.cpp" -or -name "*.h" | xargs perl cloc.pl
echo "----- Total ----------------------------------------------"
find ./ -name "*.cpp" -or -name "*.h" | grep -viE '^\./(Old|mps|SoundLib|Linux)/' | xargs perl cloc.pl

echo "----- Basic Storm ----------------------------------------"
find ./ -name "*.bs" | xargs perl cloc.pl --force-lang=Java
echo "----- Syntax ---------------------------------------------"
find ./ -name "*.bnf" | xargs perl cloc.pl --force-lang=Java
