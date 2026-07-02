#!/usr/bin/env sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

if [ "${INCLUDE+x}" ]; then
    INCLUDE="$script_dir/include;$INCLUDE"
else
    INCLUDE="$script_dir/include"
fi
export INCLUDE

exec "$script_dir/fasmg.x64" -i"Include('fasm2.inc')" "$@"
