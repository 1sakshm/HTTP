#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
server="$project_dir/build/debug/cserve"
port=${CSERVE_TEST_PORT:-18080}
stdout_log=$(mktemp)
stderr_log=$(mktemp)
server_pid=

cleanup() {
    if [ -n "$server_pid" ] && kill -0 "$server_pid" 2>/dev/null; then
        kill -TERM "$server_pid" 2>/dev/null || true
        wait "$server_pid" 2>/dev/null || true
    fi
    rm -f "$stdout_log" "$stderr_log"
}
trap cleanup EXIT INT TERM

if [ ! -x "$server" ]; then
    echo "missing server executable: $server" >&2
    exit 1
fi

"$server" --host 127.0.0.1 --port "$port" >"$stdout_log" 2>"$stderr_log" &
server_pid=$!

python3 - "$port" <<'PY'
import socket
import sys
import time

port = int(sys.argv[1])
expected = (
    b"HTTP/1.1 200 OK\r\n"
    b"Content-Length: 13\r\n"
    b"Content-Type: text/plain\r\n"
    b"Connection: close\r\n"
    b"\r\n"
    b"Hello, world!"
)

deadline = time.monotonic() + 5.0
while True:
    try:
        client = socket.create_connection(("127.0.0.1", port), timeout=1.0)
        break
    except OSError:
        if time.monotonic() >= deadline:
            raise
        time.sleep(0.05)

with client:
    client.sendall(b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
    chunks = []
    while True:
        chunk = client.recv(4096)
        if not chunk:
            break
        chunks.append(chunk)

actual = b"".join(chunks)
if actual != expected:
    raise AssertionError(f"unexpected response:\n{actual!r}")
PY

kill -TERM "$server_pid"
wait "$server_pid"
server_pid=

echo "stage 1 smoke test passed"
