#!/bin/bash

set -e

mkdir -p AppDir/usr/bin AppDir/usr/lib/x86_64-linux-gnu
cp clip-share-client AppDir/usr/bin/
cd AppDir/usr/lib/x86_64-linux-gnu
ldd ../../bin/clip-share-client | grep -oE '/lib.*/x86_64-linux-gnu/[^ ]+\.so[^ ]*' | xargs -n 1 -I {} cp {} .
rm libc.so* libpthread.so* libdl.so*
cd ../../../../
ARCH=x86_64 appimagetool --appimage-extract-and-run AppDir clipshare-client.AppImage
