set -e

script="${0##*/}"
script="${script%.sh}"
interface="${script:${#script}-3}"
[ "$interface" = 'cli' ] || [ "$interface" = 'web' ] || exit 1

. "scripts/interface_common/${script::${#script}-4}.sh"
