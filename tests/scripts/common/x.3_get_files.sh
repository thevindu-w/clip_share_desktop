#!/bin/bash

. init.sh

run_server --proto-max="$proto" --files=1

mkdir files
cd files
"$program" -c fg 127.0.0.1 >../client.log

diffOutput=$({ diff -rq . "${TESTS_DIR}/files/${files_dir}" 2>&1 | { grep -v '.keep' || true; }; } || echo failed)
if [ -n "$diffOutput" ]; then
    showStatus info 'Files do not match.'
    exit 1
fi
cd ..

check_logs
