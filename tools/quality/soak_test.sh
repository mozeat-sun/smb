#!/usr/bin/env bash
set -euo pipefail

DURATION_MINUTES="${1:-30}"
BUILD_DIR="${BUILD_DIR:-build}"
ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
RELIABILITY_MODE="${RELIABILITY_MODE:-ci}"
CTEST_ARGS="${CTEST_ARGS:-}"
mkdir -p "${ARTIFACT_DIR}"

if [[ ! -d "${BUILD_DIR}" ]]; then
  cmake -S . -B "${BUILD_DIR}"
  cmake --build "${BUILD_DIR}" -j"$(nproc)"
fi

END_EPOCH=$(( $(date +%s) + DURATION_MINUTES * 60 ))
START_EPOCH=$(date +%s)
ITER=0
FAILURES=0
PASSES=0
FIRST_FAILURE_ITER=0
FIRST_FAILURE_TS=""

ITERATION_TSV="${ARTIFACT_DIR}/soak_iterations.tsv"
CLASSIFICATION_TSV="${ARTIFACT_DIR}/reliability_failure_classification.tsv"

printf "iteration,start_utc,end_utc,status,failed_tests\n" > "${ITERATION_TSV}"

count_failed_tests() {
  local log_file="$1"
  grep -c "\*\*\*Failed" "${log_file}" 2>/dev/null || true
}

run_soak_iteration() {
  local iteration_log="$1"
  local cmd=(ctest --test-dir "${BUILD_DIR}" --output-on-failure)

  if [[ -n "${CTEST_ARGS}" ]]; then
    # shellcheck disable=SC2206
    local extra_args=(${CTEST_ARGS})
    cmd+=("${extra_args[@]}")
  fi

  "${cmd[@]}" | tee "${iteration_log}"
}

while [[ $(date +%s) -lt ${END_EPOCH} ]]; do
  ITER=$((ITER + 1))
  ITER_START_UTC=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
  ITER_LOG="${ARTIFACT_DIR}/soak_iteration_${ITER}.log"

  if ! run_soak_iteration "${ITER_LOG}"; then
    FAILURES=$((FAILURES + 1))
    FAILED_TESTS=$(count_failed_tests "${ITER_LOG}")
    if [[ ${FIRST_FAILURE_ITER} -eq 0 ]]; then
      FIRST_FAILURE_ITER=${ITER}
      FIRST_FAILURE_TS="${ITER_START_UTC}"
    fi
    ITER_STATUS="failed"
  else
    PASSES=$((PASSES + 1))
    FAILED_TESTS=0
    ITER_STATUS="passed"
  fi

  ITER_END_UTC=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

  printf "%d,%s,%s,%s,%d\n" \
    "${ITER}" \
    "${ITER_START_UTC}" \
    "${ITER_END_UTC}" \
    "${ITER_STATUS}" \
    "${FAILED_TESTS}" \
    >> "${ITERATION_TSV}"

  printf "soak iteration=%d status=%s failures=%d\n" "${ITER}" "${ITER_STATUS}" "${FAILURES}" | tee -a "${ARTIFACT_DIR}/soak.log"
done

if [[ ${FIRST_FAILURE_ITER} -eq 0 ]]; then
  MTTR_ESTIMATE_SEC=0
else
  NOW_EPOCH=$(date +%s)
  MTTR_ESTIMATE_SEC=$((NOW_EPOCH - START_EPOCH))
fi

printf "category,count\n" > "${CLASSIFICATION_TSV}"
printf "ctest_failure,%d\n" "${FAILURES}" >> "${CLASSIFICATION_TSV}"

cat > "${ARTIFACT_DIR}/soak_summary.json" <<EOF
{
  "mode": "${RELIABILITY_MODE}",
  "duration_minutes": ${DURATION_MINUTES},
  "iterations": ${ITER},
  "passes": ${PASSES},
  "failures": ${FAILURES},
  "first_failure_iteration": ${FIRST_FAILURE_ITER},
  "first_failure_utc": "${FIRST_FAILURE_TS}",
  "failure_classification": "$(basename "${CLASSIFICATION_TSV}")",
  "iteration_log": "$(basename "${ITERATION_TSV}")",
  "mttr_estimate_seconds": ${MTTR_ESTIMATE_SEC}
}
EOF
