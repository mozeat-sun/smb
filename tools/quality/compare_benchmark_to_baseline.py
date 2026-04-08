#!/usr/bin/env python3
import json
import os
import sys
from pathlib import Path

baseline_path = Path(sys.argv[1] if len(sys.argv) > 1 else "config/quality_baselines.ci.json")
summary_path = Path(sys.argv[2] if len(sys.argv) > 2 else "artifacts/quality/benchmark_metrics.json")
profile_name = sys.argv[3] if len(sys.argv) > 3 else os.environ.get("QUALITY_BASELINE_PROFILE", "ci")


def fail(code: int, msg: str) -> None:
    print(msg)
    sys.exit(code)


def validate_baseline(doc: dict, profile: str) -> None:
    required_top = ["schema_version", "owner", "updated_at_utc", "benchmark_profiles"]
    for key in required_top:
        if key not in doc:
            fail(3, f"invalid baseline: missing top-level key '{key}'")

    if not isinstance(doc["schema_version"], int) or doc["schema_version"] < 1:
        fail(3, "invalid baseline: schema_version must be integer >= 1")

    profiles = doc.get("benchmark_profiles")
    if not isinstance(profiles, dict) or not profiles:
        fail(3, "invalid baseline: benchmark_profiles must be a non-empty object")

    if profile not in profiles:
        fail(3, f"baseline profile '{profile}' not found in {baseline_path}")

    profile_doc = profiles[profile]
    for metric_key in ["p99_latency_ms_max", "throughput_ops_min", "jitter_ms_max"]:
        if metric_key not in profile_doc:
            fail(3, f"invalid baseline profile '{profile}': missing '{metric_key}'")
        value = profile_doc[metric_key]
        if value is not None and not isinstance(value, (int, float)):
            fail(3, f"invalid baseline profile '{profile}': '{metric_key}' must be number or null")

if not baseline_path.exists():
    fail(1, f"baseline file missing: {baseline_path}")

if not summary_path.exists():
    print(f"benchmark metrics missing: {summary_path}")
    print("No threshold comparison performed.")
    sys.exit(0)

baseline = json.loads(baseline_path.read_text())
metrics = json.loads(summary_path.read_text())
validate_baseline(baseline, profile_name)
profile = baseline.get("benchmark_profiles", {}).get(profile_name, {})
require_metrics = os.environ.get("QUALITY_REQUIRE_METRICS", "0") == "1"

failures = []
p99_max = profile.get("p99_latency_ms_max")
throughput_min = profile.get("throughput_ops_min")
jitter_max = profile.get("jitter_ms_max")

p99_actual = metrics.get("p99_latency_ms")
throughput_actual = metrics.get("throughput_ops")
jitter_actual = metrics.get("jitter_ms")

if require_metrics:
    if p99_max is not None and p99_actual is None:
        failures.append("p99 latency metric missing while threshold is configured")
    if throughput_min is not None and throughput_actual is None:
        failures.append("throughput metric missing while threshold is configured")
    if jitter_max is not None and jitter_actual is None:
        failures.append("jitter metric missing while threshold is configured")

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

print(
    f"Benchmark baseline comparison passed for profile '{profile_name}' "
    "or thresholds are not yet populated."
)
