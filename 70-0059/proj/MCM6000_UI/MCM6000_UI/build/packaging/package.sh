#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

NSIS_MAKE="/c/Program Files (x86)/NSIS/makensis.exe"

function help ()
{
    echo "Usage: $0 <product name> <product veresion> <output directory> <input directory> [executable name]"
    echo "Arguments:"
    echo '    "product name"            The name of the product'
    echo '    "product version"         The version of the product'
    echo '    "output directory"        The directory to output the installer'
    echo '    "input directory"         The directory to package.'
    echo '    "executable name"         Optional (defaults to "<product name>.exe")'
    echo '                              The name of the executable for path linking.'
    echo '                              This should be a relative path from the input directory.'
}

if [[ $1 == "help" ]]; then
    help $0
    exit 0
fi

if [[ $# == 4 ]]; then
    EXEC_NAME="$1"
elif [[ $# == 5 ]]; then
    EXEC_NAME="$5"
else
    help $0
    exit 1
fi

OUT_DIR=`realpath "$3"`
OBJ="$1"-Installer.exe

"$NSIS_MAKE" -DPRODUCT_NAME="$1" -DPRODUCT_VERSION="$2" -DOUT_DIR="$OUT_DIR" -DPACKAGE_DIR="`realpath "$4"`" -DEXECUTABLE="$EXEC_NAME" $SCRIPT_DIR/Install_Script.nsi

if [[ ! -f "$OUT_DIR/$OBJ" ]]; then
    echo "Could not find output target:  $OUT_DIR/$OBJ"
    exit 1
fi
