#!/bin/sh
set -eu

if [ "${1:-}" != "--inner" ]; then
    exec dbus-run-session -- "$0" --inner
fi

cd "$(dirname "$0")/.."
command -v Xwayland >/dev/null

log=$(mktemp)
pid=''
cleanup() {
    if [ -n "$pid" ]; then
        kill "$pid" 2>/dev/null || true
        wait "$pid" 2>/dev/null || true
    fi
    rm -f "$log"
}
trap cleanup EXIT INT TERM

LWM_XWAYLAND=1 ./lwm >"$log" 2>&1 &
pid=$!

for _ in 1 2 3 4 5 6 7 8 9 10; do
    if grep -q "Xwayland on DISPLAY=" "$log"; then
        exit 0
    fi
    sleep 0.2
done

echo 'lwm: failed to bring up Xwayland' >&2
cat "$log" >&2
exit 1
