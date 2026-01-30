#!/usr/bin/bash
SOURCE=../Firmware/config.mk

get_index () {
    cat $SOURCE | grep "FW_.*_VER" | head -n $(( 1 + $1 )) | tail -n 1 | sed -E "s/.*=.*([0-9]+)/\1/"
}

echo `get_index 0`.`get_index 1`.`get_index 2`
