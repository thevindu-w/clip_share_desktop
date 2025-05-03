#!/bin/bash

. init.sh

run_server --proto-max="$proto"

sample='Sample text for send text'
copy_text "$sample"
"$program" -c s 127.0.0.1 >client.log

check_logs
