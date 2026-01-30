#!/bin/bash

SIGN_TOOL=/c/Program\ Files\ \(x86\)/Microsoft\ SDKs/ClickOnce/SignTool/signtool.exe
TIMESTAMP_URL="http://timestamp.digicert.com"


"$SIGN_TOOL" sign -f $SIGNING_KEY -p $SIGNING_PASSWORD -fd SHA256 -t $TIMESTAMP_URL "$@"
