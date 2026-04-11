# Release Quality Scorecard

Complete this scorecard for every release candidate.

## Release metadata

- Version: M3-RC1
- Commit: release tag M3-RC1
- Date: 2026-04-11
- Release owner: zoo-release-board

## Current grade claim

- Current grade status: Pre-Grade-1 completion.
- Repository maturity statement: Late Milestone M3 with repository-level M4 scaffolding in place; this release candidate does not claim that Grade 1 Industrial exit gates have been fully achieved.
- Unsupported grade claims: Grade 2 Automotive and Grade 3 Military/Aerospace are not claimed for this release candidate.

## Reliability

- Availability target met: Yes for Milestone 3 release-gate scope; latest soak artifact completed 40 of 40 passing CI iterations with 0 failures.
- Soak duration completed: 1 minute local CI-mode soak capture recorded in artifacts/quality/soak_summary.json.
- Soak failures: 0
- MTBF estimate: No failure observed across 40 soak iterations in the latest local artifact set.
- Recovery time objective met: Not a Milestone 3 release gate. Formal restart/RTO evidence remains future work.

## Performance

- Throughput baseline delta: Pass. Latest CI-profile benchmark artifact measured 249 msg/s against 200 msg/s minimum aggregate throughput, with all per-size throughput thresholds satisfied.
- p50 latency delta: Informational only. Latest bench6 artifact measured 1547.264 ms.
- p95 latency delta: Informational only. Latest bench6 artifact measured 2931.166 ms.
- p99 latency delta: Pass. Latest bench6 artifact measured 3064.077 ms against 3200 ms CI threshold.
- Jitter envelope status: Pass. Latest bench6 artifact measured 3091.866 ms against 3200 ms CI threshold.

## Safety and correctness

- Requirements traceability coverage: Pass. Traceability gate reports 0 unlinked requirements and 0 missing required test links.
- Critical requirement test pass rate: 100% for the traceability gate's required verification set in the latest artifact run.
- Deterministic behavior checks: Pass for current Milestone 3 repository gate scope; deterministic memory-profile validation and payload budget enforcement are implemented, while broader worst-case timing evidence remains future work.
- Regression suite status: Pass for the latest local release evidence set including benchmark, soak, repeat-until-fail, traceability, fuzz, and vulnerability SLA artifacts.

## Security

- New critical vulnerabilities: 0 tracked in artifacts/quality/vulnerability_sla_summary.json.
- High severity vulnerabilities: 0 tracked in artifacts/quality/vulnerability_sla_summary.json.
- Mean time to remediate: 0.0 days in the current tracked log artifact.
- Threat model updated: Yes
- Fuzz and static analysis status: Pass for fuzz artifact generation and repository security evidence scope. Static-analysis automation is part of CI workflow and should be included in release-run approvals.

## Compatibility

- API compatibility status: Compatible within current Milestone 3 release scope; no new incompatible API declaration recorded for this release candidate.
- Wire protocol compatibility status: v1.0 baseline with exact v1.0 compatibility accepted and mismatches blocked per artifacts/quality/protocol_compatibility_matrix.md.
- Breaking changes declared: No
- Deprecation notices complete: Yes, with no active deprecation notices required for this release candidate.

## Release decision

- Decision: Go for Milestone 3 release candidate.
- Blockers: No repository-level Milestone 3 blockers remain. Higher-grade gaps such as formal RTO guarantees, broader latency scenario coverage, and transport-wide interoperability are not Milestone 3 release blockers.
- Required mitigations: Execute [docs/releases/M3-RC1_EXECUTION_CHECKLIST.md](docs/releases/M3-RC1_EXECUTION_CHECKLIST.md), run the full CI workflow on the release commit, and archive the generated artifacts alongside [docs/releases/M3-RC1_RELEASE_NOTES.md](docs/releases/M3-RC1_RELEASE_NOTES.md).
- Approvals: Release owner, quality gate owner, and security reviewer sign-off required at release execution time.
