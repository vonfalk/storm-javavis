#!/bin/bash

export PATH=$(pwd)/scripts:$PATH
export STORM_ROOT=$(pwd)

if [[ $# -ge 1 ]]
then
    cmd=$1
    shift
    scripts/release-$cmd "$@"
else
    scripts/release-full
fi
