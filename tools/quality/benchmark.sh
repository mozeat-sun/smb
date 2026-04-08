#!/usr/bin/env bash
set -euo pipefail

ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
QUALITY_BASELINE_FILE="${QUALITY_BASELINE_FILE:-config/quality_baselines.ci.json}"
QUALITY_BASELINE_PROFILE="${QUALITY_BASELINE_PROFILE:-ci}"
QUALITY_REQUIRE_METRICS="${QUALITY_REQUIRE_METRICS:-0}"
mkdir -p "${ARTIFACT_DIR}"

if [[ -x "smb/run_performance_tests.sh" ]]; then
  bash smb/run_performance_tests.sh | tee "${ARTIFACT_DIR}/benchmark.log"
  echo '{"source":"smb/run_performance_tests.sh"}' > "${ARTIFACT_DIR}/benchmark_summary.json"
  if [[ ! -f "${ARTIFACT_DIR}/benchmark_metrics.json" ]]; then
    echo '{"p99_latency_ms":null,"throughput_ops":null,"jitter_ms":null}' > "${ARTIFACT_DIR}/benchmark_metrics.json"
  fi
  python3 tools/quality/compare_benchmark_to_baseline.py "${QUALITY_BASELINE_FILE}" "${ARTIFACT_DIR}/benchmark_metrics.json" "${QUALITY_BASELINE_PROFILE}"
  exit 0
fi

BUILD_DIR="${BUILD_DIR:-build}"
if [[ ! -d "${BUILD_DIR}" ]]; then
  cmake -S . -B "${BUILD_DIR}"
  cmake --build "${BUILD_DIR}" -j"$(nproc)"
fi

/usr/bin/time -v ctest --test-dir "${BUILD_DIR}" --output-on-failure |& tee "${ARTIFACT_DIR}/benchmark.log"

echo '{"source":"ctest-timing-fallback"}' > "${ARTIFACT_DIR}/benchmark_summary.json"
python3 - "${BUILD_DIR}" "${ARTIFACT_DIR}/benchmark.log" > "${ARTIFACT_DIR}/benchmark_metrics.json" <<'PY'
import json
import re
import subprocess
import sys

build_dir = sys.argv[1]
log_path = sys.argv[2]
metrics = {"p99_latency_ms": None, "throughput_ops": None, "jitter_ms": None}

try:
  with open(log_path, "r", encoding="utf-8", errors="ignore") as f:
    content = f.read()

  m = re.search(r"Total Test time \(real\) =\s*([0-9]+(?:\.[0-9]+)?)\s*sec", content)
  if m:
    duration_sec = float(m.group(1))
    out = subprocess.check_output(
      ["ctest", "--test-dir", build_dir, "-N"],
      stderr=subprocess.STDOUT,
      text=True,
    )
    n = re.search(r"Total Tests:\s*([0-9]+)", out)
    if n and duration_sec > 0:
      metrics["throughput_ops"] = round(int(n.group(1)) / duration_sec, 6)
except Exception:
  pass

print(json.dumps(metrics))
PY

QUALITY_REQUIRE_METRICS="${QUALITY_REQUIRE_METRICS}" \
python3 tools/quality/compare_benchmark_to_baseline.py "${QUALITY_BASELINE_FILE}" "${ARTIFACT_DIR}/benchmark_metrics.json" "${QUALITY_BASELINE_PROFILE}"
