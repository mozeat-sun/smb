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
BENCHMARK_LOG="${ARTIFACT_DIR}/benchmark.log"
exec > >(tee "${BENCHMARK_LOG}") 2>&1

run_isolated_bench2_payload() {
  local payload_size="$1"
  local rounds="${BENCH2_ROUNDS:-5000}"
  local publish_delay_us="${BENCH2_PUBLISH_DELAY_US:-1000}"
  local run_id
  local subscriber_name
  local publisher_name
  local topic_name
  local subscriber_log
  local publisher_log
  local subscriber_pid
  local summary_line

  run_id="$(date +%s)${payload_size}"
  subscriber_name="b2s_${run_id}"
  publisher_name="b2p_${run_id}"
  topic_name="b2/${run_id}"
  subscriber_log="${ARTIFACT_DIR}/bench2_payload_${payload_size}_subscriber.log"
  publisher_log="${ARTIFACT_DIR}/bench2_payload_${payload_size}_publisher.log"
  rm -f "${subscriber_log}" "${publisher_log}"

  "${BENCHMARK_BIN}" bench6-subscriber \
    "${subscriber_name}" \
    "${publisher_name}" \
    "${topic_name}" \
    "${rounds}" >"${subscriber_log}" 2>&1 &
  subscriber_pid=$!

  sleep 2

  if ! "${BENCHMARK_BIN}" bench6-publisher \
    "${rounds}" \
    "${publisher_name}" \
    "${publisher_name}" \
    "${topic_name}" \
    "${payload_size}" \
    "${publish_delay_us}" >"${publisher_log}" 2>&1; then
    echo "[bench2] payload=${payload_size}B success=0/${rounds} throughput=0 msg/s"
    echo "[bench2-latency] no successful samples"
    if [[ -n "${subscriber_pid:-}" ]] && kill -0 "${subscriber_pid}" 2>/dev/null; then
      kill "${subscriber_pid}" 2>/dev/null || true
      wait "${subscriber_pid}" 2>/dev/null || true
    fi
    rm -f "${subscriber_log}" "${publisher_log}"
    return 0
  fi

  wait "${subscriber_pid}" || true
  summary_line="$(grep '\[bench6-subscriber\]' "${subscriber_log}" | tail -n 1 || true)"

  if [[ -z "${summary_line}" ]]; then
    echo "[bench2] payload=${payload_size}B success=0/${rounds} throughput=0 msg/s"
    echo "[bench2-latency] no successful samples"
    rm -f "${subscriber_log}" "${publisher_log}"
    return 0
  fi

  python3 - "${payload_size}" "${rounds}" "${summary_line}" <<'PY'
import re
import sys

payload_size = sys.argv[1]
rounds = sys.argv[2]
summary = sys.argv[3]
match = re.search(
    r"received=(\d+)\s+loss=(\d+)\s+success=(\d+)\s+throughput=([0-9]+(?:\.[0-9]+)?)\s+msg/s\s+avg=([0-9]+(?:\.[0-9]+)?)\s+us\s+p50=([0-9]+(?:\.[0-9]+)?)\s+us\s+p95=([0-9]+(?:\.[0-9]+)?)\s+us\s+p99=([0-9]+(?:\.[0-9]+)?)\s+us\s+min=([0-9]+(?:\.[0-9]+)?)\s+us\s+max=([0-9]+(?:\.[0-9]+)?)\s+us",
    summary,
)
if not match:
    print(f"[bench2] payload={payload_size}B success=0/{rounds} throughput=0 msg/s")
    print("[bench2-latency] no successful samples")
    raise SystemExit(0)

received = match.group(1)
throughput = match.group(4)
avg_us = match.group(5)
p50_us = match.group(6)
p95_us = match.group(7)
p99_us = match.group(8)
min_us = match.group(9)
max_us = match.group(10)

print(f"[bench2] payload={payload_size}B success={received}/{rounds} throughput={throughput} msg/s")
print(
    f"[bench2-latency] success={received} throughput={throughput} msg/s avg={avg_us} us "
    f"p50={p50_us} us p95={p95_us} us p99={p99_us} us min={min_us} us max={max_us} us"
)
PY

  rm -f "${subscriber_log}" "${publisher_log}"
}

echo "Benchmark execution path: isolated modes 1, 2, 5, and 6"
echo "Date: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo
echo "[mode 2]"
run_isolated_bench2_payload 64
sleep 2
run_isolated_bench2_payload 256
sleep 2
run_isolated_bench2_payload 1024
sleep 2
run_isolated_bench2_payload 4096
sleep 6
echo
echo "[mode 1]"
"${BENCHMARK_BIN}" 1 2>&1 | grep '\[bench' || true
sleep 6
echo
echo "[mode 5]"
"${BENCHMARK_BIN}" 5 2>&1 | grep '\[bench' || true
sleep 6
echo
echo "[mode 6]"
"${BENCHMARK_BIN}" 6 2>&1 | grep '\[bench' || true

python3 - "${ARTIFACT_DIR}/benchmark.log" > "${ARTIFACT_DIR}/benchmark_metrics.json" <<'PY'
import json
import re
import sys

log_path = sys.argv[1]
metrics = {
  "p99_latency_ms": None,
  "throughput_ops": None,
  "jitter_ms": None,
  "bench5_throughput_ops": None,
  "bench5_p50_latency_ms": None,
  "bench5_p95_latency_ms": None,
  "bench5_p99_latency_ms": None,
  "bench5_jitter_ms": None,
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


  # Match all bench2 throughput lines: attempted/received/loss, window_ms, or success
  for m in re.finditer(r"\[bench2\]\s+payload=([0-9]+)B.*?throughput=([0-9]+(?:\.[0-9]+)?)\s+msg/s", content):
    payload_size, throughput = m.group(1), m.group(2)
    metrics["message_size_throughput_ops"][payload_size] = float(throughput)

  bench5 = re.search(
    r"\[bench5\].*?throughput=([0-9]+(?:\.[0-9]+)?)\s+msg/s\s+avg=([0-9]+(?:\.[0-9]+)?)\s+us\s+p50=([0-9]+(?:\.[0-9]+)?)\s+us\s+p95=([0-9]+(?:\.[0-9]+)?)\s+us\s+p99=([0-9]+(?:\.[0-9]+)?)\s+us\s+min=([0-9]+(?:\.[0-9]+)?)\s+us\s+max=([0-9]+(?:\.[0-9]+)?)\s+us",
    content,
  )
  if bench5:
    metrics["bench5_throughput_ops"] = float(bench5.group(1))
    metrics["bench5_p50_latency_ms"] = round(float(bench5.group(3)) / 1000.0, 3)
    metrics["bench5_p95_latency_ms"] = round(float(bench5.group(4)) / 1000.0, 3)
    metrics["bench5_p99_latency_ms"] = round(float(bench5.group(5)) / 1000.0, 3)
    bench5_min_ms = float(bench5.group(6)) / 1000.0
    bench5_max_ms = float(bench5.group(7)) / 1000.0
    metrics["bench5_jitter_ms"] = round(bench5_max_ms - bench5_min_ms, 3)

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
  "bench5_throughput_ops": metrics.get("bench5_throughput_ops"),
  "bench5_p50_latency_ms": metrics.get("bench5_p50_latency_ms"),
  "bench5_p95_latency_ms": metrics.get("bench5_p95_latency_ms"),
  "bench5_p99_latency_ms": metrics.get("bench5_p99_latency_ms"),
  "bench5_jitter_ms": metrics.get("bench5_jitter_ms"),
    "bench6_throughput_ops": metrics.get("bench6_throughput_ops"),
    "p50_latency_ms": metrics.get("p50_latency_ms"),
    "p95_latency_ms": metrics.get("p95_latency_ms"),
    "p99_latency_ms": metrics.get("p99_latency_ms"),
    "jitter_ms": metrics.get("jitter_ms"),
    "message_size_throughput_ops": metrics.get("message_size_throughput_ops", {}),
}
print(json.dumps(summary))
PY

python3 - "${ARTIFACT_DIR}/benchmark_metrics.json" > "${ARTIFACT_DIR}/latency_scenario_matrix.json" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as f:
  metrics = json.load(f)

matrix = {
  "scenarios": [
    {
      "name": "rpc_round_trip",
      "source": "bench5",
      "throughput_ops": metrics.get("bench5_throughput_ops"),
      "p50_latency_ms": metrics.get("bench5_p50_latency_ms"),
      "p95_latency_ms": metrics.get("bench5_p95_latency_ms"),
      "p99_latency_ms": metrics.get("bench5_p99_latency_ms"),
      "jitter_ms": metrics.get("bench5_jitter_ms"),
    },
    {
      "name": "pubsub_one_way",
      "source": "bench6",
      "throughput_ops": metrics.get("bench6_throughput_ops"),
      "p50_latency_ms": metrics.get("p50_latency_ms"),
      "p95_latency_ms": metrics.get("p95_latency_ms"),
      "p99_latency_ms": metrics.get("p99_latency_ms"),
      "jitter_ms": metrics.get("jitter_ms"),
    },
  ]
}

print(json.dumps(matrix))
PY

python3 - "${ARTIFACT_DIR}/latency_scenario_matrix.json" > "${ARTIFACT_DIR}/latency_scenario_matrix.md" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as f:
  matrix = json.load(f)

print("# Latency Scenario Matrix")
print()
print("| Scenario | Source | Throughput (msg/s) | p50 (ms) | p95 (ms) | p99 (ms) | Jitter (ms) |")
print("|---|---|---:|---:|---:|---:|---:|")
for row in matrix.get("scenarios", []):
  print(
    f"| {row.get('name')} | {row.get('source')} | "
    f"{row.get('throughput_ops')} | {row.get('p50_latency_ms')} | "
    f"{row.get('p95_latency_ms')} | {row.get('p99_latency_ms')} | "
    f"{row.get('jitter_ms')} |"
  )
PY

QUALITY_REQUIRE_METRICS="${QUALITY_REQUIRE_METRICS}" \
python3 tools/quality/compare_benchmark_to_baseline.py "${QUALITY_BASELINE_FILE}" "${ARTIFACT_DIR}/benchmark_metrics.json" "${QUALITY_BASELINE_PROFILE}"
