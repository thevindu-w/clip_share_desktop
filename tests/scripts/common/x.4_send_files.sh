#!/bin/bash

. init.sh

mkdir files
cp -r "${TESTS_DIR}/files/${files_dir}" original
cd original
find . -type f -name '.keep' -delete
find . -type f -name '*.DS_Store' -delete
copy_files *
cd ../files
run_server --proto-max="$proto"
cd ..

if [ "$interface" = "web" ]; then
    "$program" >client.log &
    sleep 0.05
    sleep 0.1
    curl -fs -X POST -H 'Content-Length: 0' 'http://127.0.0.1:8888/send/file?server=127.0.0.1' >/dev/null
else
    "$program" -c fs 127.0.0.1 >client.log
fi

if [ "$proto" = '1' ]; then
    diffOutput=$(diff -q original/file.txt files/file.txt 2>&1 || echo failed)
else
    diffOutput=$(diff -rq original files 2>&1 || echo failed)
fi
if [ -n "$diffOutput" ]; then
    showStatus info 'Files do not match.'
    echo 'Diff:'
    echo "$diffOutput"
    exit 1
fi

check_logs
