# M3-RC1 Release Execution Checklist

Use this checklist at release execution time for the M3-RC1 candidate.

## Preflight

- [ ] Confirm the release candidate is tagged as `M3-RC1` and that the tag points to the approved release commit.
- [ ] Confirm [docs/RELEASE_QUALITY_SCORECARD.md](docs/RELEASE_QUALITY_SCORECARD.md) records a `Go` decision for M3-RC1.
- [ ] Confirm [docs/releases/M3-RC1_RELEASE_NOTES.md](docs/releases/M3-RC1_RELEASE_NOTES.md) is the release-note record distributed with the candidate.
- [ ] Confirm no unreviewed changes remain in the release branch or release tag target.

## CI execution

- [ ] Run the `Quality Gates` workflow from [.github/workflows/quality-gates.yml](.github/workflows/quality-gates.yml) on the release commit.
- [ ] Verify successful jobs for traceability, protocol-compatibility, static-analysis, dependency-scan, vulnerability-sla, soak, reliability-until-fail, fault-injection, fuzz, and benchmark.
- [ ] Preserve the uploaded workflow artifacts for the release record.

## Artifact archive

- [ ] Archive `artifacts/quality/benchmark_summary.json`.
- [ ] Archive `artifacts/quality/soak_summary.json`.
- [ ] Archive `artifacts/quality/reliability_until_fail_summary.json`.
- [ ] Archive `artifacts/quality/traceability_gate_summary.json`.
- [ ] Archive `artifacts/quality/vulnerability_sla_summary.json`.
- [ ] Archive `artifacts/quality/fuzz_summary.json`.
- [ ] Archive `artifacts/quality/protocol_compatibility_matrix.md`.

## Approval record

- [ ] Record release owner approval.
- [ ] Record quality gate owner approval.
- [ ] Record security reviewer approval.
- [ ] Attach approvals to the release record with the artifact bundle.

## Rollback and known risk capture

- [ ] Record the rollback target release or tag before deployment.
- [ ] Record the known limitations from [docs/releases/M3-RC1_RELEASE_NOTES.md](docs/releases/M3-RC1_RELEASE_NOTES.md).
- [ ] Confirm no Critical or High overdue vulnerability findings are open at execution time.

## Completion

- [ ] Publish the release notes with the release candidate.
- [ ] Store the final artifact bundle, scorecard, and release notes in the release record.
- [ ] Mark the release execution timestamp and operator in the release log.