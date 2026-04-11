# Benchmark Baseline Process

Use this process to make benchmark thresholds enforceable.

## Step 1: Capture approved baseline

- Run `tools/quality/benchmark.sh` on a controlled reference environment.
- Record aggregate throughput and per-message-size throughput values from the isolated payload benchmark modes.
- Record p50, p95, p99, and jitter values from stabilized bench6 pub/sub captures.
- Store approved CI thresholds in `config/quality_baselines.ci.json` under profile `ci`.
- Store approved release thresholds in `config/quality_baselines.release.json` under profile `release`.
- Keep schema conformance with `config/quality_baselines.schema.json`.

## Step 2: Enforce thresholds in CI

- The benchmark job should generate `artifacts/quality/benchmark_metrics.json`.
- `tools/quality/compare_benchmark_to_baseline.py` compares current metrics with baseline thresholds.
- The benchmark gate uses isolated benchmark modes instead of the historical `all` chain so throughput measurements are reproducible.
- Mode `6` now contributes active regression-gating metrics for p99 latency and jitter.
- CI uses `QUALITY_BASELINE_FILE=config/quality_baselines.ci.json` and `QUALITY_BASELINE_PROFILE=ci`.
- CI fails when populated thresholds are violated.

## Step 3: Rebaseline intentionally

- Only rebaseline after approved architecture or environment changes.
- Record the reason in release notes and scorecard.

## Step 4: Release verification

- Run benchmark gate with release profile in controlled hardware:
	- `QUALITY_BASELINE_FILE=config/quality_baselines.release.json`
	- `QUALITY_BASELINE_PROFILE=release`
- Require change approval for baseline updates (schema version, owner, timestamp, and threshold deltas).

## Current approved baseline

The repository now includes approved CI and release thresholds for:

- aggregate publish throughput
- per-message-size throughput profiles for 64B, 256B, 1024B, and 4096B payloads

Pub/sub latency benchmarking in mode `6` is now release-gating for p99 latency and jitter. Exploratory RPC round-trip benchmarking remains available in mode `5`, but it is still informational and not yet threshold-gated.
