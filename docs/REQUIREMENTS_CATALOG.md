# Requirements Catalog

This catalog defines top-level system requirements for the ZOO communication bus.

## Format

- ID format: `REQ-<domain>-<number>`
- Every implementation PR should reference at least one requirement ID.
- Tests should include requirement IDs in test names, comments, or metadata where practical.

## Reliability Requirements

- `REQ-REL-001`: The bus shall start and enter operational state within a bounded startup time for the configured deployment profile.
- `REQ-REL-002`: The bus shall recover from a single-process restart without violating configured recovery time objectives.
- `REQ-REL-003`: The bus shall provide backpressure handling modes for overload conditions.
- `REQ-REL-004`: The bus shall expose health state as ready, live, or degraded.

## Performance Requirements

- `REQ-PERF-001`: The bus shall publish benchmark evidence for throughput and latency by message size class.
- `REQ-PERF-002`: The bus shall track p50, p95, and p99 latency for defined benchmark profiles.
- `REQ-PERF-003`: The bus shall detect benchmark regressions against approved baselines.

## Safety Requirements

- `REQ-SAFE-001`: The bus shall provide deterministic execution behavior for safety-critical message paths.
- `REQ-SAFE-002`: The bus shall support bounded-memory operation in configured deterministic mode.
- `REQ-SAFE-003`: The bus shall maintain requirement-to-test traceability for graded releases.

## Security Requirements

- `REQ-SEC-001`: The bus shall maintain a threat model for protocol, control plane, and update surface.
- `REQ-SEC-002`: The bus shall run continuous static analysis for supported code paths.
- `REQ-SEC-003`: The bus shall run dependency and component inventory scans for each release candidate.
- `REQ-SEC-004`: The bus shall support private reporting and tracked remediation of vulnerabilities.

## Compatibility Requirements

- `REQ-COMP-001`: The bus shall expose an explicit wire protocol versioning mechanism.
- `REQ-COMP-002`: The bus shall publish API and protocol compatibility policy.
- `REQ-COMP-003`: The bus shall document breaking changes and deprecations in each applicable release.
