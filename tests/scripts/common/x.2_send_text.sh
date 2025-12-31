#!/bin/bash

. init.sh

run_server --proto-max="$proto"

copy_text 'Sample text for send text'
if [ "$interface" = "web" ]; then
    "$program" >client.log &
    sleep 0.1
    http_status="$(curl -fs -w '%{http_code}' -o /dev/null -X POST -H 'Content-Length: 0' 'http://127.0.0.1:8888/send/text?server=127.0.0.1')"
    if [ "$http_status" != 200 ]; then
        showStatus info "Incorrect HTTP status: $http_status"
        exit 1
    fi
else
    "$program" -c s 127.0.0.1 >client.log
fi

check_logs
