# !/bin/bash

SOURCE_DIR=../MCM6000_UI/MCM6000_UI/bin/Debug/xml/system/configs/
OUTPUT_DIR=${SOURCE_DIR}bin/
COMPILER=build/lutbuild.exe

# Create output dir, if it doesn't exist
mkdir -p $OUTPUT_DIR

# Compile each file
for file in $SOURCE_DIR*; do
    if [[ ! -d $file ]]; then
        file_name=`echo $file | sed -r "s/.+\/(.+)\..+/\1/"`
        file_ext=`echo $file | sed -r "s/.+\/.+\.(.+)/\1/"`
        echo $file_name.$file_ext
        if [[ file_ext -eq "xml" ]]; then
            $COMPILER $file --output_path=$OUTPUT_DIR$file_name.bin
        fi
    fi
done