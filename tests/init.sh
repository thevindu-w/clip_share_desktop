set -e

program="$1"

shopt -s expand_aliases

alias showStatus="showStatus $0"
alias check_logs="check_logs $0"

"$program" -s &>/dev/null
rm -rf tmp
cp -r config tmp
cd tmp
