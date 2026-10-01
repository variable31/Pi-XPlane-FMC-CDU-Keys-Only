#!/bin/sh
# Drives packaging/display/.../display-launch against a local web server
# standing in for WebFMC, with the browser replaced by a stub that logs.
# Checks: waiting page -> CDU when X-Plane appears -> waiting after it is
# lost -> CDU again -> browser restart after a crash -> clean stop.
set -u
SRC=${1:?usage: test_display_launch.sh SOURCE_DIR}
T=$(mktemp -d)
LOG=$T/kiosk.log
fail() { echo "FAIL: $*"; echo "--- kiosk log"; cat "$LOG"; echo "--- launcher log"; cat "$T/launch.log"; cleanup; exit 1; }
cleanup() {
	[ -n "${LP:-}" ] && kill "$LP" 2>/dev/null
	[ -n "${SP:-}" ] && kill "$SP" 2>/dev/null
	pkill -f "$T/kiosk" 2>/dev/null
	wait 2>/dev/null
	rm -rf "$T"
}

# Stub browser: logs START/STOP and stays up until killed.
cat > "$T/kiosk" <<'K'
#!/bin/sh
echo "START $1" >> "$KLOG"
trap 'echo "STOP $1" >> "$KLOG"; exit 0' TERM
while :; do sleep 0.2; done
K
chmod +x "$T/kiosk"

PORT=$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1])')
printf '[webfmc]\nhost =\nport = %s\npath = /cdu\n[display]\nlost_seconds = 2\n' "$PORT" > "$T/display.conf"
mkdir "$T/www"; echo ok > "$T/www/cdu"

serve() { (cd "$T/www" && exec python3 -m http.server "$PORT" --bind 127.0.0.1 >/dev/null 2>&1) & SP=$!; sleep 1; }
# wait_for PATTERN: up to 10 s for a line in the kiosk log
wait_for() { i=0; while [ $i -lt 50 ]; do grep -q "$1" "$LOG" 2>/dev/null && return 0; sleep 0.2; i=$((i+1)); done; return 1; }
count() { n=$(grep -c "$1" "$LOG" 2>/dev/null) || true; echo "${n:-0}"; }

KLOG=$LOG FSD_KIOSK=$T/kiosk FSD_CONF=$T/display.conf FSD_HOST_FILE=$T/xplane-host \
	FSD_WAITING=/w.html FSD_POLL=1 \
	sh "$SRC/packaging/display/usr/lib/flight-simulator/display-launch" > "$T/launch.log" 2>&1 &
LP=$!
export KLOG=$LOG

wait_for "START file:///w.html" || fail "waiting page not shown at start"

# X-Plane known but WebFMC not answering yet: stay on waiting page.
echo 127.0.0.1 > "$T/xplane-host"; sleep 2
[ "$(count "START http")" = 0 ] || fail "switched to CDU before WebFMC answered"

serve
wait_for "START http://127.0.0.1:$PORT/cdu" || fail "CDU not shown once WebFMC answered"
grep -q "STOP file:///w.html" "$LOG" || fail "waiting page not closed"

kill "$SP"; SP=""
i=0; while [ $i -lt 50 ] && [ "$(count "START file:///w.html")" -lt 2 ]; do sleep 0.2; i=$((i+1)); done
[ "$(count "START file:///w.html")" -ge 2 ] || fail "waiting page not shown after X-Plane was lost"

serve
i=0; while [ $i -lt 50 ] && [ "$(count "START http")" -lt 2 ]; do sleep 0.2; i=$((i+1)); done
[ "$(count "START http")" -ge 2 ] || fail "CDU not shown again after X-Plane came back"

# Browser crash: launcher must start it again.
pkill -f "$T/kiosk" ; i=0
while [ $i -lt 50 ] && [ "$(count "START http")" -lt 3 ]; do sleep 0.2; i=$((i+1)); done
[ "$(count "START http")" -ge 3 ] || fail "browser not restarted after it exited"

kill "$LP"; wait "$LP" 2>/dev/null; LP=""
sleep 0.5
[ "$(count START)" = "$(count STOP)" ] || [ "$(( $(count START) - 1 ))" = "$(count STOP)" ] || fail "browser left running"
echo "display launcher: all checks passed"
cleanup
exit 0
