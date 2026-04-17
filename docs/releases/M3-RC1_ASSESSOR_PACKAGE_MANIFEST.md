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
| Pre-assessment readiness report | CI `pre-assessment-readiness` workflow job | Prepared locally; CI attachment pending | Local file exists in `artifacts/quality/pre_assessment_readiness_report.md`; attach matching CI artifact from the final handoff baseline |
| Pre-assessment readiness summary | CI `pre-assessment-readiness` workflow job | Prepared locally; CI attachment pending | Local file exists in `artifacts/quality/pre_assessment_readiness_summary.json`; attach matching CI artifact from the final handoff baseline |
| Pre-assessment remediation plan | CI `pre-assessment-readiness` workflow job | Prepared locally; CI attachment pending | Local file exists in `artifacts/quality/pre_assessment_remediation_plan.md`; attach matching CI artifact from the final handoff baseline |
| Pre-assessment remediation summary | CI `pre-assessment-readiness` workflow job | Prepared locally; CI attachment pending | Local file exists in `artifacts/quality/pre_assessment_remediation_plan_summary.json`; attach matching CI artifact from the final handoff baseline |
| Static-analysis artifact bundle | CI quality workflow | Local fallback only; CI bundle pending | Local `static_analysis_summary.json` and `cppcheck.txt` were generated with status `skipped` because `cppcheck` is not installed in this environment |
| Dependency-scan artifact bundle | CI quality workflow | Local fallback only; CI bundle pending | Local `dependency_inventory.csv` and `dependency_scan_summary.json` were generated with status `skipped` because `trivy` is not installed in this environment |
| Named reviewer approvals | Release execution record | Pending | Human sign-off required |
| External assessor findings | Pre-assessment execution | Pending | Outside repository-only execution scope |

## Package handoff note

This package is sufficient for internal readiness review and pre-assessment preparation. It is not a substitute for a completed external advisor review or a certified baseline release package.

## Post-freeze M4 pre-assessment addendum (2026-04-16)

This addendum tracks repository evidence added after the original M3-RC1 freeze record. It does not alter the frozen M3 baseline identity above.

### Added documents

| Item | Path | Status | Notes |
|---|---|---|---|
| Pre-assessment findings log | `docs/PRE_ASSESSMENT_FINDINGS_LOG.csv` | Included | Internal round `INT-R1-2026-04-16` with owners, target dates, and remediation notes |
| Pre-assessment readiness summary | `docs/PRE_ASSESSMENT_READINESS_SUMMARY.md` | Included | Published stakeholder summary for internal round `INT-R1-2026-04-16` |
| Grade 1 exit checklist | `docs/GRADE1_EXIT_CHECKLIST.md` | Included | Defines Grade 1 gate IDs, evidence paths, and repository scope boundary |
| Grade 1 long-soak evidence intake | `docs/releases/GRADE1_LONG_SOAK_EVIDENCE.json` | Included (template) | Intake record for independently operated 30-day soak evidence used by gate `G1-REL-001` |
| Grade 1 production pilot evidence intake | `docs/releases/GRADE1_PRODUCTION_PILOT_EVIDENCE.md` | Included (template) | Intake record for external-user pilot evidence used by gate `G1-EXIT-003` |

### Added generated artifacts

| Item | Path | Status | Notes |
|---|---|---|---|
| Pre-assessment readiness report | `artifacts/quality/pre_assessment_readiness_report.md` | Included (local) | Generated locally; CI-aligned artifact still required for final external handoff baseline |
| Pre-assessment readiness summary | `artifacts/quality/pre_assessment_readiness_summary.json` | Included (local) | Latest local generation reflects 5 findings, 4 open |
| Pre-assessment remediation plan | `artifacts/quality/pre_assessment_remediation_plan.md` | Included (local) | Findings-linked action plan with owner and due-state rollup |
| Pre-assessment remediation summary | `artifacts/quality/pre_assessment_remediation_plan_summary.json` | Included (local) | Latest local generation reports 4 open actions, all on-track |
| Static analysis summary | `artifacts/quality/static_analysis_summary.json` | Included (local fallback) | Local generation reports `skipped` because `cppcheck` is unavailable in this environment |
| Dependency scan summary | `artifacts/quality/dependency_scan_summary.json` | Included (local fallback) | Local generation reports `skipped` because `trivy` is unavailable in this environment |
| Recovery RTO report | `artifacts/quality/recovery_rto_report.md` | Included (local) | Latest local generation reports restart-cycle p95 below 5000 ms threshold |
| Recovery RTO summary | `artifacts/quality/recovery_rto_summary.json` | Included (local) | Latest local generation includes per-run restart-cycle timings |
| Latency scenario matrix | `artifacts/quality/latency_scenario_matrix.md` | Included (local) | Benchmark matrix now covers bench5 RPC and bench6 pub/sub latency paths |
| Latency scenario matrix data | `artifacts/quality/latency_scenario_matrix.json` | Included (local) | Machine-readable scenario coverage artifact for release and assessor review |
| Grade 1 exit status report | `artifacts/quality/grade1_exit_report.md` | Included (local) | Consolidated status of Grade 1 gates from current artifact set |
| Grade 1 exit status data | `artifacts/quality/grade1_exit_summary.json` | Included (local) | Machine-readable gate pass/blocked rollup for Grade 1 promotion tracking |