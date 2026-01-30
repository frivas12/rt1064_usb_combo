#!/usr/bin/bash
cat $1 | sed "s/{{FGPN}}/$2/g"
