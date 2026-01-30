#!/bin/bash

# Gets the version of the File provided

#regex="\\s*[\\s*assembly:\\s*AssemblyVersion(\"(.*)\")\\s*]"
regex="^\\s*\[\\s*assembly: AssemblyVersion\(\"(.*)\""

while IFS= read -r line;
do
    if [[ "$line" =~ $regex ]];
    then
        echo "${BASH_REMATCH[1]}"
    fi
done < $1