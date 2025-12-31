#!/bin/bash

. init.sh

clear_clipboard
if [ "$interface" = "web" ]; then
    "$program" >client.log &
    sleep 0.1
    http_status="$(curl -s -w '%{http_code}' -o /dev/null -X POST -H 'Content-Length: 0' 'http://127.0.0.1:8888/send/text?server=127.0.0.1')"
    if [ "$http_status" != 404 ]; then
        showStatus info "Incorrect HTTP status: $http_status"
        exit 1
    fi
else
    actual="$("$program" -c s 127.0.0.1 2>&1)"
    expected='Send text failed! - No text copied'
    if [ "$actual" != "$expected" ]; then
        showStatus info 'Incorrect CLI output.'
        echo 'Expected:' "$expected"
        echo 'Received:' "$actual"
        exit 1
    fi
fi
