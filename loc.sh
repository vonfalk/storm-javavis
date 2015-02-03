#!/bin/bash
echo "----- Storm ----------------------------------------------"
find Storm/ StormBuiltin/ StormTest/ -name "*.cpp" -or -name "*.h" | xargs perl cloc.pl
echo "----- Code  ----------------------------------------------"
find Code/ CodeTest/ -name "*.cpp" -or -name "*.h" | xargs perl cloc.pl
echo "----- Total ----------------------------------------------"
find ./ -name "*.cpp" -or -name "*.h" | xargs perl cloc.pl
