#!/bin/bash
size=`find . -name "*.cpp" -or -name "*.h" | xargs ls -l | awk '{ total += \$5 }; END { print total }'`
echo "Total size: $size bytes"

