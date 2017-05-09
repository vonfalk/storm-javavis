#!/bin/bash

echo "Creating zip-file..."
if [[ -e release/storm.zip ]]
then
    rm release/storm.zip
fi

7z a release/storm.zip doc/ > /dev/null

echo "Uploading documentation to fprg.se..."
cat release/storm.zip | ssh filip-www@fprg.se /home/filip-www/upload-doc.sh
