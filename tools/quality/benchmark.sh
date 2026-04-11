#!/usr/bin/env bash
set -euo pipefail

ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
QUALITY_BASELINE_FILE="${QUALITY_BASELINE_FILE:-config/quality_baselines.ci.json}"
QUALITY_BASELINE_PROFILE="${QUALITY_BASELINE_PROFILE:-ci}"
QUALITY_REQUIRE_METRICS="${QUALITY_REQUIRE_METRICS:-0}"
BUILD_DIR="${BUILD_DIR:-build}"
mkdir -p "${ARTIFACT_DIR}"

if [[ ! -d "${BUILD_DIR}" ]]; then
  cmake -S . -B "${BUILD_DIR}"
fi
cmake --build "${BUILD_DIR}" -j"$(nproc)" --target e2e_performance_benchmark

BENCHMARK_BIN="stage/bin/e2e_performance_benchmark"
if [[ ! -x "${BENCHMARK_BIN}" ]]; then
  echo "benchmark binary missing: ${BENCHMARK_BIN}" | tee "${ARTIFACT_DIR}/benchmark.log"
  exit 1
fi

export LD_LIBRARY_PATH="$PWD/stage/lib:$PWD/${BUILD_DIR}/hidden_shared_libs:${LD_LIBRARY_PATH:-}"

{
  echo "Benchmark execution path: isolated modes 1, 2, and 6"
  echo "Date: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo
  echo "[mode 1]"
  "${BENCHMARK_BIN}" 1 2>&1 | grep '\[bench' || true
  sleep 6
  echo
  echo "[mode 2]"
  "${BENCHMARK_BIN}" 2 2>&1 | grep '\[bench' || true
  sleep 6
  echo
  echo "[mode 6]"
  "${BENCHMARK_BIN}" 6 2>&1 | grep '\[bench' || true
} | tee "${ARTIFACT_DIR}/benchmark.log"

python3 - "${ARTIFACT_DIR}/benchmark.log" > "${ARTIFACT_DIR}/benchmark_metrics.json" <<'PY'
import json
import re
import sys

log_path = sys.argv[1]
metrics = {
  "p99_latency_ms": None,
  "throughput_ops": None,
  "jitter_ms": None,
  "message_size_throughput_ops": {},
  "bench6_throughput_ops": None,
  "p50_latency_ms": None,
  "p95_latency_ms": None,
}

try:
  with open(log_path, "r", encoding="utf-8", errors="ignore") as f:
    content = f.read()

  bench1 = re.search(r"\[bench1\].*?,\s*([0-9]+(?:\.[0-9]+)?)\s+msg/s", content)
  if bench1:
    metrics["throughput_ops"] = float(bench1.group(1))

  for payload_size, throughput in re.findall(r"\[bench2\]\s+payload=([0-9]+)B\s+success=[0-9]+/[0-9]+\s+throughput=([0-9]+(?:\.[0-9]+)?)\s+msg/s", content):
    metrics["message_size_throughput_ops"][payload_size] = float(throughput)

  bench6 = re.search(
    r"\[bench6\].*?throughput=([0-9]+(?:\.[0-9]+)?)\s+msg/s\s+avg=([0-9]+(?:\.[0-9]+)?)\s+us\s+p50=([0-9]+(?:\.[0-9]+)?)\s+us\s+p95=([0-9]+(?:\.[0-9]+)?)\s+us\s+p99=([0-9]+(?:\.[0-9]+)?)\s+us\s+min=([0-9]+(?:\.[0-9]+)?)\s+us\s+max=([0-9]+(?:\.[0-9]+)?)\s+us",
    content,
  )
  if bench6:
    metrics["bench6_throughput_ops"] = float(bench6.group(1))
    metrics["p50_latency_ms"] = round(float(bench6.group(3)) / 1000.0, 3)
    metrics["p95_latency_ms"] = round(float(bench6.group(4)) / 1000.0, 3)
    metrics["p99_latency_ms"] = round(float(bench6.group(5)) / 1000.0, 3)
    min_ms = float(bench6.group(6)) / 1000.0
    max_ms = float(bench6.group(7)) / 1000.0
    metrics["jitter_ms"] = round(max_ms - min_ms, 3)
except Exception:
  pass

print(json.dumps(metrics))
PY

python3 - "${ARTIFACT_DIR}/benchmark_metrics.json" "${QUALITY_BASELINE_PROFILE}" > "${ARTIFACT_DIR}/benchmark_summary.json" <<'PY'
import json
import sys

metrics_path = sys.argv[1]
profile_name = sys.argv[2]
with open(metrics_path, "r", encoding="utf-8") as f:
    metrics = json.load(f)

summary = {
    "source": "isolated-e2e-benchmark-modes",
    "profile": profile_name,
    "throughput_ops": metrics.get("throughput_ops"),
    "bench6_throughput_ops": metrics.get("bench6_throughput_ops"),
    "p50_latency_ms": metrics.get("p50_latency_ms"),
    "p95_latency_ms": metrics.get("p95_latency_ms"),
    "p99_latency_ms": metrics.get("p99_latency_ms"),
    "jitter_ms": metrics.get("jitter_ms"),
    "message_size_throughput_ops": metrics.get("message_size_throughput_ops", {}),
}
print(json.dumps(summary))
PY

QUALITY_REQUIRE_METRICS="${QUALITY_REQUIRE_METRICS}" \
python3 tools/quality/compare_benchmark_to_baseline.py "${QUALITY_BASELINE_FILE}" "${ARTIFACT_DIR}/benchmark_metrics.json" "${QUALITY_BASELINE_PROFILE}"
