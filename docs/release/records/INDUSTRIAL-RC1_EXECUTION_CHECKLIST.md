# INDUSTRIAL-RC1 Release Execution Checklist

## Preflight

- [ ] Confirm candidate tag `INDUSTRIAL-RC1` points to commit `ea4d810e7a7807f588ba17dc4f74dc6623dc0aaf`.
- [x] Confirm release notes are prepared in docs/release/records/INDUSTRIAL-RC1_RELEASE_NOTES.md.
- [x] Confirm scorecard is prepared in docs/release/records/INDUSTRIAL-RC1_QUALITY_SCORECARD.md.
- [ ] Confirm no unreviewed release-branch changes remain.
- [ ] Confirm clean release baseline (review observed `241` local changes at `2026-04-25T04:00:59Z`).

## CI execution

- [ ] Run quality gates workflow on the release commit.
- [ ] Verify traceability, protocol-compatibility, vulnerability-SLA, fuzz, benchmark, and soak jobs.
- [ ] Archive workflow artifacts linked to the release record.

## Candidate validation

- [x] Configure industrial profile: `cmake -S . -B build-industrial -DZOO_DOMAIN_PROFILE=industrial -DZOO_ENABLE_RUNTIME_ASSURANCE_TEST=ON`.
- [x] Build industrial profile: `cmake --build build-industrial -j4`.
- [x] Run focused assurance tests: `ctest --test-dir build-industrial --output-on-failure -R "test_zoo_assurance_domain_unit|test_zoo_smb_runtime_assurance_unit"`.
- [x] Refresh quality evidence scripts: protocol compatibility matrix, traceability gate, and vulnerability SLA report (2026-04-25T04:00:59Z review run).

## Artifact archive

- [x] Archive docs/release/records/INDUSTRIAL-RC1_RELEASE_NOTES.md.
- [x] Archive docs/release/records/INDUSTRIAL-RC1_QUALITY_SCORECARD.md.
- [x] Archive docs/release/records/INDUSTRIAL-RC1_EVIDENCE_FREEZE_RECORD.md.
- [x] Archive docs/release/records/INDUSTRIAL-RC1_RELEASE_LOG.md.
- [ ] Attach CI-generated artifacts for full release package.

## Approval record

- [ ] Record release owner approval.
- [ ] Record quality gate owner approval.
- [ ] Record security reviewer approval.

## Completion

- [ ] Publish candidate tag and refs.
- [ ] Mark final release operator and timestamp in release log.
