#!/usr/bin/bash

OUTPUT_FILE=test-jig.zip

INCLUDE_FILES=" 
comms.py
pls_jig.py
auto_jig.py
"

rm "$OUTPUT_FILE" requirements.txt
pipreqs . || exit -1
mkdir packages/
pushd packages/
while read line; do
    pip download "$line" || exit -1
done < ../requirements.txt
popd
zip -r "$OUTPUT_FILE" $INCLUDE_FILES requirements.txt packages/
rm requirements.txt
rm -r packages/