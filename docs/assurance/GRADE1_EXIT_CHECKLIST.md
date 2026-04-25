# Grade 1 Industrial Exit Checklist

Use this checklist to determine whether the repository can claim Grade 1 Industrial completion.

## Usage

- Run repository quality gates and collect artifacts in `artifacts/quality/`.
- Generate Grade 1 status artifacts with `bash tools/quality/grade1_exit_gate.sh`.
- Record any blocked gates in backlog items with owner and target date.

## Automated status artifacts

- `artifacts/quality/grade1_exit_report.md`
- `artifacts/quality/grade1_exit_summary.json`

## Gate checklist

### Reliability and recovery

- [ ] `G1-REL-001` 30-day soak evidence (`duration_minutes >= 43200` in `artifacts/quality/soak_summary.json` or `docs/release/records/GRADE1_LONG_SOAK_EVIDENCE.json` with `status=completed|verified`)
- [ ] `G1-REL-002` Recovery time objective under 5 seconds (`restart_cycle_max_ms <= 5000` in `artifacts/quality/recovery_rto_summary.json`)
- [ ] Fault-injection and reliability-until-fail artifacts archived for release baseline

### Performance evidence

- [ ] `G1-EXIT-001` Benchmark evidence includes throughput and latency (`p50`, `p95`, `p99`) in `artifacts/quality/benchmark_summary.json`
- [ ] Latency scenario matrix includes both RPC and pub/sub paths in `artifacts/quality/latency_scenario_matrix.json`

### Release governance

- [ ] `G1-EXIT-002` Release checklist approvals are fully signed in `docs/release/records/M3-RC1_EXECUTION_CHECKLIST.md`
- [ ] Release evidence freeze and archive records are complete for the claimed baseline

### External deployment evidence

- [ ] `G1-EXIT-003` Production pilot evidence lists at least two external industrial users in `docs/release/records/GRADE1_PRODUCTION_PILOT_EVIDENCE.md`

### Evidence intake files

- [ ] `docs/release/records/GRADE1_LONG_SOAK_EVIDENCE.json` is populated from a real 30-day operational window.
- [ ] `docs/release/records/GRADE1_PRODUCTION_PILOT_EVIDENCE.md` is populated with at least two verified external user records.

## Repository scope boundary

The repository can automate and verify artifact quality and many gate metrics, but cannot self-assert external user pilot execution without independently provided evidence.
