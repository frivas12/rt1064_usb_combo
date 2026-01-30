#!/usr/bin/bash
SCRIPT_DIR="$(dirname -- "${BASH_SOURCE[0]}")"            # relative
pushd "$SCRIPT_DIR" &>/dev/null
pyinstaller -F -p . production_mpm_jig.py
popd &>/dev/null
