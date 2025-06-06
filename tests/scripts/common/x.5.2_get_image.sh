#!/bin/bash

. init.sh

run_server --proto-max="$proto" --image=screenshot

"$program" -c i 127.0.0.1 >client.log

diffOutput=$(diff -q *.png "${TESTS_DIR}/files/screen.png" 2>&1 || echo failed)
if [ ! -z "$diffOutput" ]; then
    showStatus info 'Image does not match.'
    exit 1
fi

check_logs
