#!/bin/sh
set -eu

if [ "${1:-}" != "--inner" ]; then
    exec dbus-run-session -- "$0" --inner
fi

cd "$(dirname "$0")/.."
command -v waybar >/dev/null

log=$(mktemp)
cfg=$(mktemp)
css=$(mktemp)
pid=''
wpid=''
cleanup() {
    if [ -n "$wpid" ]; then
        kill "$wpid" 2>/dev/null || true
        wait "$wpid" 2>/dev/null || true
    fi
    if [ -n "$pid" ]; then
        kill "$pid" 2>/dev/null || true
        wait "$pid" 2>/dev/null || true
    fi
    rm -f "$log" "$cfg" "$css"
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

cat >"$cfg" <<'EOF'
[{
  "layer": "top",
  "position": "top",
  "height": 24,
  "modules-left": ["clock"]
}]
EOF

cat >"$css" <<'EOF'
* { border: none; min-height: 0; }
window#waybar { background: #111111; color: #dddddd; }
EOF

WAYLAND_DISPLAY="$way" XDG_CURRENT_DESKTOP=lwm XDG_SESSION_TYPE=wayland waybar -c "$cfg" -s "$css" >/tmp/lwm-waybar.log 2>&1 &
wpid=$!
sleep 1
kill -0 "$wpid"
