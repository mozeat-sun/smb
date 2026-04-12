#!/usr/bin/env bash

# ============================================================================
# Zoo Project Transport Example Verification Script
# -----------------------------------------------------------------------------
# This script automates the verification of both UDP and SHM transport examples
# for the Zoo project. It launches the corresponding server and client binaries
# for each transport, checks for successful message exchange, and reports the
# result. Intended for CI, developer smoke tests, and regression checks.
#
# Key Features:
#   - Verifies both UDP and SHM transports in isolation
#   - Ensures all required binaries are present and executable
#   - Starts servers in the background, runs clients, and checks logs for success
#   - Cleans up background processes on exit or error
#   - Provides detailed error output for troubleshooting
#
# Usage:
#   bash tools/run_transport_examples.sh
#
# Requirements:
#   - Example binaries must be built and available in stage/bin
#   - Script must be run from the project root or tools directory
# ============================================================================


# Exit immediately on error, treat unset variables as errors, and fail on pipeline errors
set -euo pipefail


# Determine the absolute path to the project root (one level above this script)
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# Path to the directory containing built example binaries
BIN_DIR="${ROOT_DIR}/stage/bin"


# Absolute paths to the UDP and SHM server/client example binaries
UDP_SERVER_BIN="${BIN_DIR}/zoo_example_udp_server"
UDP_CLIENT_BIN="${BIN_DIR}/zoo_example_udp_client"
SHM_SERVER_BIN="${BIN_DIR}/zoo_example_shm_server"
SHM_CLIENT_BIN="${BIN_DIR}/zoo_example_shm_client"


# Ensure all required example binaries exist and are executable.
# If any are missing, print a helpful build command and exit with error.
for bin in "${UDP_SERVER_BIN}" "${UDP_CLIENT_BIN}" "${SHM_SERVER_BIN}" "${SHM_CLIENT_BIN}"; do
    if [[ ! -x "${bin}" ]]; then
        echo "[ERROR] Missing executable: ${bin}"
        echo "[HINT] Build with: cmake -S . -B build -DZOO_BUILD_EXAMPLES=ON && cmake --build build"
        exit 1
    fi
done


# Track background server PIDs for cleanup on exit or error
udp_server_pid=""
shm_server_pid=""


# ------------------------------------------------------------------------------
# cleanup: Ensures any background server processes are killed on script exit.
# This prevents orphaned processes if the script fails or is interrupted.
# ------------------------------------------------------------------------------
cleanup() {
    # Kill UDP server if running
    if [[ -n "${udp_server_pid}" ]] && kill -0 "${udp_server_pid}" 2>/dev/null; then
        kill "${udp_server_pid}" 2>/dev/null || true
    fi
    # Kill SHM server if running
    if [[ -n "${shm_server_pid}" ]] && kill -0 "${shm_server_pid}" 2>/dev/null; then
        kill "${shm_server_pid}" 2>/dev/null || true
    fi
}
# Register cleanup to run on any script exit (normal or error)
trap cleanup EXIT


# ------------------------------------------------------------------------------
# run_udp_check: Verifies UDP transport by running server and client examples.
#
# Steps:
#   1. Launches the UDP server in the background with a unique target name.
#   2. Runs the UDP client, sending a test payload to the server.
#   3. Waits for the client to complete (with a timeout for robustness).
#   4. Checks the client log for a "Received reply" marker indicating success.
#   5. If any step fails, prints the last 40 lines of logs for debugging.
#   6. Cleans up the server process.
# ------------------------------------------------------------------------------
run_udp_check() {
    local target="verify_udp_${$}"
    local server_log
    local client_log
    server_log="$(mktemp)"   # Temporary file for server log output
    client_log="$(mktemp)"   # Temporary file for client log output

    echo "[UDP] starting server target=${target}"
    "${UDP_SERVER_BIN}" -t "${target}" -l 2 >"${server_log}" 2>&1 &
    udp_server_pid=$!

    echo "[UDP] running client"
    # Run client with a 20s timeout to avoid hanging if server is unresponsive
    if ! timeout 20s "${UDP_CLIENT_BIN}" -t "${target}" -p "hello udp verify" -l 2 >"${client_log}" 2>&1; then
        echo "[UDP] client failed (nonzero exit or timeout)"
        tail -n 40 "${server_log}" || true
        tail -n 40 "${client_log}" || true
        return 1
    fi

    # Look for the expected success marker in the client log
    if ! grep -q "Received reply" "${client_log}"; then
        echo "[UDP] missing success marker in client log (communication failed)"
        tail -n 40 "${client_log}" || true
        return 1
    fi

    # Clean up server process and reset PID
    kill "${udp_server_pid}" 2>/dev/null || true
    wait "${udp_server_pid}" 2>/dev/null || true
    udp_server_pid=""
    echo "[UDP] PASS"
}


# ------------------------------------------------------------------------------
# run_shm_check: Verifies SHM transport by running server and client examples.
#
# Steps:
#   1. Launches the SHM server in the background with a unique channel and name.
#   2. Runs the SHM client, sending a test payload to the server.
#   3. Waits for the client to complete (with a timeout for robustness).
#   4. Checks the client log for a "Received reply" marker indicating success.
#   5. If any step fails, prints the last 40 lines of logs for debugging.
#   6. Cleans up the server process.
# ------------------------------------------------------------------------------
run_shm_check() {
    local channel="zoo_shm_verify_${$}"
    local server_name="shm_verify_server_${$}"
    local server_log
    local client_log
    server_log="$(mktemp)"   # Temporary file for server log output
    client_log="$(mktemp)"   # Temporary file for client log output

    echo "[SHM] starting server channel=${channel} server=${server_name}"
    "${SHM_SERVER_BIN}" --channel "${channel}" --server-name "${server_name}" --log-level 2 >"${server_log}" 2>&1 &
    shm_server_pid=$!

    echo "[SHM] running client"
    # Run client with a 20s timeout to avoid hanging if server is unresponsive
    if ! timeout 20s "${SHM_CLIENT_BIN}" --channel "${channel}" --server-name "${server_name}" --payload "hello shm verify" --log-level 2 >"${client_log}" 2>&1; then
        echo "[SHM] client failed (nonzero exit or timeout)"
        tail -n 40 "${server_log}" || true
        tail -n 40 "${client_log}" || true
        return 1
    fi

    # Look for the expected success marker in the client log
    if ! grep -q "Received reply" "${client_log}"; then
        echo "[SHM] missing success marker in client log (communication failed)"
        tail -n 40 "${client_log}" || true
        return 1
    fi

    # Clean up server process and reset PID
    kill "${shm_server_pid}" 2>/dev/null || true
    wait "${shm_server_pid}" 2>/dev/null || true
    shm_server_pid=""
    echo "[SHM] PASS"
}

# ------------------------------------------------------------------------------
# Main execution: run both transport checks and report overall result
# ------------------------------------------------------------------------------
run_udp_check
run_shm_check

echo "All transport checks passed."
