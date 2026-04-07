#!/usr/bin/env python3
import json
import sys
from pathlib import Path

baseline_path = Path(sys.argv[1] if len(sys.argv) > 1 else "config/quality_baselines.json")
summary_path = Path(sys.argv[2] if len(sys.argv) > 2 else "artifacts/quality/benchmark_metrics.json")

if not baseline_path.exists():
    print(f"baseline file missing: {baseline_path}")
    sys.exit(1)

if not summary_path.exists():
    print(f"benchmark metrics missing: {summary_path}")
    print("No threshold comparison performed.")
    sys.exit(0)

baseline = json.loads(baseline_path.read_text())
metrics = json.loads(summary_path.read_text())
profile = baseline.get("benchmark_profiles", {}).get("default", {})

failures = []
p99_max = profile.get("p99_latency_ms_max")
throughput_min = profile.get("throughput_ops_min")
jitter_max = profile.get("jitter_ms_max")

p99_actual = metrics.get("p99_latency_ms")
throughput_actual = metrics.get("throughput_ops")
jitter_actual = metrics.get("jitter_ms")

if p99_max is not None and p99_actual is not None and p99_actual > p99_max:
    failures.append(f"p99 latency {p99_actual} exceeds {p99_max}")
if throughput_min is not None and throughput_actual is not None and throughput_actual < throughput_min:
    failures.append(f"throughput {throughput_actual} below {throughput_min}")
if jitter_max is not None and jitter_actual is not None and jitter_actual > jitter_max:
    failures.append(f"jitter {jitter_actual} exceeds {jitter_max}")

if failures:
    for failure in failures:
        print(failure)
    sys.exit(2)

print("Benchmark baseline comparison passed or baseline thresholds are not yet populated.")
