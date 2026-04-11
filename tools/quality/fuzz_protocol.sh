#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
mkdir -p "${ARTIFACT_DIR}"

if [[ ! -d "${BUILD_DIR}" ]]; then
  cmake -S . -B "${BUILD_DIR}"
  cmake --build "${BUILD_DIR}" -j"$(nproc)"
fi

FUZZ_TEST_COUNT=$(ctest --test-dir "${BUILD_DIR}" -N | grep -Ei "Test #[0-9]+: .*fuzz|Test #[0-9]+: .*afl|Test #[0-9]+: .*libfuzzer" | wc -l)

if [[ ${FUZZ_TEST_COUNT} -gt 0 ]]; then
  FUZZ_ARTIFACT_DIR="${ARTIFACT_DIR}" \
    ctest --test-dir "${BUILD_DIR}" --output-on-failure -R "fuzz|afl|libfuzzer" | tee "${ARTIFACT_DIR}/fuzz.log"

  FUZZ_EXECUTABLE=$(find "${BUILD_DIR}" "stage/bin" -maxdepth 3 -type f -name "fuzz_zoo_smb_protocol" 2>/dev/null | head -n 1 || true)
  if [[ -n "${FUZZ_EXECUTABLE}" ]]; then
    LD_LIBRARY_PATH="$PWD/stage/lib:$PWD/${BUILD_DIR}/hidden_shared_libs:${LD_LIBRARY_PATH:-}" \
      FUZZ_ARTIFACT_DIR="${ARTIFACT_DIR}" \
      "${FUZZ_EXECUTABLE}" >> "${ARTIFACT_DIR}/fuzz.log" 2>&1
  fi

  if [[ ! -f "${ARTIFACT_DIR}/fuzz_protocol_coverage.json" ]]; then
    echo '{"target":"fuzz-protocol","executed":true,"entrypoints":[],"seed_cases":0,"mutated_cases":0}' > "${ARTIFACT_DIR}/fuzz_protocol_coverage.json"
  fi

  python3 - "${ARTIFACT_DIR}/fuzz_protocol_coverage.json" "${FUZZ_TEST_COUNT}" > "${ARTIFACT_DIR}/fuzz_summary.json" <<'PY'
import json
import sys

coverage_path = sys.argv[1]
target_count = int(sys.argv[2])

with open(coverage_path, "r", encoding="utf-8") as f:
    coverage = json.load(f)

summary = {
    "executed": True,
    "target_count": target_count,
    "coverage_report": coverage_path.split("/")[-1],
    "seed_cases": int(coverage.get("seed_cases", 0)),
    "mutated_cases": int(coverage.get("mutated_cases", 0)),
    "entrypoints": coverage.get("entrypoints", []),
}

print(json.dumps(summary))
PY
else
  echo "No fuzz target registered yet" | tee "${ARTIFACT_DIR}/fuzz.log"
  echo '{"executed":false,"reason":"no-fuzz-target"}' > "${ARTIFACT_DIR}/fuzz_summary.json"
fi
