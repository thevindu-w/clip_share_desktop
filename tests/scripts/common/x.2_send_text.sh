#!/bin/bash

. init.sh

run_server --proto-max="$proto"

sample='Sample text for send text'
copy_text "$sample"
if [ "$interface" = "web" ]; then
    "$program" >client.log >>client.log &
    sleep 0.05
    sleep 0.1
    curl -fs -X POST -H 'Content-Length: 0' 'http://127.0.0.1:8888/send/text?server=127.0.0.1' >/dev/null
else
    "$program" -c s 127.0.0.1 >client.log
fi

check_logs
