# Benchmark Baseline Process

Use this process to make benchmark thresholds enforceable.

## Step 1: Capture approved baseline

- Run `tools/quality/benchmark.sh` on a controlled reference environment.
- Record p50, p95, p99, throughput, and jitter values.
- Store approved thresholds in `config/quality_baselines.json`.

## Step 2: Enforce thresholds in CI

- The benchmark job should generate `artifacts/quality/benchmark_metrics.json`.
- `tools/quality/compare_benchmark_to_baseline.py` compares current metrics with baseline thresholds.
- CI fails when populated thresholds are violated.

## Step 3: Rebaseline intentionally

- Only rebaseline after approved architecture or environment changes.
- Record the reason in release notes and scorecard.

## Initial state

The repository currently includes the threshold framework, but default thresholds are unset until the first approved benchmark capture is completed.
