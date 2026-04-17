# M3-RC1 Release Execution Checklist

Use this checklist at release execution time for the M3-RC1 candidate.

## Preflight

- [x] Confirm the release candidate is tagged as `M3-RC1` and that the tag points to the approved release commit.
- [x] Confirm [docs/RELEASE_QUALITY_SCORECARD.md](docs/RELEASE_QUALITY_SCORECARD.md) records a `Go` decision for M3-RC1.
- [x] Confirm [docs/releases/M3-RC1_RELEASE_NOTES.md](docs/releases/M3-RC1_RELEASE_NOTES.md) is the release-note record distributed with the candidate.
- [ ] Confirm no unreviewed changes remain in the release branch or release tag target.
	Current note: repository-side Milestone 4 scaffolding changes exist after the tagged release candidate and should not be treated as part of the frozen M3 baseline.

## CI execution

- [ ] Run the `Quality Gates` workflow from [.github/workflows/quality-gates.yml](.github/workflows/quality-gates.yml) on the release commit.
- [ ] Verify successful jobs for traceability, protocol-compatibility, static-analysis, dependency-scan, vulnerability-sla, pre-assessment-readiness, soak, reliability-until-fail, recovery-rto, fault-injection, fuzz, and benchmark.
- [ ] Verify successful jobs for traceability, protocol-compatibility, static-analysis, dependency-scan, vulnerability-sla, pre-assessment-readiness, soak, reliability-until-fail, recovery-rto, fault-injection, fuzz, benchmark, and grade1-exit-status.
- [ ] Preserve the uploaded workflow artifacts for the release record.
	Current note: local artifact evidence is present under `artifacts/quality/`; remote workflow verification is still pending from this environment.

## Artifact archive

- [x] Archive `artifacts/quality/benchmark_summary.json`.
- [x] Archive `artifacts/quality/soak_summary.json`.
- [x] Archive `artifacts/quality/reliability_until_fail_summary.json`.
- [x] Archive `artifacts/quality/traceability_gate_summary.json`.
- [x] Archive `artifacts/quality/vulnerability_sla_summary.json`.
- [x] Archive `artifacts/quality/fuzz_summary.json`.
- [x] Archive `artifacts/quality/protocol_compatibility_matrix.md`.
- [x] Archive `artifacts/quality/pre_assessment_readiness_report.md`.
- [x] Archive `artifacts/quality/pre_assessment_readiness_summary.json`.
- [x] Archive `artifacts/quality/pre_assessment_remediation_plan.md`.
- [x] Archive `artifacts/quality/pre_assessment_remediation_plan_summary.json`.
- [x] Archive `artifacts/quality/static_analysis_summary.json`.
- [x] Archive `artifacts/quality/dependency_scan_summary.json`.
- [x] Archive `artifacts/quality/recovery_rto_report.md`.
- [x] Archive `artifacts/quality/recovery_rto_summary.json`.
- [x] Archive `artifacts/quality/latency_scenario_matrix.md`.
- [x] Archive `artifacts/quality/latency_scenario_matrix.json`.
- [x] Archive `artifacts/quality/grade1_exit_report.md`.
- [x] Archive `artifacts/quality/grade1_exit_summary.json`.
- [x] Archive `docs/releases/GRADE1_LONG_SOAK_EVIDENCE.json`.
- [x] Archive `docs/releases/GRADE1_PRODUCTION_PILOT_EVIDENCE.md`.
	Archive record: [docs/releases/M3-RC1_ASSESSOR_PACKAGE_MANIFEST.md](docs/releases/M3-RC1_ASSESSOR_PACKAGE_MANIFEST.md)

## Approval record

- [ ] Record release owner approval.
- [ ] Record quality gate owner approval.
- [ ] Record security reviewer approval.
- [ ] Attach approvals to the release record with the artifact bundle.

## Rollback and known risk capture

- [ ] Record the rollback target release or tag before deployment.
- [x] Record the known limitations from [docs/releases/M3-RC1_RELEASE_NOTES.md](docs/releases/M3-RC1_RELEASE_NOTES.md).
- [x] Confirm no Critical or High overdue vulnerability findings are open at execution time.

## Completion

- [x] Publish the release notes with the release candidate.
- [x] Store the final artifact bundle, scorecard, and release notes in the release record.
- [x] Mark the release execution timestamp and operator in the release log.
	Release record: [docs/releases/M3-RC1_RELEASE_LOG.md](docs/releases/M3-RC1_RELEASE_LOG.md)