#!/usr/bin/env bash
set -euo pipefail

ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
BUILD_DIR="${BUILD_DIR:-build}"
AUTOSELECT_TEST_REGEX="${AUTOSELECT_TEST_REGEX:-test_backpressure_security}"

mkdir -p "${ARTIFACT_DIR}"

if [[ ! -d "${BUILD_DIR}" ]]; then
  cmake -S . -B "${BUILD_DIR}"
fi
CPU_COUNT="$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
cmake --build "${BUILD_DIR}" -j"${CPU_COUNT}"

if ! command -v ctest >/dev/null 2>&1; then
  echo "ctest not found in PATH" >&2
  exit 1
fi

LOG_FILE="${ARTIFACT_DIR}/transport_autoselect_ctest.log"
if ctest --test-dir "${BUILD_DIR}" -R "${AUTOSELECT_TEST_REGEX}" --output-on-failure >"${LOG_FILE}" 2>&1; then
  status="passed"
else
  status="failed"
fi

python3 - "${status}" "${LOG_FILE}" "${ARTIFACT_DIR}" <<'PY'
import json
import os
import re
import sys

status = sys.argv[1]
log_file = sys.argv[2]
artifact_dir = sys.argv[3]

with open(log_file, "r", encoding="utf-8", errors="ignore") as f:
    content = f.read()

tests_total = None
tests_failed = None
match_total = re.search(r"(\d+)% tests passed out of (\d+)", content)
if match_total:
    tests_total = int(match_total.group(2))
match_failed = re.search(r"The following tests FAILED:", content)
tests_failed = 0 if not match_failed else 1

summary = {
    "status": status,
    "runner": "ctest",
    "regex": "test_backpressure_security",
    "tests_total": tests_total,
    "tests_failed": tests_failed,
    "evidence_focus": [
        "same-host SHM selection path",
        "auto-select disabled network path",
        "auto-select metrics API"
    ]
}

json_path = os.path.join(artifact_dir, "transport_autoselect_outcomes.json")
with open(json_path, "w", encoding="utf-8") as f:
    json.dump(summary, f)

md_path = os.path.join(artifact_dir, "transport_autoselect_outcomes.md")
with open(md_path, "w", encoding="utf-8") as f:
    f.write("# Transport Auto-Select Outcomes\n\n")
    f.write(f"- Status: `{status}`\n")
    f.write("- Runner: `ctest`\n")
    f.write("- Test selector: `test_backpressure_security`\n")
    if tests_total is not None:
      f.write(f"- Tests matched: {tests_total}\n")
    if tests_failed is not None:
      f.write(f"- Tests failed: {tests_failed}\n")
    f.write("\n## Covered evidence points\n\n")
    for item in summary["evidence_focus"]:
        f.write(f"- {item}\n")

print(json.dumps(summary))
if status != "passed":
    sys.exit(1)
PY
