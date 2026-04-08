# Benchmark Baseline Process

Use this process to make benchmark thresholds enforceable.

## Step 1: Capture approved baseline

- Run `tools/quality/benchmark.sh` on a controlled reference environment.
- Record p50, p95, p99, throughput, and jitter values.
- Store approved CI thresholds in `config/quality_baselines.ci.json` under profile `ci`.
- Store approved release thresholds in `config/quality_baselines.release.json` under profile `release`.
- Keep schema conformance with `config/quality_baselines.schema.json`.

## Step 2: Enforce thresholds in CI

- The benchmark job should generate `artifacts/quality/benchmark_metrics.json`.
- `tools/quality/compare_benchmark_to_baseline.py` compares current metrics with baseline thresholds.
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

## Initial state

The repository now includes schema-validated CI/release baseline profiles, but thresholds are intentionally unset until first approved captures are completed.
