# M3-RC1 Release Notes

Release candidate for Milestone 3 repository delivery.

## Release metadata

- Version: M3-RC1
- Commit: release tag M3-RC1
- Date: 2026-04-11
- Release owner: zoo-release-board

## Compatibility declaration

- API compatibility status: Compatible within the current Milestone 3 release scope. No new incompatible API declaration is recorded for this release candidate.
- Wire compatibility status: Protocol baseline remains v1.0. Exact v1.0 peers are accepted and incompatible version tuples are blocked according to the protocol compatibility policy and generated matrix artifact.
- Breaking changes: None declared for M3-RC1.
- Deprecation notices: None required for this release candidate.

## Summary

- Highlights: Milestone 3 release governance is now backed by active benchmark, traceability, vulnerability-SLA, reliability, fuzz, and protocol-compatibility artifacts. Bench6 pub/sub latency is stabilized and now participates in active regression gating through p99 and jitter thresholds.
- Fixes: Bench6 now performs a publisher warmup association before measured traffic, eliminating the lazy-registration negotiation race that previously caused publish success with zero receives. Release scorecard, compatibility governance, and evidence artifacts are aligned for Milestone 3 release execution.
- Known limitations: Formal restart and recovery time objective evidence is not yet a Milestone 3 gate artifact. Broader latency scenario coverage and transport-wide interoperability remain follow-on work for higher assurance levels.

## Migration guidance

- Required user actions: None for current protocol baseline v1.0 consumers operating within the documented compatibility window.
- Configuration changes: None required for existing CI and benchmark profiles.
- Rollback notes: Roll back by redeploying the previous approved release candidate or release tag and regenerating release evidence for that target commit. Do not carry forward M3-RC1 artifacts after rollback; archive the superseded artifact bundle separately.

## Evidence references

- Compatibility matrix artifact: artifacts/quality/protocol_compatibility_matrix.md
- Benchmark or soak artifacts: artifacts/quality/benchmark_summary.json, artifacts/quality/soak_summary.json, artifacts/quality/reliability_until_fail_summary.json
- Security or traceability artifacts: artifacts/quality/vulnerability_sla_summary.json, artifacts/quality/fuzz_summary.json, artifacts/quality/traceability_gate_summary.json