#!/bin/bash

. init.sh

mkdir files
cp -r "${TESTS_DIR}/files/${files_dir}" original
cd original
find . -type f -name '.keep' -delete
find . -type f -name '*.DS_Store' -delete
shopt -s nullglob
file_list=(./*)
shopt -u nullglob
copy_files "${file_list[@]}"
cd ../files
run_server --proto-max="$proto"
cd ..

if [ "$interface" = "web" ]; then
    "$program" >client.log &
    sleep 0.1
    http_status="$(curl -fs -w '%{http_code}' -o /dev/null -X POST -H 'Content-Length: 0' 'http://127.0.0.1:8888/send/file?server=127.0.0.1')"
    if [ "$http_status" != 200 ]; then
        showStatus info "Incorrect HTTP status: $http_status"
        exit 1
    fi
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
