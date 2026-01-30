#!/usr/bin/bash
SOURCE=../cpld/sources/commands.v

get_index () {
    cat $SOURCE | grep "CPLD_REV" | head -n $(( 1 + $1 )) | tail -n 1 | sed -E "s/.*CPLD_REV.*([0-9]+).*/\\1/"
}

echo `get_index 0`.`get_index 1`.0
