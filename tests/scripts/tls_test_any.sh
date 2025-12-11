#!/bin/bash

. init.sh
clear_clipboard

update_config secure_mode_enabled true
update_config client_cert client.pfx
update_config ca_cert testCA.crt
update_config trusted_servers trusted_servers.txt

sample='Sample text for get text'
run_server --tls=true --text="$sample"

"$program" -c g 127.0.0.1 >client.log

received=$(get_copied_text 2>/dev/null || echo 'Error')
if [ "$received" != "$sample" ]; then
    showStatus info 'Incorrect text received.'
    echo 'Expected:' "$sample"
    echo 'Received:' "$received"
    exit 1
fi

check_logs
