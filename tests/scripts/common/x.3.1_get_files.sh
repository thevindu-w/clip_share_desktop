#!/bin/bash

. init.sh

run_server --proto-max="$proto"

if [ "$interface" = "web" ]; then
    "$program" >client.log &
    sleep 0.1
    http_status="$(curl -s -w '%{http_code}' -o /dev/null -X POST -H 'Content-Length: 0' 'http://127.0.0.1:8888/get/file?server=127.0.0.1')"
    if [ "$http_status" != 404 ]; then
        showStatus info "Incorrect HTTP status: $http_status"
        exit 1
    fi
else
    "$program" -c fg 127.0.0.1 >client.log
fi

check_logs
