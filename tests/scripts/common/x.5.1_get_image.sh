#!/bin/bash

. init.sh

run_server --proto-max="$proto" --image=copied

if [ "$interface" = "web" ]; then
    "$program" >client.log &
    sleep 0.1
    http_status="$(curl -fs -w '%{http_code}' -o /dev/null -X POST -H 'Content-Length: 0' 'http://127.0.0.1:8888/get/image?server=127.0.0.1')"
    if [ "$http_status" != 200 ]; then
        showStatus info "Incorrect HTTP status: $http_status"
    fi
else
    "$program" -c i 127.0.0.1 >client.log
fi

diffOutput=$(diff -q *.png "${TESTS_DIR}/files/image.png" 2>&1 || echo failed)
if [ ! -z "$diffOutput" ]; then
    showStatus info 'Image does not match.'
    exit 1
fi

check_logs
