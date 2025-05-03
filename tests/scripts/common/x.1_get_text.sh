#!/bin/bash

. init.sh

clear_clipboard

sample='Sample text for get text'
run_server --proto-max="$proto" --text="$sample"

"$program" -c g 127.0.0.1 >client.log

received=$(get_copied_text 2>/dev/null || echo 'Error')
if [ "$received" != "$sample" ]; then
    showStatus info 'Incorrect text received.'
    echo 'Expected:' "$sample"
    echo 'Received:' "$received"
    exit 1
fi

check_logs
