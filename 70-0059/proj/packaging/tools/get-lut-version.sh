#!/usr/bin/bash
cat $1 | grep version | sed -E "s/.*=.*([0-9]+\.[0-9]+\.[0-9]+).*/\1/"
