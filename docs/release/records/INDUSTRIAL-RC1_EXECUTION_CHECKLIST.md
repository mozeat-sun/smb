# INDUSTRIAL-RC1 Release Execution Checklist

## Preflight

- [x] Confirm candidate tag `INDUSTRIAL-RC1` points to commit `a3fa41191b26e327630cc4002db63e61228b059a`.
- [x] Confirm release notes are prepared in docs/release/records/INDUSTRIAL-RC1_RELEASE_NOTES.md.
- [x] Confirm scorecard is prepared in docs/release/records/INDUSTRIAL-RC1_QUALITY_SCORECARD.md.
- [x] Confirm no unreviewed release-branch changes remain (validated at `2026-04-25T04:07:32Z`).
- [x] Confirm clean release baseline (`CHANGED=0` observed at `2026-04-25T04:07:32Z`).

## CI execution

- [ ] Run quality gates workflow on the release commit.
- [x] Verify traceability, protocol-compatibility, vulnerability-SLA, fuzz, benchmark, and soak jobs (local evidence refresh completed on 2026-04-25).
- [ ] Archive workflow artifacts linked to the release record.

## Candidate validation

- [x] Configure industrial profile: `cmake -S . -B build-industrial -DZOO_DOMAIN_PROFILE=industrial -DZOO_ENABLE_RUNTIME_ASSURANCE_TEST=ON`.
- [x] Build industrial profile: `cmake --build build-industrial -j4`.
- [x] Run focused assurance tests: `ctest --test-dir build-industrial --output-on-failure -R "test_zoo_assurance_domain_unit|test_zoo_smb_runtime_assurance_unit"`.
- [x] Refresh quality evidence scripts: protocol compatibility matrix, traceability gate, and vulnerability SLA report (2026-04-25T04:00:59Z review run).
- [x] Run local fuzz, benchmark, and short soak quality jobs to refresh artifacts (`tools/quality/fuzz_protocol.sh`, `tools/quality/benchmark.sh`, `tools/quality/soak_test.sh 1`).

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

- [x] Publish candidate tag and refs (`INDUSTRIAL-RC1` pushed to origin).
- [ ] Mark final release operator and timestamp in release log.
