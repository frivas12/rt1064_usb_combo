#!/bin/bash
SCRIPT_PATH="$(dirname -- "${BASH_SOURCE[0]}")"            # relative
PROJ_PATH="$(cd -- "$SCRIPT_PATH/../.." && pwd)"    # absolutized and normalized
DOCS_PATH="$(cd -- "$PROJ_PATH/../../../docs" && pwd)"

SIGN_PROG="$SCRIPT_PATH/../signing/bulk-sign.sh"
PACKAGE_PROG="$SCRIPT_PATH/../packaging/package.sh"

if [[ -z "$PROJ_PATH" ]] ; then
  # error; for some reason, the path is not accessible
  # to the script (e.g. permissions re-evaled after suid)
  exit 1  # fail
fi
if [[ -z "$DOCS_PATH" ]] ; then
  # error; for some reason, the path is not accessible
  # to the script (e.g. permissions re-evaled after suid)
  exit 1  # fail
fi

if [[ -z "$SIGNING_KEY" ]] ; then
    # error: for some reason, the signing key is not accessible.
    >&2 echo "error: environment variable SIGNING_KEY must be declared for binary signing"
    exit 1
fi

if [[ -z "$SIGNING_PASSWORD" ]] ; then
    # error: for some reason, the signing key is not accessible.
    >&2 echo "error: environment variable SIGNING_PASSWORD must be declared for binary signing"
    exit 1
fi

VERSION=`"$PROJ_PATH/build/get-version.sh" "$PROJ_PATH/Properties/AssemblyInfo.cs"` || exit 1

if [[ -z $VERSION ]]
then
  echo "Could not get version"
  exit 1
fi

BIN_PATH="$PROJ_PATH/bin/Debug"
PACKAGE_PATH="$PROJ_PATH/bin/package"
OUTPUT_PATH="$PROJ_PATH/bin" 
INSTALLER_PATH="$OUTPUT_PATH/PnP-MCM UI $VERSION-Installer.exe"
ZIP_PATH="$OUTPUT_PATH/PnP-MCM UI $VERSION.zip"

if [[ -e "$PACKAGE_PATH" ]]; then
    echo "Removing package directory ($PACKAGE_PATH)"
    rm -r "$PACKAGE_PATH/"
fi
if [[ -e "$INSTALLER_PATH" ]]; then
    echo "Removing previous installer ($INSTALLER_PATH/)"
    rm "$INSTALLER_PATH"
fi
if [[ -e "$ZIP_PATH" ]]; then
    echo "Removing previous archive ($ZIP_PATH)"
    rm "$ZIP_PATH"
fi

mkdir -p "$PACKAGE_PATH"
cp "$BIN_PATH/"*.dll "$PACKAGE_PATH/"
cp "$BIN_PATH/"*.exe "$PACKAGE_PATH/"
cp "$BIN_PATH/LUTDevices.accdb" "$PACKAGE_PATH/"
cp -r "$DOCS_PATH/Drivers" "$PACKAGE_PATH/Driver"
cp -r "$BIN_PATH/xml" "$PACKAGE_PATH/xml"
cp "$BIN_PATH/config.txt" "$PACKAGE_PATH/"
cp "$BIN_PATH/"*.csv "$PACKAGE_PATH/"


"$SIGN_PROG" "$PACKAGE_PATH/"*.dll "$PACKAGE_PATH/"*.exe
"$PACKAGE_PROG" "PnP-MCM_UI" "$VERSION" "$OUTPUT_PATH" "$PACKAGE_PATH" MCM301_UI
mv "$OUTPUT_PATH/PnP_MCM-Installer.exe" "$INSTALLER_PATH"
(cd "$PACKAGE_PATH" && zip -r "$OUTPUT_PATH/PnP-MCM UI $VERSION.zip" .)
rm -r "$PACKAGE_PATH/"
