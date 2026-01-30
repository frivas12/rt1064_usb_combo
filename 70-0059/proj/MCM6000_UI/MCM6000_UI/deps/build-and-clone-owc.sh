#/usr/bin/bash
SCRIPT_PATH="$(dirname -- "${BASH_SOURCE[0]}")"            # relative

pushd "$SCRIPT_PATH/../../../LUT_Builder" > /dev/null
make
cp bin/owc.exe "$SCRIPT_PATH/owc.exe"
popd > /dev/null