# INDUSTRIAL-RC1 Release Notes

Industrial domain candidate release focused on domain and kernal assurance hardening.

## Release metadata

- Version: INDUSTRIAL-RC1
- Commit: ea4d810e7a7807f588ba17dc4f74dc6623dc0aaf
- Date: 2026-04-25
- Release owner: zoo-release-board

## Compatibility declaration

- API compatibility status: Compatible within current Industrial RC scope.
- Wire compatibility status: Protocol baseline remains v1.x compatibility with major-version enforcement and minor-version backward checks.
- Breaking changes: None declared for INDUSTRIAL-RC1.
- Deprecation notices: None required for this release candidate.

## Summary

- Highlights: Industrial profile path is now release-validated for domain and kernal composition. Runtime supports per-instance domain profile override and admission-time assurance checks in heartbeat path.
- Fixes: Runtime assurance test registration is enabled for Industrial validation runs; admission and partition-range checks are enforced by domain and assurance contracts.
- Known limitations: This candidate does not claim Grade 1 Industrial exit completion. Long-duration soak, external pilot evidence, and full release-governance approvals remain required for final grade claim.

## Readiness verdict

- Internal candidate readiness: Ready for internal Industrial RC validation scope.
- External release readiness: Not ready.
- Blocking conditions: Candidate tag publication pending, approvals pending, full CI release-gate artifact archive pending, and working tree not clean during review (`241` local changes).

## Migration guidance

- Required user actions: Configure industrial builds with `-DZOO_DOMAIN_PROFILE=industrial`.
- Configuration changes: For assurance test coverage in CTest use `-DZOO_ENABLE_RUNTIME_ASSURANCE_TEST=ON`.
- Rollback notes: Roll back to the previously approved release candidate tag and regenerate evidence for that baseline.

## Evidence references

- Compatibility matrix artifact: artifacts/quality/protocol_compatibility_matrix.md
- Benchmark or soak artifacts: artifacts/quality/benchmark_summary.json, artifacts/quality/soak_summary.json, artifacts/quality/reliability_until_fail_summary.json
- Security or traceability artifacts: artifacts/quality/vulnerability_sla_summary.json, artifacts/quality/fuzz_summary.json, artifacts/quality/traceability_gate_summary.json
- Industrial profile validation run: build-industrial with `ZOO_DOMAIN_PROFILE=industrial`, tests `test_zoo_assurance_domain_unit` and `test_zoo_smb_runtime_assurance_unit` passed.
- Review evidence refresh (2026-04-25T04:00:59Z): configure/build succeeded, focused industrial assurance tests passed (`2/2`), protocol compatibility matrix generated, traceability gate passed, vulnerability SLA gate passed.
