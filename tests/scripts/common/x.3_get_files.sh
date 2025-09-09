#!/bin/bash

. init.sh

run_server --proto-max="$proto" --files=1

mkdir files
cd files
if [ "$interface" = "web" ]; then
    "$program" >../client.log &
    sleep 0.1
    curl -fs -X POST -H 'Content-Length: 0' 'http://127.0.0.1:8888/get/file?server=127.0.0.1' >/dev/null
else
    "$program" -c fg 127.0.0.1 >../client.log
fi

diffOutput=$({ diff -rq . "${TESTS_DIR}/files/${files_dir}" 2>&1 | { grep -v '.keep' || true; }; } || echo failed)
if [ -n "$diffOutput" ]; then
    showStatus info 'Files do not match.'
    echo 'Diff:'
    echo "$diffOutput"
    exit 1
fi
cd ..

check_logs
