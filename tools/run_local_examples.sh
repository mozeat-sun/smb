#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

MODE="${1:-all}"  # tcp|udp|pubsub|shm|all

BUILD_DIR="${LOCAL_BUILD_DIR:-build}"
BIN_DIR="${REPO_ROOT}/stage/bin"
LOG_DIR="${REPO_ROOT}/artifacts/local_examples"

DEMO_ID="$(date +%s)"
TCP_TARGET="local_tcp_${DEMO_ID}"
UDP_TARGET="local_udp_${DEMO_ID}"
PUBSUB_TOPIC="local/topic/${DEMO_ID}"
SHM_CHANNEL="zoo_shm_local_${DEMO_ID}"
SHM_SERVER_NAME="shm_local_server_${DEMO_ID}"

TCP_SERVER_BIN="${BIN_DIR}/zoo_example_server"
TCP_CLIENT_BIN="${BIN_DIR}/zoo_example_client"
UDP_SERVER_BIN="${BIN_DIR}/zoo_example_udp_server"
UDP_CLIENT_BIN="${BIN_DIR}/zoo_example_udp_client"
PUB_BIN="${BIN_DIR}/zoo_example_publisher"
SUB_BIN="${BIN_DIR}/zoo_example_subscriber"
SHM_SERVER_BIN="${BIN_DIR}/zoo_example_shm_server"
SHM_CLIENT_BIN="${BIN_DIR}/zoo_example_shm_client"

server_pid=""
sub_pid=""
pub_pid=""

log() {
    printf '[local-examples] %s\n' "$*"
}

usage() {
    cat <<EOF
Usage: $0 [tcp|udp|pubsub|shm|all]

Runs Zoo examples on a single local machine.

Modes:
  tcp      Run client/server (default transport) example pair
  udp      Run UDP client/server example pair
  pubsub   Run publisher/subscriber example pair
  shm      Run SHM client/server example pair
  all      Run all modes in order (default)

Environment variables:
  LOCAL_BUILD_DIR     Build directory used for cmake configure/build (default: build)
  EXAMPLES_LOG_LEVEL  Log level passed to examples (default: 2)
  EXAMPLES_TIMEOUT_S  Timeout per client phase in seconds (default: 20)
  SKIP_BUILD          If set to 1, skips cmake configure/build checks

Examples:
  bash tools/run_local_examples.sh all
  EXAMPLES_LOG_LEVEL=1 bash tools/run_local_examples.sh udp
  SKIP_BUILD=1 bash tools/run_local_examples.sh shm
EOF
}

cleanup() {
    if [[ -n "${server_pid}" ]] && kill -0 "${server_pid}" 2>/dev/null; then
        kill "${server_pid}" 2>/dev/null || true
        wait "${server_pid}" 2>/dev/null || true
    fi

    if [[ -n "${sub_pid}" ]] && kill -0 "${sub_pid}" 2>/dev/null; then
        kill "${sub_pid}" 2>/dev/null || true
        wait "${sub_pid}" 2>/dev/null || true
    fi

    if [[ -n "${pub_pid}" ]] && kill -0 "${pub_pid}" 2>/dev/null; then
        kill "${pub_pid}" 2>/dev/null || true
        wait "${pub_pid}" 2>/dev/null || true
    fi
}

trap cleanup EXIT

require_mode() {
    case "$MODE" in
        tcp|udp|pubsub|shm|all) ;;
        -h|--help|help)
            usage
            exit 0
            ;;
        *)
            log "Invalid mode: $MODE"
            usage
            exit 1
            ;;
    esac
}

build_examples_if_needed() {
    if [[ "${SKIP_BUILD:-0}" == "1" ]]; then
        log "Skipping build step because SKIP_BUILD=1"
        return
    fi

    log "Configuring with examples enabled"
    cmake -S "$REPO_ROOT" -B "$REPO_ROOT/$BUILD_DIR" -DZOO_BUILD_EXAMPLES=ON >/dev/null

    log "Building example targets"
    cmake --build "$REPO_ROOT/$BUILD_DIR" -j"$(nproc)" >/dev/null
}

check_bins() {
    local missing=0
    local bins=(
        "$TCP_SERVER_BIN" "$TCP_CLIENT_BIN"
        "$UDP_SERVER_BIN" "$UDP_CLIENT_BIN"
        "$PUB_BIN" "$SUB_BIN"
        "$SHM_SERVER_BIN" "$SHM_CLIENT_BIN"
    )

    for bin in "${bins[@]}"; do
        if [[ ! -x "$bin" ]]; then
            log "Missing executable: $bin"
            missing=1
        fi
    done

    if [[ "$missing" -ne 0 ]]; then
        log "Build with: cmake -S . -B ${BUILD_DIR} -DZOO_BUILD_EXAMPLES=ON && cmake --build ${BUILD_DIR}"
        exit 1
    fi
}

run_tcp() {
    local lvl="${EXAMPLES_LOG_LEVEL:-2}"
    local timeout_s="${EXAMPLES_TIMEOUT_S:-20}"
    local server_log="$LOG_DIR/tcp_server.log"
    local client_log="$LOG_DIR/tcp_client.log"

    log "[tcp] starting server target=$TCP_TARGET"
    "$TCP_SERVER_BIN" -t "$TCP_TARGET" -l "$lvl" >"$server_log" 2>&1 &
    server_pid=$!

    log "[tcp] running client"
    if ! timeout "${timeout_s}s" "$TCP_CLIENT_BIN" -t "$TCP_TARGET" -p "hello tcp local" -l "$lvl" >"$client_log" 2>&1; then
        log "[tcp] client failed"
        tail -n 40 "$server_log" || true
        tail -n 40 "$client_log" || true
        return 1
    fi

    if ! grep -q "Received reply" "$client_log"; then
        log "[tcp] missing success marker in client log"
        tail -n 40 "$client_log" || true
        return 1
    fi

    kill "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
    server_pid=""
    log "[tcp] PASS"
}

run_udp() {
    local lvl="${EXAMPLES_LOG_LEVEL:-2}"
    local timeout_s="${EXAMPLES_TIMEOUT_S:-20}"
    local server_log="$LOG_DIR/udp_server.log"
    local client_log="$LOG_DIR/udp_client.log"

    log "[udp] starting server target=$UDP_TARGET"
    "$UDP_SERVER_BIN" -t "$UDP_TARGET" -l "$lvl" >"$server_log" 2>&1 &
    server_pid=$!

    log "[udp] running client"
    if ! timeout "${timeout_s}s" "$UDP_CLIENT_BIN" -t "$UDP_TARGET" -p "hello udp local" -l "$lvl" >"$client_log" 2>&1; then
        log "[udp] client failed"
        tail -n 40 "$server_log" || true
        tail -n 40 "$client_log" || true
        return 1
    fi

    if ! grep -q "Received reply" "$client_log"; then
        log "[udp] missing success marker in client log"
        tail -n 40 "$client_log" || true
        return 1
    fi

    kill "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
    server_pid=""
    log "[udp] PASS"
}

run_pubsub() {
    local lvl="${EXAMPLES_LOG_LEVEL:-2}"
    local timeout_s="${EXAMPLES_TIMEOUT_S:-20}"
    local observe_s="${EXAMPLES_PUBSUB_OBSERVE_S:-4}"
    local sub_log="$LOG_DIR/pubsub_subscriber.log"
    local pub_log="$LOG_DIR/pubsub_publisher.log"

    log "[pubsub] starting subscriber"
    "$SUB_BIN" "$lvl" >"$sub_log" 2>&1 &
    sub_pid=$!

    # Give subscriber time to initialize and subscribe before publishing
    sleep 2

    log "[pubsub] starting publisher"
    "$PUB_BIN" "$lvl" >"$pub_log" 2>&1 &
    pub_pid=$!

    sleep "$observe_s"

    if ! kill -0 "$pub_pid" 2>/dev/null; then
        log "[pubsub] publisher exited unexpectedly"
        tail -n 40 "$sub_log" || true
        tail -n 40 "$pub_log" || true
        return 1
    fi

    if ! grep -Eq "Published message|Publisher running" "$pub_log"; then
        log "[pubsub] publisher did not emit expected activity"
        tail -n 40 "$sub_log" || true
        tail -n 40 "$pub_log" || true
        return 1
    fi

    if ! grep -Eq "msg_id:|PUB callback|Received PUB message|Subscriber running" "$sub_log"; then
        log "[pubsub] subscriber did not observe published traffic"
        tail -n 40 "$sub_log" || true
        tail -n 40 "$pub_log" || true
        return 1
    fi

    kill "$pub_pid" 2>/dev/null || true
    wait "$pub_pid" 2>/dev/null || true
    pub_pid=""

    kill "$sub_pid" 2>/dev/null || true
    wait "$sub_pid" 2>/dev/null || true
    sub_pid=""
    log "[pubsub] PASS"
}

run_shm() {
    local lvl="${EXAMPLES_LOG_LEVEL:-2}"
    local timeout_s="${EXAMPLES_TIMEOUT_S:-20}"
    local server_log="$LOG_DIR/shm_server.log"
    local client_log="$LOG_DIR/shm_client.log"

    log "[shm] starting server channel=$SHM_CHANNEL server=$SHM_SERVER_NAME"
    "$SHM_SERVER_BIN" --channel "$SHM_CHANNEL" --server-name "$SHM_SERVER_NAME" --log-level "$lvl" >"$server_log" 2>&1 &
    server_pid=$!

    log "[shm] running client"
    if ! timeout "${timeout_s}s" "$SHM_CLIENT_BIN" --channel "$SHM_CHANNEL" --server-name "$SHM_SERVER_NAME" --payload "hello shm local" --log-level "$lvl" >"$client_log" 2>&1; then
        log "[shm] client failed"
        tail -n 40 "$server_log" || true
        tail -n 40 "$client_log" || true
        return 1
    fi

    if ! grep -q "Received reply" "$client_log"; then
        log "[shm] missing success marker in client log"
        tail -n 40 "$client_log" || true
        return 1
    fi

    kill "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
    server_pid=""
    log "[shm] PASS"
}

main() {
    require_mode
    mkdir -p "$LOG_DIR"

    build_examples_if_needed
    check_bins

    case "$MODE" in
        tcp) run_tcp ;;
        udp) run_udp ;;
        pubsub) run_pubsub ;;
        shm) run_shm ;;
        all)
            run_tcp
            run_udp
            run_pubsub
            run_shm
            ;;
    esac

    log "All requested local example runs passed. Logs: $LOG_DIR"
}

main
