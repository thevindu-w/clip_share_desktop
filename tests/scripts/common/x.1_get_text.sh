#!/bin/bash

. init.sh

clear_clipboard

sample='Sample text for get text'
run_server --proto-max="$proto" --text="$sample"

if [ "$interface" = "web" ]; then
    "$program" >client.log &
    sleep 0.1
    curl -fs -X POST -H 'Content-Length: 0' 'http://127.0.0.1:8888/get/text?server=127.0.0.1' >/dev/null
else
    "$program" -c g 127.0.0.1 >client.log
fi

received=$(get_copied_text 2>/dev/null || echo 'Error')
if [ "$received" != "$sample" ]; then
    showStatus info 'Incorrect text received.'
    echo 'Expected:' "$sample"
    echo 'Received:' "$received"
    exit 1
fi

check_logs
