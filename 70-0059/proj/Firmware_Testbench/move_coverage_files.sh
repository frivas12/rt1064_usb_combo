#!/bin/bash

if [ $# -gt 0 ]; then
    i=1
    while [ "$i" -le $# ];
    do
        if [ -e ${!i} ]; then
            mv ${!i} build
        fi

        i=$(( i + 1 ))
    done
fi