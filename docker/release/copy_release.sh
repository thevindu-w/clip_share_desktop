#!/bin/bash

ldd_head="$(ldd --version | head -n 1)"
glibc_version=$(echo -n "${ldd_head##* }" | grep -oE '[0-9]+\.[0-9]+' | head -n 1)

mv -n clip-share-client "dist/clip-share-client_GLIBC-${glibc_version}" || echo 'Not replacing'

if [ -f clipshare-client.AppImage ]; then
    mv -n clipshare-client.AppImage dist/
fi
