#!/bin/bash

. init.sh

run_server --proto-max=3 --image=copied

if [ "$interface" = "web" ]; then
    "$program" >client.log &
    sleep 0.1
    curl -fs -X POST -H 'Content-Length: 0' 'http://127.0.0.1:8888/get/copied-image?server=127.0.0.1' >/dev/null
else
    "$program" -c ic 127.0.0.1 >client.log
fi

diffOutput=$(diff -q *.png "${TESTS_DIR}/files/image.png" 2>&1 || echo failed)
if [ ! -z "$diffOutput" ]; then
    showStatus info 'Image does not match.'
    exit 1
fi

check_logs
