#!/usr/bin/bash
echo "$@" | sed -E "s/ /\\n/g" | sort -rV | head -n 1
