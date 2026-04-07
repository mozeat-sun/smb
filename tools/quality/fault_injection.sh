#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
mkdir -p "${ARTIFACT_DIR}"

if [[ ! -d "${BUILD_DIR}" ]]; then
  cmake -S . -B "${BUILD_DIR}"
  cmake --build "${BUILD_DIR}" -j"$(nproc)"
fi

# Industrial-grade approximation: repeated execution under tighter process limits.
ulimit -n 1024 || true
ulimit -u 2048 || true

ctest --test-dir "${BUILD_DIR}" --output-on-failure --repeat until-fail:5 | tee "${ARTIFACT_DIR}/fault_injection.log"

echo "{\"mode\":\"repeat-until-fail\",\"repetitions\":5}" > "${ARTIFACT_DIR}/fault_injection_summary.json"
