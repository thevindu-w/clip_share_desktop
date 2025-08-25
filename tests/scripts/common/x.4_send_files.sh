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

"$program" -c fs 127.0.0.1 >client.log

if [ "$proto" = '1' ]; then
    diffOutput=$(diff -q original/file.txt files/file.txt 2>&1 || echo failed)
else
    diffOutput=$(diff -rq original files 2>&1 || echo failed)
fi
if [ -n "$diffOutput" ]; then
    showStatus info 'Files do not match.'
    exit 1
fi

check_logs
