# INDUSTRIAL-RC1 Quality Scorecard

## Release metadata

- Version: INDUSTRIAL-RC1
- Commit: a3fa41191b26e327630cc4002db63e61228b059a
- Date: 2026-04-25
- Release owner: zoo-release-board

## Current grade claim

- Current grade status: Pre-Grade-1 completion.
- Repository maturity statement: Industrial domain and kernal release path is validated for this candidate baseline; full Grade 1 Industrial exit gates are not yet claimed.
- Unsupported grade claims: Grade 2 Automotive and Grade 3 Military/Aerospace are not claimed.

## Reliability

- Availability target met: Pass for this candidate's focused assurance-domain runtime gate.
- Soak duration completed: Partial local soak evidence run completed (`tools/quality/soak_test.sh 1`).
- Soak failures: None in local refresh run (`failures=0`).
- Recovery time objective met: Not a gate in this focused candidate run.
- Last reliability validation refresh: 2026-04-25T04:00:59Z (`2/2` focused industrial assurance tests passed).

## Performance

- Throughput baseline delta: Reevaluated in local benchmark refresh run (`tools/quality/benchmark.sh`).
- Latency baseline delta: Reevaluated in local benchmark refresh run (`tools/quality/benchmark.sh`).

## Safety and correctness

- Industrial profile build configuration: Pass (`-DZOO_DOMAIN_PROFILE=industrial`).
- Assurance-domain startup policy validation: Pass (`test_zoo_assurance_domain_unit`).
- Runtime assurance startup/admission validation: Pass (`test_zoo_smb_runtime_assurance_unit`).
- Regression suite status: Focused pass for industrial assurance-domain and runtime assurance tests.

## Security

- New critical vulnerabilities: Not reevaluated in this focused candidate run.
- High severity vulnerabilities: Not reevaluated in this focused candidate run.
- Threat model updated: No update required for this focused release note.
- Vulnerability SLA gate: Pass (report refreshed in review run).
- Fuzz protocol gate: Pass (local fuzz refresh run completed).

## Compatibility

- API compatibility status: Compatible for candidate scope.
- Wire protocol compatibility status: Enforced by startup compatibility checks and existing policy artifacts.
- Breaking changes declared: No.
- Deprecation notices complete: Yes.
- Protocol compatibility matrix refresh: Pass (review run).

## Release decision

- Decision: No-Go for external Industrial release publication. Go for internal Industrial RC validation scope.
- Blockers: Release approvals are pending and full CI workflow artifact archive is pending.
- Required mitigations: Complete checklist approvals, run full release workflow on the final tagged commit, archive CI artifacts, and freeze evidence from a clean/tagged baseline.
