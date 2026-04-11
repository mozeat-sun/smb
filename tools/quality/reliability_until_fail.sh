#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
MAX_REPETITIONS="${MAX_REPETITIONS:-20}"
SUITE_ARGS="${SUITE_ARGS:-}"
mkdir -p "${ARTIFACT_DIR}"

if [[ ! -d "${BUILD_DIR}" ]]; then
  cmake -S . -B "${BUILD_DIR}"
  cmake --build "${BUILD_DIR}" -j"$(nproc)"
fi

LOG_FILE="${ARTIFACT_DIR}/reliability_until_fail.log"
SUMMARY_FILE="${ARTIFACT_DIR}/reliability_until_fail_summary.json"
CLASSIFICATION_FILE="${ARTIFACT_DIR}/reliability_until_fail_classification.tsv"

CMD=(ctest --test-dir "${BUILD_DIR}" --output-on-failure --repeat "until-fail:${MAX_REPETITIONS}")
if [[ -n "${SUITE_ARGS}" ]]; then
  # shellcheck disable=SC2206
  EXTRA_ARGS=(${SUITE_ARGS})
  CMD+=("${EXTRA_ARGS[@]}")
fi

set +e
"${CMD[@]}" | tee "${LOG_FILE}"
CTEST_EXIT=$?
set -e

FAILED_COUNT=$(grep -c "\*\*\*Failed" "${LOG_FILE}" 2>/dev/null || true)
if [[ ${FAILED_COUNT} -gt 0 ]]; then
  STATUS="failed"
  GATE_PASSED="false"
else
  STATUS="passed"
  GATE_PASSED="true"
fi

printf "category,count\n" > "${CLASSIFICATION_FILE}"
printf "ctest_failure,%d\n" "${FAILED_COUNT}" >> "${CLASSIFICATION_FILE}"

cat > "${SUMMARY_FILE}" <<EOF
{
  "mode": "repeat-until-fail",
  "max_repetitions": ${MAX_REPETITIONS},
  "status": "${STATUS}",
  "gate_passed": ${GATE_PASSED},
  "ctest_exit_code": ${CTEST_EXIT},
  "failed_test_events": ${FAILED_COUNT},
  "failure_classification": "$(basename "${CLASSIFICATION_FILE}")"
}
EOF

if [[ ${CTEST_EXIT} -ne 0 ]]; then
  exit ${CTEST_EXIT}
fi
