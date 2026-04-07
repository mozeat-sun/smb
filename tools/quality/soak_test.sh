#!/usr/bin/env bash
set -euo pipefail

DURATION_MINUTES="${1:-30}"
BUILD_DIR="${BUILD_DIR:-build}"
ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
mkdir -p "${ARTIFACT_DIR}"

if [[ ! -d "${BUILD_DIR}" ]]; then
  cmake -S . -B "${BUILD_DIR}"
  cmake --build "${BUILD_DIR}" -j"$(nproc)"
fi

END_EPOCH=$(( $(date +%s) + DURATION_MINUTES * 60 ))
ITER=0
FAILURES=0

while [[ $(date +%s) -lt ${END_EPOCH} ]]; do
  ITER=$((ITER + 1))
  if ! ctest --test-dir "${BUILD_DIR}" --output-on-failure; then
    FAILURES=$((FAILURES + 1))
  fi
  printf "soak iteration=%d failures=%d\n" "${ITER}" "${FAILURES}" | tee -a "${ARTIFACT_DIR}/soak.log"
done

cat > "${ARTIFACT_DIR}/soak_summary.json" <<EOF
{
  "duration_minutes": ${DURATION_MINUTES},
  "iterations": ${ITER},
  "failures": ${FAILURES}
}
EOF
