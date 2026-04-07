#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
mkdir -p "${ARTIFACT_DIR}"

if [[ ! -d "${BUILD_DIR}" ]]; then
  cmake -S . -B "${BUILD_DIR}"
  cmake --build "${BUILD_DIR}" -j"$(nproc)"
fi

if ctest --test-dir "${BUILD_DIR}" -N | grep -Ei "fuzz|afl|libfuzzer" > /dev/null; then
  ctest --test-dir "${BUILD_DIR}" --output-on-failure -R "fuzz|afl|libfuzzer" | tee "${ARTIFACT_DIR}/fuzz.log"
  echo '{"executed":true}' > "${ARTIFACT_DIR}/fuzz_summary.json"
else
  echo "No fuzz target registered yet" | tee "${ARTIFACT_DIR}/fuzz.log"
  echo '{"executed":false,"reason":"no-fuzz-target"}' > "${ARTIFACT_DIR}/fuzz_summary.json"
fi
