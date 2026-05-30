#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BIN_DIR="$REPO_ROOT/stage/bin"

MODE="${1:-all}"
ROLE="${2:-}"

log() {
    printf '[examples-start] %s\n' "$*"
}

usage() {
    cat <<EOF
Usage:
  bash examples/start.sh [tcp|udp|pubsub|shm|all]
  bash examples/start.sh <mode> <role> [extra args]

Modes:
  tcp, udp, shm  Roles: server | client
  pubsub         Roles: publisher | subscriber
  all            Runs all local example pairs

If only mode is provided, this script delegates to tools/run_local_examples.sh.

Examples:
  bash examples/start.sh
  bash examples/start.sh udp
  bash examples/start.sh udp server
  bash examples/start.sh udp client
  UDP_TARGET=demo_udp bash examples/start.sh udp server -l 1
  SKIP_BUILD=1 bash examples/start.sh shm
EOF
}

if [[ "$MODE" == "-h" || "$MODE" == "--help" || "$MODE" == "help" ]]; then
    usage
    exit 0
fi

build_examples_if_needed() {
    if [[ "${SKIP_BUILD:-0}" == "1" ]]; then
        log "Skipping build because SKIP_BUILD=1"
        return
    fi

    log "Configuring with examples enabled"
    cmake -S "$REPO_ROOT" -B "$REPO_ROOT/build" -DZOO_BUILD_EXAMPLES=ON >/dev/null

    log "Building examples"
    cmake --build "$REPO_ROOT/build" -j"$(nproc)" >/dev/null
}

require_bin() {
    local bin="$1"
    if [[ ! -x "$bin" ]]; then
        build_examples_if_needed
    fi
    if [[ ! -x "$bin" ]]; then
        log "Missing executable: $bin"
        exit 1
    fi
}

run_role() {
    local mode="$1"
    local role="$2"
    shift 2

    case "$mode:$role" in
        udp:server)
            local bin="$BIN_DIR/zoo_example_udp_server"
            require_bin "$bin"
            exec "$bin" -t "${UDP_TARGET:-demo_udp}" -l "${EXAMPLES_LOG_LEVEL:-2}" "$@"
            ;;
        udp:client)
            local bin="$BIN_DIR/zoo_example_udp_client"
            require_bin "$bin"
            exec "$bin" -t "${UDP_TARGET:-demo_udp}" -p "${UDP_PAYLOAD:-hello udp}" -l "${EXAMPLES_LOG_LEVEL:-2}" "$@"
            ;;
        tcp:server)
            local bin="$BIN_DIR/zoo_example_server"
            require_bin "$bin"
            exec "$bin" -t "${TCP_TARGET:-demo_tcp}" -l "${EXAMPLES_LOG_LEVEL:-2}" "$@"
            ;;
        tcp:client)
            local bin="$BIN_DIR/zoo_example_client"
            require_bin "$bin"
            exec "$bin" -t "${TCP_TARGET:-demo_tcp}" -p "${TCP_PAYLOAD:-hello tcp}" -l "${EXAMPLES_LOG_LEVEL:-2}" "$@"
            ;;
        shm:server)
            local bin="$BIN_DIR/zoo_example_shm_server"
            require_bin "$bin"
            exec "$bin" --channel "${SHM_CHANNEL:-zoo_shm_demo}" --server-name "${SHM_SERVER_NAME:-shm_demo_server}" --log-level "${EXAMPLES_LOG_LEVEL:-2}" "$@"
            ;;
        shm:client)
            local bin="$BIN_DIR/zoo_example_shm_client"
            require_bin "$bin"
            exec "$bin" --channel "${SHM_CHANNEL:-zoo_shm_demo}" --server-name "${SHM_SERVER_NAME:-shm_demo_server}" --payload "${SHM_PAYLOAD:-hello shm}" --log-level "${EXAMPLES_LOG_LEVEL:-2}" "$@"
            ;;
        pubsub:publisher)
            local bin="$BIN_DIR/zoo_example_publisher"
            require_bin "$bin"
            exec "$bin" "${EXAMPLES_LOG_LEVEL:-2}" "$@"
            ;;
        pubsub:subscriber)
            local bin="$BIN_DIR/zoo_example_subscriber"
            require_bin "$bin"
            exec "$bin" "${EXAMPLES_LOG_LEVEL:-2}" "$@"
            ;;
        *)
            log "Unsupported mode/role: $mode $role"
            usage
            exit 1
            ;;
    esac
}

case "$MODE" in
    tcp|udp|pubsub|shm|all)
        ;;
    *)
        log "Invalid mode: $MODE"
        usage
        exit 1
        ;;
esac

if [[ -z "$ROLE" ]]; then
    exec bash "$REPO_ROOT/tools/run_local_examples.sh" "$MODE"
fi

if [[ "$MODE" == "all" ]]; then
    log "Mode 'all' does not support role selection"
    usage
    exit 1
fi

shift 2
run_role "$MODE" "$ROLE" "$@"