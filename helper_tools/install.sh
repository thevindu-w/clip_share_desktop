#!/bin/bash

if [ "$(uname)" = 'Linux' ]; then
    . helper_tools/install-linux.sh
else
    echo Could not detect the OS.
    exit 1
fi
