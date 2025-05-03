#!/bin/bash

program=$1

dependencies=(
    "../${program}"
    cat
    chmod
    cp
    env
    printf
    pwd
    realpath
    seq
    sleep
    timeout
    diff
    python3
    sed
)

if [ "$OS" = 'Windows_NT' ]; then
    DETECTED_OS='Windows'
    dependencies+=(powershell)
elif [ "$(uname)" = 'Linux' ]; then
    DETECTED_OS='Linux'
    dependencies+=(xclip)
elif [ "$(uname)" = 'Darwin' ]; then
    DETECTED_OS='macOS'
    dependencies+=(pbcopy pbpaste osascript)
    export PATH="/usr/local/opt/coreutils/libexec/gnubin:/usr/local/opt/gnu-sed/libexec/gnubin:$PATH"
else
    DETECTED_OS='unknown'
fi

# Check if all dependencies are available
for dependency in "${dependencies[@]}"; do
    if ! type "$dependency" &>/dev/null; then
        echo "\"$dependency\"" not found
        exit 1
    fi
done

if [ "$DETECTED_OS" = 'macOS' ]; then
    for lcvar in $(env | grep '^LC_' | sed 's/=.*//g'); do
        unset "$lcvar"
    done
    export LC_CTYPE='UTF-8'
fi

# Export variables
export MIN_PROTO=1
export MAX_PROTO=3
export DETECTED_OS
export TESTS_DIR="$(pwd)"
export LOG_DIR="${TESTS_DIR}/server_output"

# Get the absolute path of clip_share executable
program="$(realpath "../${program}")"

shopt -s expand_aliases

# Set the color for console output
setColor() {
    # Get the color code from color name
    getColorCode() {
        if [ "$1" = 'red' ]; then
            echo 31
        elif [ "$1" = 'green' ]; then
            echo 32
        elif [ "$1" = 'yellow' ]; then
            echo 33
        elif [ "$1" = 'blue' ]; then
            echo 34
        else
            echo 0
        fi
    }

    # Set the color for console output
    colorSet() {
        local color_code="$(getColorCode $1)"
        if [ "$2" = 'bold' ]; then
            printf "\033[1;${color_code}m"
        else
            printf "\033[${color_code}m"
        fi
    }
    case "$TERM" in
        xterm-color | *-256color) colorSet "$@" ;;
    esac
}

# Show status message. Usage showStatus <script-name> <status:pass|fail|warn|info> [message]
showStatus() {
    local name=$(basename -- "$1" | sed 's/\(.*\)\..*/\1/g')
    if [ "$2" = 'pass' ]; then
        setColor 'green'
        echo -n 'PASS: '
        setColor reset
        echo "$name"
    elif [ "$2" = 'fail' ]; then
        setColor 'red'
        echo -n 'FAIL: '
        setColor reset
        printf '%-27s %s\n' "$name" "$3"
    elif [ "$2" = 'warn' ]; then
        setColor 'yellow'
        echo -n 'WARN: '
        setColor reset
        printf '%-27s %s\n' "$name" "$3"
    elif [ "$2" = 'info' ]; then
        setColor 'blue'
        echo -n 'INFO: '
        setColor reset
        printf '%-27s %s\n' "$name" "$3"
    else
        setColor reset
    fi
}

# Copy a string to clipboard
copy_text() {
    if [ "$DETECTED_OS" = 'Linux' ]; then
        echo -n "$1" | xclip -in -sel clip &>/dev/null
    elif [ "$DETECTED_OS" = 'Windows' ]; then
        powershell -c "Set-Clipboard -Value '$1'"
    elif [ "$DETECTED_OS" = 'macOS' ]; then
        echo -n "$1" | pbcopy -pboard general &>/dev/null
    else
        echo "Copy text is not available for OS: $DETECTED_OS"
        exit 1
    fi
}

# Get copied text from clipboard
get_copied_text() {
    if [ "$DETECTED_OS" = 'Linux' ]; then
        xclip -out -sel clip
    elif [ "$DETECTED_OS" = 'Windows' ]; then
        local prev_dir="$(pwd)"
        cd /tmp
        powershell -c 'Get-Clipboard -Raw | Out-File "clip.txt" -Encoding utf8'
        local copied_text="$(cat 'clip.txt')"
        copied_text="$(echo -n "$copied_text" | xxd -p)"
        cd "$prev_dir"
        # remove BOM if present
        if [ "${copied_text::6}" = 'efbbbf' ]; then
            copied_text="${copied_text:6}"
        fi
        echo -n "$copied_text" | xxd -r -p
    elif [ "$DETECTED_OS" = 'macOS' ]; then
        pbpaste -pboard general -Prefer txt
    else
        echo "Get copied text is not available for OS: $DETECTED_OS"
        exit 1
    fi
}

# Clear the content of clipboard
clear_clipboard() {
    if [ "$DETECTED_OS" = 'Linux' ]; then
        xclip -in -sel clip -l 1 <<<'dummy' &>/dev/null
        xclip -out -sel clip &>/dev/null
    elif [ "$DETECTED_OS" = 'Windows' ]; then
        powershell -c 'Set-Clipboard -Value $null'
    elif [ "$DETECTED_OS" = 'macOS' ]; then
        pbcopy </dev/null
    else
        echo "Clear clipboard is not available for OS: $DETECTED_OS"
        exit 1
    fi
}

run_server() {
    args=("$@")
    python3 "${TESTS_DIR}/utils/mock_server.py" "${args[@]/#/}" >>${TESTS_DIR}/tmp/server.log &
    sleep 0.1
}

check_logs() {
    cur_file="${0##*/}"
    cur_file="${cur_file%.sh}"
    diffOutput=$(diff --strip-trailing-cr ${LOG_DIR}/${cur_file}.txt server.log 2>&1 || echo failed)
    if [ ! -z "$diffOutput" ]; then
        showStatus info 'Server logs mismatch.'
        echo "$diffOutput"
        exit 1
    fi
}

export -f setColor showStatus copy_text get_copied_text clear_clipboard run_server check_logs

exitCode=0
passCnt=0
failCnt=0
for script in scripts/*.sh; do
    chmod +x "$script"
    passed=
    attempts=3 # number of retries before failure
    for attempt in $(seq "$attempts"); do
        if timeout 60 "$script" "$program"; then
            passed=1
            showStatus "$script" pass
            break
        fi
        scriptExitCode="${PIPESTATUS[0]}"
        if [ "$scriptExitCode" == '124' ]; then
            showStatus "$script" info 'Test timeout'
        fi
        attemt_msg="Attempt ${attempt} / ${attempts} failed."
        if [ "$attempt" != "$attempts" ]; then
            attemt_msg="${attemt_msg} trying again ..."
        fi
        showStatus "$script" warn "$attemt_msg"
    done
    if [ "$passed" = '1' ]; then
        passCnt="$((passCnt + 1))"
    else
        exitCode=1
        failCnt="$((failCnt + 1))"
        showStatus "$script" fail
    fi
    "$program" -s &>/dev/null
    rm -rf tmp
done

clear_clipboard

totalTests="$((passCnt + failCnt))"
test_s='tests'
if [ "$totalTests" = '1' ]; then
    test_s='test'
fi
ftest_s='tests'
if [ "$failCnt" = '1' ]; then
    ftest_s='test'
fi

if [ "$failCnt" = '0' ]; then
    setColor 'green' 'bold'
    echo '======================================================'
    setColor 'green' 'bold'
    echo "Passed all ${totalTests} ${test_s}."
    setColor 'green' 'bold'
    echo '======================================================'
else
    setColor 'red' 'bold'
    echo '======================================================'
    setColor 'red' 'bold'
    echo -n "Failed ${failCnt} ${ftest_s}."
    setColor 'reset'
    echo " Passed ${passCnt} out of ${totalTests} ${test_s}."
    setColor 'red' 'bold'
    echo '======================================================'
fi
setColor 'reset'

exit "$exitCode"
