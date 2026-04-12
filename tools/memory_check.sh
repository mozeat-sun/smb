#!/usr/bin/env bash

# ============================================================================
# Zoo Project Valgrind Memory Check Tool
# -----------------------------------------------------------------------------
# Runs selected Zoo executables (or a custom command) under Valgrind memcheck
# and stores logs under artifacts/quality/valgrind.
#
# Modes:
#   1) Preset mode for transport examples (udp-client, shm-client, both)
#   2) Custom command mode via --cmd
#   3) Long-run preset mode (udp-client-long, shm-client-long)
#
# Usage examples:
#   bash tools/memory_check.sh --preset udp-client
#   bash tools/memory_check.sh --preset both --timeout 45
#   bash tools/memory_check.sh --preset udp-client-long
#   bash tools/memory_check.sh --preset both-long
#   bash tools/memory_check.sh --preset shm-client --iterations 1000
#   bash tools/memory_check.sh --cmd "./stage/bin/zoo_example_udp_client -t sea -p hello -l 1" --name udp_custom
#
# Exit codes:
#   0   No memory errors found
#   1   Script/config/runtime error
#   42  Valgrind detected memory errors (via --error-exitcode)
# ============================================================================

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DIR="${ROOT_DIR}/stage/bin"
OUT_DIR="${ROOT_DIR}/artifacts/quality/valgrind"

VALGRIND_ERROR_EXIT_CODE=42
DEFAULT_TIMEOUT_SEC=30
DEFAULT_ITERATIONS=1
LONG_TEST_MIN_ITERATIONS=1000
SERVER_WARMUP_SEC=2
ITERATION_GAP_SEC=0.05

UDP_SERVER_BIN="${BIN_DIR}/zoo_example_udp_server"
UDP_CLIENT_BIN="${BIN_DIR}/zoo_example_udp_client"
SHM_SERVER_BIN="${BIN_DIR}/zoo_example_shm_server"
SHM_CLIENT_BIN="${BIN_DIR}/zoo_example_shm_client"

preset=""
custom_cmd=""
name=""
timeout_sec="${DEFAULT_TIMEOUT_SEC}"
iterations="${DEFAULT_ITERATIONS}"

server_pid=""
server_log=""

usage() {
    cat <<EOF
Usage:
    bash tools/memory_check.sh --preset <udp-client|shm-client|both|udp-client-long|shm-client-long|both-long> [--timeout <sec>] [--iterations <n>] [--name <label>]
    bash tools/memory_check.sh --cmd "<command>" [--timeout <sec>] [--iterations <n>] [--name <label>]

Options:
  --preset      Built-in scenario to run under valgrind
  --cmd         Custom command to run under valgrind
  --timeout     Timeout in seconds (default: ${DEFAULT_TIMEOUT_SEC})
    --iterations  Repeat command N times and print summary (default: ${DEFAULT_ITERATIONS})
  --name        Friendly label used in output log filename
  -h, --help    Show this help
EOF
}

cleanup() {
    if [[ -n "${server_pid}" ]] && kill -0 "${server_pid}" 2>/dev/null; then
        kill "${server_pid}" 2>/dev/null || true
        wait "${server_pid}" 2>/dev/null || true
    fi
}
trap cleanup EXIT

require_exe() {
    local exe="$1"
    if [[ ! -x "${exe}" ]]; then
        echo "[ERROR] Missing executable: ${exe}"
        echo "[HINT] Build with: cmake -S . -B build -DZOO_BUILD_EXAMPLES=ON && cmake --build build"
        exit 1
    fi
}

run_under_valgrind() {
    local label="$1"
    local cmd="$2"

    mkdir -p "${OUT_DIR}"

    local ts
    ts="$(date -u +%Y%m%dT%H%M%SZ)"
    local log_file="${OUT_DIR}/${label}_${ts}.log"

    echo "[INFO] Running valgrind check: ${label}"
    echo "[INFO] Command: ${cmd}"
    echo "[INFO] Log: ${log_file}"

    set +e
    valgrind \
        --tool=memcheck \
        --leak-check=full \
        --show-leak-kinds=all \
        --errors-for-leak-kinds=definite,possible \
        --trace-children=yes \
        --child-silent-after-fork=yes \
        --track-origins=yes \
        --num-callers=30 \
        --error-exitcode="${VALGRIND_ERROR_EXIT_CODE}" \
        --log-file="${log_file}" \
        timeout "${timeout_sec}s" bash -lc "${cmd}"
    local rc=$?
    set -e

    if [[ ${rc} -eq 124 ]]; then
        echo "[FAIL] Timeout after ${timeout_sec}s"
        echo "[HINT] Increase timeout with --timeout"
        return 1
    fi

    if [[ ${rc} -eq ${VALGRIND_ERROR_EXIT_CODE} ]]; then
        echo "[FAIL] Valgrind detected memory errors"
        echo "[INFO] See log: ${log_file}"
        return ${VALGRIND_ERROR_EXIT_CODE}
    fi

    if [[ ${rc} -ne 0 ]]; then
        echo "[FAIL] Command exited with status ${rc}"
        echo "[INFO] See log: ${log_file}"
        return ${rc}
    fi

    echo "[PASS] No valgrind-detected memory errors"
    echo "[INFO] See log: ${log_file}"
    return 0
}

start_udp_server() {
    local target="$1"
    server_log="$(mktemp)"
    "${UDP_SERVER_BIN}" -t "${target}" -l 1 >"${server_log}" 2>&1 &
    server_pid=$!
    # Give server and discovery a brief warm-up window before client requests.
    sleep "${SERVER_WARMUP_SEC}"
}

start_shm_server() {
    local channel="$1"
    local server_name="$2"
    server_log="$(mktemp)"
    "${SHM_SERVER_BIN}" --channel "${channel}" --server-name "${server_name}" --log-level 1 >"${server_log}" 2>&1 &
    server_pid=$!
        # Give server and discovery a brief warm-up window before client requests.
        sleep "${SERVER_WARMUP_SEC}"
}

build_repeated_command() {
        local base_cmd="$1"
        local count="$2"

        cat <<EOF
i=1
success=0
failed=0
while [[ "\${i}" -le ${count} ]]; do
    if ${base_cmd} >/dev/null 2>&1; then
        success=\$((success + 1))
    else
        failed=\$((failed + 1))
    fi
    sleep ${ITERATION_GAP_SEC}
    i=\$((i + 1))
done
echo "[RESULT] sends=${count} success=\${success} failed=\${failed}"
if [[ "\${failed}" -ne 0 ]]; then
    echo "[WARN] Some sends failed during repeated test"
fi
true
EOF
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --preset)
                preset="${2:-}"
                shift 2
                ;;
            --cmd)
                custom_cmd="${2:-}"
                shift 2
                ;;
            --name)
                name="${2:-}"
                shift 2
                ;;
            --timeout)
                timeout_sec="${2:-}"
                shift 2
                ;;
            --iterations)
                iterations="${2:-}"
                shift 2
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                echo "[ERROR] Unknown argument: $1"
                usage
                exit 1
                ;;
        esac
    done

    if [[ -n "${preset}" && -n "${custom_cmd}" ]]; then
        echo "[ERROR] Use either --preset or --cmd, not both"
        exit 1
    fi

    if [[ -z "${preset}" && -z "${custom_cmd}" ]]; then
        echo "[ERROR] Missing required mode (--preset or --cmd)"
        usage
        exit 1
    fi

    if ! [[ "${timeout_sec}" =~ ^[0-9]+$ ]] || [[ "${timeout_sec}" -le 0 ]]; then
        echo "[ERROR] --timeout must be a positive integer"
        exit 1
    fi

    if ! [[ "${iterations}" =~ ^[0-9]+$ ]] || [[ "${iterations}" -le 0 ]]; then
        echo "[ERROR] --iterations must be a positive integer"
        exit 1
    fi
}

run_preset_udp_client() {
    require_exe "${UDP_SERVER_BIN}"
    require_exe "${UDP_CLIENT_BIN}"

    local target="vg_udp_${$}"
    start_udp_server "${target}"

    local label="${name:-udp_client}"
    local base_cmd="${UDP_CLIENT_BIN} -t ${target} -p 'hello udp memcheck' -l 1"
    local cmd
    if [[ "${iterations}" -gt 1 ]]; then
        cmd="$(build_repeated_command "${base_cmd}" "${iterations}")"
    else
        cmd="${base_cmd}"
    fi

    run_under_valgrind "${label}" "${cmd}"
}

run_preset_shm_client() {
    require_exe "${SHM_SERVER_BIN}"
    require_exe "${SHM_CLIENT_BIN}"

    local channel="vg_shm_${$}"
    local shm_server_name="vg_server_${$}"
    start_shm_server "${channel}" "${shm_server_name}"

    local label="${name:-shm_client}"
    local base_cmd="${SHM_CLIENT_BIN} --channel ${channel} --server-name ${shm_server_name} --payload 'hello shm memcheck' --log-level 1"
    local cmd
    if [[ "${iterations}" -gt 1 ]]; then
        cmd="$(build_repeated_command "${base_cmd}" "${iterations}")"
    else
        cmd="${base_cmd}"
    fi

    run_under_valgrind "${label}" "${cmd}"
}

run_preset_udp_client_long() {
    if [[ "${iterations}" -lt "${LONG_TEST_MIN_ITERATIONS}" ]]; then
        iterations="${LONG_TEST_MIN_ITERATIONS}"
    fi
    echo "[INFO] Running long UDP test with iterations=${iterations}"
    run_preset_udp_client
}

run_preset_shm_client_long() {
    if [[ "${iterations}" -lt "${LONG_TEST_MIN_ITERATIONS}" ]]; then
        iterations="${LONG_TEST_MIN_ITERATIONS}"
    fi
    echo "[INFO] Running long SHM test with iterations=${iterations}"
    run_preset_shm_client
}

run_preset_both_long() {
    if [[ "${iterations}" -lt "${LONG_TEST_MIN_ITERATIONS}" ]]; then
        iterations="${LONG_TEST_MIN_ITERATIONS}"
    fi
    echo "[INFO] Running long UDP+SHM test with iterations=${iterations}"
    run_preset_udp_client
    cleanup
    server_pid=""
    run_preset_shm_client
}

main() {
    parse_args "$@"

    if ! command -v valgrind >/dev/null 2>&1; then
        echo "[ERROR] valgrind not found in PATH"
        exit 1
    fi

    if [[ -n "${custom_cmd}" ]]; then
        local label="${name:-custom_cmd}"
        local cmd
        if [[ "${iterations}" -gt 1 ]]; then
            cmd="$(build_repeated_command "${custom_cmd}" "${iterations}")"
        else
            cmd="${custom_cmd}"
        fi
        run_under_valgrind "${label}" "${cmd}"
        exit $?
    fi

    case "${preset}" in
        udp-client)
            run_preset_udp_client
            ;;
        shm-client)
            run_preset_shm_client
            ;;
        both)
            run_preset_udp_client
            cleanup
            server_pid=""
            run_preset_shm_client
            ;;
        udp-client-long)
            run_preset_udp_client_long
            ;;
        shm-client-long)
            run_preset_shm_client_long
            ;;
        both-long)
            run_preset_both_long
            ;;
        *)
            echo "[ERROR] Unsupported preset: ${preset}"
            usage
            exit 1
            ;;
    esac
}

main "$@"
