#!/bin/bash

. init.sh

run_server --proto-max=3

if [ "$interface" = "web" ]; then
    "$program" >client.log &
    sleep 0.1
    http_status="$(curl -s -w '%{http_code}' -o /dev/null -X POST -H 'Content-Length: 0' 'http://127.0.0.1:8888/get/copied-image?server=127.0.0.1')"
    if [ "$http_status" != 404 ]; then
        showStatus info "Incorrect HTTP status: $http_status"
    fi
else
    "$program" -c ic 127.0.0.1 >client.log
fi

check_logs
