#!/usr/bin/env bash
set -euo pipefail

ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
QUALITY_BASELINE_FILE="${QUALITY_BASELINE_FILE:-config/quality_baselines.ci.json}"
QUALITY_BASELINE_PROFILE="${QUALITY_BASELINE_PROFILE:-ci}"
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
echo '{"p99_latency_ms":null,"throughput_ops":null,"jitter_ms":null}' > "${ARTIFACT_DIR}/benchmark_metrics.json"
python3 tools/quality/compare_benchmark_to_baseline.py "${QUALITY_BASELINE_FILE}" "${ARTIFACT_DIR}/benchmark_metrics.json" "${QUALITY_BASELINE_PROFILE}"
