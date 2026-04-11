# M3-RC1 Assessor Package Manifest

This manifest enumerates the repository evidence package prepared for Milestone 3 review and Milestone 4 pre-assessment readiness.

## Baseline identity

- Release tag: M3-RC1
- Release commit: 675bff26c55e712b1f3be62693f4b5cb9dd821c0
- Package prepared: 2026-04-11T01:59:36Z

## Included documents

| Item | Path | Status | Notes |
|---|---|---|---|
| Release scorecard | `docs/RELEASE_QUALITY_SCORECARD.md` | Included | Records `Go` decision for M3-RC1 |
| Release notes | `docs/releases/M3-RC1_RELEASE_NOTES.md` | Included | Candidate compatibility and limitations declared |
| Execution checklist | `docs/releases/M3-RC1_EXECUTION_CHECKLIST.md` | Included | Updated with current completion status |
| Evidence freeze record | `docs/releases/M3-RC1_EVIDENCE_FREEZE_RECORD.md` | Included | Current local freeze record |
| Independent review record | `docs/releases/M3-RC1_INDEPENDENT_SAFETY_REVIEW.md` | Included | Pre-review completed with actions |
| Safety case skeleton | `docs/SAFETY_CASE_SKELETON.md` | Included | M4 package structure baseline |
| Toolchain and change control | `docs/TOOLCHAIN_BASELINE_AND_CHANGE_CONTROL.md` | Included | Controlled baseline policy |

## Included artifact evidence

| Item | Path | Status | Notes |
|---|---|---|---|
| Benchmark summary | `artifacts/quality/benchmark_summary.json` | Included | Throughput and bench6 latency metrics |
| Soak summary | `artifacts/quality/soak_summary.json` | Included | 40 passes, 0 failures |
| Reliability-until-fail summary | `artifacts/quality/reliability_until_fail_summary.json` | Included | Passed |
| Traceability gate summary | `artifacts/quality/traceability_gate_summary.json` | Included | Passed |
| Traceability audit sample | `artifacts/quality/traceability_audit_sample.md` | Included | Requirement-to-evidence sample |
| Vulnerability SLA summary | `artifacts/quality/vulnerability_sla_summary.json` | Included | Passed, no overdue findings |
| Fuzz summary | `artifacts/quality/fuzz_summary.json` | Included | Executed with one target |
| Protocol compatibility matrix | `artifacts/quality/protocol_compatibility_matrix.md` | Included | v1.0 baseline matrix |

## Pending attachments before external handoff

| Item | Expected source | Status | Reason |
|---|---|---|---|
| Static-analysis artifact bundle | CI quality workflow | Pending | Not present in the local frozen artifact set |
| Dependency-scan artifact bundle | CI quality workflow | Pending | Not present in the local frozen artifact set |
| Named reviewer approvals | Release execution record | Pending | Human sign-off required |
| External assessor findings | Pre-assessment execution | Pending | Outside repository-only execution scope |

## Package handoff note

This package is sufficient for internal readiness review and pre-assessment preparation. It is not a substitute for a completed external advisor review or a certified baseline release package.