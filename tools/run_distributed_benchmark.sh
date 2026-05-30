#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

MODE="${1:-all}" # bench5|bench6|all

JETSON_HOST="${JETSON_HOST:-10.168.5.9}"
JETSON_USER="${JETSON_USER:-sky}"
JETSON_PASS="${JETSON_PASS:-sky123456}"
JETSON_ZOO_DIR="${JETSON_ZOO_DIR:-/home/sky/zoo}"
JETSON_CMAKE_BIN="${JETSON_CMAKE_BIN:-/home/sky/tools/cmake-3.28.6-linux-aarch64/bin/cmake}"
JETSON_BUILD_EXAMPLES="${JETSON_BUILD_EXAMPLES:-ON}"

LOCAL_BUILD_DIR="${LOCAL_BUILD_DIR:-build}"
REMOTE_BUILD_DIR="${REMOTE_BUILD_DIR:-build}"

RPC_ROUNDS="${RPC_ROUNDS:-200}"
PUBSUB_ROUNDS="${PUBSUB_ROUNDS:-200}"

LOCAL_BIN_STAGE="$REPO_ROOT/stage/bin/e2e_performance_benchmark"
LOCAL_BIN_BUILD="$REPO_ROOT/$LOCAL_BUILD_DIR/tests/benchmark/e2e_performance_benchmark"
REMOTE_BIN_STAGE="$JETSON_ZOO_DIR/stage/bin/e2e_performance_benchmark"
REMOTE_BIN_BUILD="$JETSON_ZOO_DIR/$REMOTE_BUILD_DIR/tests/benchmark/e2e_performance_benchmark"
LOCAL_BIN=""
REMOTE_BIN=""

RUN_ID="$(date +%s)"
RPC_SERVER_NAME="bench5_srv_lan_${RUN_ID}"
RPC_CLIENT_NAME="bench5_cli_lan_${RUN_ID}"
RPC_TARGET_NAME="bench5_target_lan_${RUN_ID}"
RPC_TOPIC="bench5/topic/lan/${RUN_ID}"

PUB_NAME="bench6_pub_lan_${RUN_ID}"
SUB_NAME="bench6_sub_lan_${RUN_ID}"
PUB_TOPIC="bench6/topic/lan/${RUN_ID}"

log() {
    printf '[distributed-bench] %s\n' "$*"
}

print_usage() {
        cat <<EOF
Usage: $0 [bench5|bench6|all]

Modes:
    bench5   Run distributed RPC latency benchmark only
    bench6   Run distributed pub/sub latency benchmark only
    all      Run bench5 then bench6 (default)

Key environment variables:
    JETSON_HOST               Jetson IP/hostname (default: 10.168.5.9)
    JETSON_USER               SSH username (default: sky)
    JETSON_PASS               SSH password fallback (default: sky123456)
    JETSON_ZOO_DIR            Remote repo path (default: /home/sky/zoo)
    JETSON_CMAKE_BIN          Remote cmake path
    JETSON_BUILD_EXAMPLES     Remote -DZOO_BUILD_EXAMPLES value (default: ON)
    LOCAL_BUILD_DIR           Local build dir (default: build)
    REMOTE_BUILD_DIR          Remote build dir (default: build)
    RPC_ROUNDS                bench5 request count (default: 200)
    PUBSUB_ROUNDS             bench6 message count (default: 200)
EOF
}

require_cmd() {
    command -v "$1" >/dev/null 2>&1 || {
        log "Missing required command: $1"
        exit 1
    }
}

resolve_local_ip() {
    ip route get "$JETSON_HOST" 2>/dev/null | awk '{for (i = 1; i <= NF; ++i) { if ($i == "src") { print $(i+1); exit } }}'
}

remote_exec() {
    local cmd="$1"
    if command -v sshpass >/dev/null 2>&1; then
        sshpass -p "$JETSON_PASS" ssh -o StrictHostKeyChecking=no "$JETSON_USER@$JETSON_HOST" "$cmd"
    else
        ssh -o StrictHostKeyChecking=no "$JETSON_USER@$JETSON_HOST" "$cmd"
    fi
}

prepare_remote_binary() {
    log "Preparing benchmark binary on Jetson (${JETSON_USER}@${JETSON_HOST})"
    remote_exec "set -e; CMAKE_BIN='cmake'; if [ -x '$JETSON_CMAKE_BIN' ]; then CMAKE_BIN='$JETSON_CMAKE_BIN'; fi; cd '$JETSON_ZOO_DIR'; rm -rf '$REMOTE_BUILD_DIR'; mkdir -p '$REMOTE_BUILD_DIR'; cd '$REMOTE_BUILD_DIR'; \"\$CMAKE_BIN\" .. -DZOO_BUILD_EXAMPLES='$JETSON_BUILD_EXAMPLES'; \"\$CMAKE_BIN\" --build . --target e2e_performance_benchmark -j\"\$(nproc)\""

    if remote_exec "test -x '$REMOTE_BIN_STAGE'"; then
        REMOTE_BIN="$REMOTE_BIN_STAGE"
    elif remote_exec "test -x '$REMOTE_BIN_BUILD'"; then
        REMOTE_BIN="$REMOTE_BIN_BUILD"
    else
        log "Remote benchmark binary not found after build"
        exit 1
    fi

    log "Using remote benchmark binary: ${REMOTE_BIN}"
}

cleanup_pid() {
    local pid="$1"
    if kill -0 "$pid" >/dev/null 2>&1; then
        kill "$pid" >/dev/null 2>&1 || true
        wait "$pid" 2>/dev/null || true
    fi
}

run_bench5() {
    local local_ip="$1"
    local server_target="${RPC_TARGET_NAME}"
    local client_output

    log "Starting bench5 server locally on target=${server_target}"
    ZOO_BENCH_LOCAL_ADDRESS="$local_ip" "$LOCAL_BIN" bench5-server "$RPC_SERVER_NAME" "$server_target" "$RPC_TOPIC" &
    local server_pid=$!

    sleep 2

    log "Running bench5 client on Jetson"
    client_output="$(remote_exec "set -e; ZOO_BENCH_LOCAL_ADDRESS='$JETSON_HOST' '$REMOTE_BIN' bench5-client '$RPC_CLIENT_NAME' '$server_target' '$RPC_TOPIC' '$RPC_ROUNDS'")"
    printf '%s\n' "$client_output"
    if echo "$client_output" | grep -q "no successful samples"; then
        log "bench5 failed: no successful RPC samples were recorded"
        cleanup_pid "$server_pid"
        return 1
    fi
    log "bench5 completed successfully"
    cleanup_pid "$server_pid"
    return 0

}

run_bench6() {
    local local_ip="$1"
    local publisher_target="${PUB_NAME}"
    local subscriber_output
    local subscriber_log
    local subscriber_job_pid
    local subscriber_status

    subscriber_log="$(mktemp /tmp/bench6_subscriber.XXXXXX.log)"

    log "Starting bench6 subscriber on Jetson"
    (
        remote_exec "set -e; ZOO_BENCH_LOCAL_ADDRESS='$JETSON_HOST' '$REMOTE_BIN' bench6-subscriber '$SUB_NAME' '$PUB_NAME' '$PUB_TOPIC' '$PUBSUB_ROUNDS'"
    ) >"$subscriber_log" 2>&1 &
    subscriber_job_pid=$!

    sleep 2

    log "Starting bench6 publisher locally on target=${publisher_target}"
    ZOO_BENCH_LOCAL_ADDRESS="$local_ip" "$LOCAL_BIN" bench6-publisher "$PUBSUB_ROUNDS" "$PUB_NAME" "$publisher_target" "$PUB_TOPIC" 64 1000 &
    local pub_pid=$!

    wait "$subscriber_job_pid"
    subscriber_status=$?
    subscriber_output="$(cat "$subscriber_log")"
    rm -f "$subscriber_log"
    printf '%s\n' "$subscriber_output"
    if [[ "$subscriber_status" -ne 0 ]]; then
        log "bench6 failed: subscriber command exited with status $subscriber_status"
        cleanup_pid "$pub_pid"
        return 1
    fi
    if echo "$subscriber_output" | grep -q "no successful samples"; then
        log "bench6 failed: no successful pub/sub samples were recorded"
        cleanup_pid "$pub_pid"
        return 1
    fi
    log "bench6 completed successfully"
    cleanup_pid "$pub_pid"
    return 0

}

main() {
    require_cmd ssh
    require_cmd cmake
    require_cmd ip

    case "$MODE" in
        --help|-h|help)
            print_usage
            exit 0
            ;;
    esac

    if [ -x "$LOCAL_BIN_STAGE" ]; then
        LOCAL_BIN="$LOCAL_BIN_STAGE"
    elif [ -x "$LOCAL_BIN_BUILD" ]; then
        LOCAL_BIN="$LOCAL_BIN_BUILD"
    fi

    if [ -z "$LOCAL_BIN" ]; then
        log "Local benchmark binary not found in:"
        log "  - $LOCAL_BIN_STAGE"
        log "  - $LOCAL_BIN_BUILD"
        log "Build locally first: cmake --build $LOCAL_BUILD_DIR --target e2e_performance_benchmark"
        exit 1
    fi

    log "Using local benchmark binary: ${LOCAL_BIN}"

    local local_ip
    local_ip="$(resolve_local_ip)"
    if [ -z "$local_ip" ]; then
        log "Failed to resolve local LAN IP toward ${JETSON_HOST}"
        exit 1
    fi

    log "Using local LAN IP: ${local_ip}"

    prepare_remote_binary

    case "$MODE" in
        bench5)
            run_bench5 "$local_ip"
            ;;
        bench6)
            run_bench6 "$local_ip"
            ;;
        all)
            run_bench5 "$local_ip"
            run_bench6 "$local_ip"
            ;;
        *)
            print_usage
            exit 1
            ;;
    esac

    log "Distributed benchmark run finished successfully"
}

main "$@"
