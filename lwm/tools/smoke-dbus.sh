#!/bin/sh
set -eu

if [ "${1:-}" != "--inner" ]; then
    exec dbus-run-session -- "$0" --inner
fi

cd "$(dirname "$0")/.."
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

./lwm >"$log" 2>&1 &
pid=$!

for _ in 1 2 3 4 5 6 7 8 9 10; do
    if grep -q "WAYLAND_DISPLAY=" "$log"; then
        break
    fi
    sleep 0.2
done

busctl --user status >/dev/null
