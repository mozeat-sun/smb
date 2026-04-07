#!/bin/bash

set -euo pipefail

readonly BUILD_DIR="${BUILD_DIR:-build}"
readonly REPORT_DIR="${REPORT_DIR:-reports}"
readonly TIMESTAMP="${TIMESTAMP:-$(date +"%Y%m%d_%H%M%S")}"
readonly TEST_TIMEOUT="${TEST_TIMEOUT:-180}"
readonly BENCH_BIN="${BUILD_DIR}/bin/e2e_performance_benchmark"

readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m'

log_info() {
    echo -e "${BLUE}[EXAMPLE-BENCH]${NC} $*"
}

log_success() {
    echo -e "${GREEN}[EXAMPLE-BENCH]${NC} $*"
}

log_warn() {
    echo -e "${YELLOW}[EXAMPLE-BENCH]${NC} $*"
}

log_error() {
    echo -e "${RED}[EXAMPLE-BENCH]${NC} $*"
}

find_benchmark_executable() {
    if [[ -x "${BENCH_BIN}" ]]; then
        echo "${BENCH_BIN}"
        return 0
    fi

    return 1
}

run_single_benchmark() {
    local executable="$1"
    local benchmark_id="$2"
    local raw_log="$3"

    log_info "Running benchmark ${benchmark_id} (timeout: ${TEST_TIMEOUT}s)..."

    if timeout "${TEST_TIMEOUT}" "${executable}" "${benchmark_id}" > "${raw_log}" 2>&1; then
        return 0
    fi

    return $?
}

extract_result_lines() {
    local benchmark_id="$1"
    local raw_log="$2"
    local lines

    lines=$(grep -F "[bench${benchmark_id}]" "${raw_log}" || true)
    if [[ -n "${lines}" ]]; then
        echo "${lines}"
    else
        echo "no benchmark result lines found"
    fi
}

write_summary_header() {
    local summary_file="$1"

    {
        echo "ZOO SMB example benchmark summary"
        echo "timestamp: ${TIMESTAMP}"
        echo "benchmark binary: ${BENCH_BIN}"
        echo "timeout_seconds: ${TEST_TIMEOUT}"
        echo
    } > "${summary_file}"
}

append_summary_entry() {
    local summary_file="$1"
    local benchmark_id="$2"
    local exit_code="$3"
    local result_lines="$4"
    local raw_log="$5"

    {
        echo "benchmark_${benchmark_id}:"
        echo "  exit_code: ${exit_code}"
        echo "  results:"
        while IFS= read -r line; do
            echo "    ${line}"
        done <<< "${result_lines}"
        echo "  raw_log: ${raw_log}"
        echo
    } >> "${summary_file}"
}

main() {
    local executable
    if ! executable=$(find_benchmark_executable); then
        log_error "Benchmark executable not found at ${BENCH_BIN}"
        log_error "Build SMB first: cmake --build build"
        exit 1
    fi

    mkdir -p "${REPORT_DIR}"

    local summary_file="${REPORT_DIR}/example_benchmark_summary_${TIMESTAMP}.txt"
    write_summary_header "${summary_file}"

    log_info "Using benchmark executable: ${executable}"
    log_info "Summary file: ${summary_file}"

    local failed=0
    local benchmark_id

    for benchmark_id in 1 2 3 4 5 6; do
        local raw_log="${REPORT_DIR}/example_benchmark_${benchmark_id}_${TIMESTAMP}.log"
        local exit_code=0

        run_single_benchmark "${executable}" "${benchmark_id}" "${raw_log}"
        exit_code=$?
        if [[ ${exit_code} -ne 0 ]]; then
            failed=1
            if [[ ${exit_code} -eq 124 ]]; then
                log_warn "Benchmark ${benchmark_id} timed out"
            else
                log_warn "Benchmark ${benchmark_id} exited with code ${exit_code}"
            fi
        fi

        local result_lines
        result_lines=$(extract_result_lines "${benchmark_id}" "${raw_log}")
        append_summary_entry "${summary_file}" "${benchmark_id}" "${exit_code}" "${result_lines}" "${raw_log}"

        local display_line
        display_line=$(printf '%s\n' "${result_lines}" | tail -n 1)

        if [[ ${exit_code} -eq 0 ]]; then
            log_success "Benchmark ${benchmark_id}: ${display_line}"
        else
            log_warn "Benchmark ${benchmark_id}: ${display_line}"
        fi
    done

    if [[ ${failed} -eq 0 ]]; then
        log_success "All example benchmarks completed"
    else
        log_warn "One or more benchmarks did not complete cleanly"
    fi

    log_info "Summary written to ${summary_file}"
}

main "$@"