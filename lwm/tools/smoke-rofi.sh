#!/bin/sh
set -eu

if [ "${1:-}" != "--inner" ]; then
    exec dbus-run-session -- "$0" --inner
fi

cd "$(dirname "$0")/.."
command -v rofi >/dev/null

log=$(mktemp)
pid=''
rpid=''
cleanup() {
    if [ -n "$rpid" ]; then
        kill "$rpid" 2>/dev/null || true
        wait "$rpid" 2>/dev/null || true
    fi
    if [ -n "$pid" ]; then
        kill "$pid" 2>/dev/null || true
        wait "$pid" 2>/dev/null || true
    fi
    rm -f "$log"
}
trap cleanup EXIT INT TERM

./lwm >"$log" 2>&1 &
pid=$!

way=''
for _ in 1 2 3 4 5 6 7 8 9 10; do
    way=$(sed -n 's/.*WAYLAND_DISPLAY=\([^ ]*\).*/\1/p' "$log" | tail -n 1)
    if [ -n "$way" ]; then
        break
    fi
    sleep 0.2
done

if [ -z "$way" ]; then
    echo 'lwm: failed to discover WAYLAND_DISPLAY' >&2
    exit 1
fi

WAYLAND_DISPLAY="$way" XDG_CURRENT_DESKTOP=lwm XDG_SESSION_TYPE=wayland rofi -show drun >/tmp/lwm-rofi.log 2>&1 &
rpid=$!
sleep 1
kill -0 "$rpid"
