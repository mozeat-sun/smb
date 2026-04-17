#!/usr/bin/env bash
set -euo pipefail

ARTIFACT_DIR="${ARTIFACT_DIR:-artifacts/quality}"
BUILD_DIR="${BUILD_DIR:-build}"
RTO_TEST_REGEX="${RTO_TEST_REGEX:-^test_zoo_smb_integration$}"
RTO_RUNS="${RTO_RUNS:-5}"
RTO_MAX_MS="${RTO_MAX_MS:-5000}"
RTO_ENFORCE_THRESHOLD="${RTO_ENFORCE_THRESHOLD:-1}"

mkdir -p "${ARTIFACT_DIR}"

if [[ ! -d "${BUILD_DIR}" ]]; then
  cmake -S . -B "${BUILD_DIR}"
fi
cmake --build "${BUILD_DIR}" -j"$(nproc)"

if ! command -v ctest >/dev/null 2>&1; then
  echo "ctest not found in PATH" >&2
  exit 1
fi

python3 - "${BUILD_DIR}" "${RTO_TEST_REGEX}" "${RTO_RUNS}" "${RTO_MAX_MS}" "${RTO_ENFORCE_THRESHOLD}" "${ARTIFACT_DIR}" <<'PY'
import json
import math
import os
import re
import statistics
import subprocess
import sys
import time


build_dir, test_regex, runs_arg, rto_max_arg, enforce_arg, artifact_dir = sys.argv[1:7]
runs = int(runs_arg)
rto_max_ms = float(rto_max_arg)
enforce = enforce_arg == "1"

if runs <= 0:
    print("RTO_RUNS must be > 0", file=sys.stderr)
    sys.exit(1)

durations_ms = []
run_logs = []

for i in range(runs):
    start_ns = time.monotonic_ns()
    proc = subprocess.run(
        [
            "ctest",
            "--test-dir",
            build_dir,
            "-R",
            test_regex,
            "--output-on-failure",
        ],
        capture_output=True,
        text=True,
    )
    elapsed_ms = (time.monotonic_ns() - start_ns) / 1_000_000.0
    durations_ms.append(round(elapsed_ms, 3))
    run_logs.append(
        {
            "run": i + 1,
            "exit_code": proc.returncode,
            "duration_ms": round(elapsed_ms, 3),
            "stdout_tail": "\n".join(proc.stdout.splitlines()[-20:]),
            "stderr_tail": "\n".join(proc.stderr.splitlines()[-20:]),
        }
    )
    if proc.returncode != 0:
        summary = {
            "status": "failed",
            "reason": "integration-test-failed",
            "test_regex": test_regex,
            "runs_requested": runs,
            "runs_completed": i + 1,
            "rto_target_ms": rto_max_ms,
            "durations_ms": durations_ms,
        }
        with open(os.path.join(artifact_dir, "recovery_rto_summary.json"), "w", encoding="utf-8") as f:
            json.dump(summary, f)
        with open(os.path.join(artifact_dir, "recovery_rto_runs.json"), "w", encoding="utf-8") as f:
            json.dump(run_logs, f)
        print(json.dumps(summary))
        sys.exit(1)

p50_ms = round(statistics.median(durations_ms), 3)
p95_ms = round(sorted(durations_ms)[max(0, math.ceil(0.95 * len(durations_ms)) - 1)], 3)
max_ms = round(max(durations_ms), 3)
mean_ms = round(statistics.fmean(durations_ms), 3)

threshold_exceeded = max_ms > rto_max_ms
status = "failed" if enforce and threshold_exceeded else "passed"

summary = {
    "status": status,
    "test_regex": test_regex,
    "runs": runs,
    "rto_target_ms": rto_max_ms,
    "enforce_threshold": enforce,
    "restart_cycle_p50_ms": p50_ms,
    "restart_cycle_p95_ms": p95_ms,
    "restart_cycle_mean_ms": mean_ms,
    "restart_cycle_max_ms": max_ms,
    "threshold_exceeded": threshold_exceeded,
    "durations_ms": durations_ms,
}

with open(os.path.join(artifact_dir, "recovery_rto_summary.json"), "w", encoding="utf-8") as f:
    json.dump(summary, f)

with open(os.path.join(artifact_dir, "recovery_rto_runs.json"), "w", encoding="utf-8") as f:
    json.dump(run_logs, f)

with open(os.path.join(artifact_dir, "recovery_rto_report.md"), "w", encoding="utf-8") as f:
    f.write("# Recovery RTO Report\n\n")
    f.write(f"- Test regex: `{test_regex}`\n")
    f.write(f"- Runs: {runs}\n")
    f.write(f"- RTO target (ms): {rto_max_ms}\n")
    f.write(f"- Threshold enforcement: {enforce}\n")
    f.write(f"- Status: `{status}`\n\n")
    f.write("## Restart cycle metrics\n\n")
    f.write(f"- p50 (ms): {p50_ms}\n")
    f.write(f"- p95 (ms): {p95_ms}\n")
    f.write(f"- mean (ms): {mean_ms}\n")
    f.write(f"- max (ms): {max_ms}\n")
    f.write(f"- threshold exceeded: {threshold_exceeded}\n\n")
    f.write("## Per-run durations\n\n")
    for idx, value in enumerate(durations_ms, start=1):
        f.write(f"- run {idx}: {value} ms\n")

print(json.dumps(summary))
if status == "failed":
    sys.exit(1)
PY