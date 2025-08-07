#!/bin/bash

set -e

if [ "$(id -u)" = 0 ]; then
    echo 'Do not run install with sudo.'
    echo 'Aborted.'
    exit 1
fi

echo 'This will install ClipShare-desktop to run on startup.'
read -p 'Proceed? [y/N] ' confirm
if [ "${confirm::1}" != 'y' ] && [ "${confirm::1}" != 'Y' ]; then
    echo 'Aborted.'
    echo 'You can still use clip-share-client by manually running the executable.'
    exit 0
fi

exec_names=(
    clip-share-client
    clip-share-client-arm64
    clip-share-client-x86_64
)

exec_path=~/.local/bin/clip-share-client
exec_not_found=1
missingLib=
for exec_name in "${exec_names[@]}"; do
    if [ -f "$exec_name" ]; then
        chmod +x "$exec_name"
        if ! sh -c "./$exec_name -h 2>&1" &>/dev/null; then
            missing="$( (sh -c "./$exec_name -h 2>&1" 2>/dev/null || true) | grep -oE 'lib[a-zA-Z0-9.-]+\.dylib' | grep -oE 'lib[a-zA-Z0-9-]+' | head -n 1)"
            [ -n "$missing" ] && missingLib="$missing"
            continue
        fi
        exec_not_found=0
        mkdir -p ~/.local/bin/
        mv "$exec_name" "$exec_path"
        chmod +x "$exec_path"
        echo Moved "$exec_name" to "$exec_path"
        break
    fi
done

if [ "$exec_not_found" = 1 ]; then
    if [ -n "$missingLib" ]; then
        echo "Error: The ${missingLib} library is not installed. Please install ${missingLib} and run the installer again."
    else
        echo "Error: 'clip-share-client' program was not found in the current directory"
    fi
    exit 1
fi

cd
export HOME="$(pwd)"

CONF_PATHS=("$XDG_CONFIG_HOME" "${HOME}/.config" "$HOME")
for directory in "${CONF_PATHS[@]}"; do
    [ -d "$directory" ] || continue
    conf_path="${directory}/clipshare-desktop.conf"
    if [ -f "$conf_path" ] && [ -r "$conf_path" ]; then
        CONF_DIR="$directory"
        break
    fi
done
for directory in "${CONF_PATHS[@]}"; do
    [ -n "$CONF_DIR" ] && break
    if [ -d "$directory" ] && [ -w "$directory" ]; then
        CONF_DIR="$directory"
    fi
done
if [ -z "$CONF_DIR" ]; then
    echo "Error: Could not find a directory for the configuration file!"
    exit 1
fi
CONF_FILE="${CONF_DIR}/clipshare-desktop.conf"

if [ ! -f "$CONF_FILE" ]; then
    mkdir -p Downloads
    echo "working_dir=${HOME}/Downloads" >"$CONF_FILE"
    echo "Created a new configuration file $CONF_FILE"
fi

mkdir -p Library/LaunchAgents/

if [ -f Library/LaunchAgents/com.tw.clipshare-desktop.plist ]; then
    echo
    echo 'A previous installation of ClipShare-desktop is available.'
    read -p 'Update? [y/N] ' confirm_update
    if [ "${confirm_update}" != 'y' ] && [ "${confirm_update}" != 'Y' ]; then
        echo 'Aborted.'
        exit 0
    fi
fi

cat >Library/LaunchAgents/com.tw.clipshare-desktop.plist <<EOF
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
   "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
    <dict>
        <key>EnvironmentVariables</key>
        <dict>
            <key>XDG_CONFIG_HOME</key>
            <string>$CONF_DIR</string>
        </dict>
        <key>Label</key>
        <string>com.tw.clipshare-desktop</string>
        <key>ProgramArguments</key>
        <array>
            <string>$exec_path</string>
            <string>-D</string>
        </array>
        <key>RunAtLoad</key>
        <true />
    </dict>
</plist>
EOF

launchctl unload -w ~/Library/LaunchAgents/com.tw.clipshare-desktop.plist &>/dev/null || true
launchctl load -w ~/Library/LaunchAgents/com.tw.clipshare-desktop.plist
echo 'Installed ClipShare-desktop to run on startup.'
