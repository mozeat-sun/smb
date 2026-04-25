# Threat Model And Risk Assessment

Requirement linkage:

- REQ-SEC-001: maintain a threat model for protocol, control plane, and update surface.
- REQ-SEC-004: track vulnerability handling and remediation expectations alongside this model.

## Scope

This threat model covers the current repository baseline for the ZOO Soft Message Bus.

In scope:

- Wire protocol parsing and message framing
- Node registration, service discovery, and control-plane behavior
- Transport selection and message routing between publisher, subscriber, client, and server roles
- Build and release surfaces that affect deployable artifacts
- Private vulnerability reports and remediation records

Out of scope for this revision:

- Hardware root of trust
- Secure boot implementation details
- Third-party deployment environments outside repository-controlled artifacts

## Assets

- Message integrity and receiver correctness
- Service availability and degraded-mode behavior
- Configuration integrity for deterministic-memory and transport limits
- Release artifact integrity and dependency provenance
- Private vulnerability reports and remediation records

## Trust Boundaries

1. External peer to transport boundary:
Incoming bytes cross from untrusted network or shared-memory peers into protocol parsing and routing.

2. Control-plane to runtime boundary:
Service discovery and node registration alter routing and availability state.

3. Build pipeline to release artifact boundary:
Source, dependencies, and generated release artifacts cross CI and packaging workflows.

4. Private security reporting to repository maintenance boundary:
Sensitive vulnerability information is handled outside public issue flow.

## Threat Actors

- Remote unauthenticated peer sending malformed or oversized frames
- Authorized but faulty component causing protocol misuse or service churn
- Supply-chain attacker attempting dependency or artifact tampering
- Insider or compromised automation misconfiguring release or remediation workflow

## Threat Scenarios

| ID | Surface | Threat | Impact | Current controls | Remaining gap |
|---|---|---|---|---|---|
| TM-001 | Protocol | Malformed header or payload causes parser instability | Crash, denial of service, undefined behavior | Header validation, bounded payload sizing, fuzz gate, unit protocol coverage | Continue expanding parser fuzz corpus and transport interop tests |
| TM-002 | Control plane | Fake or stale discovery events manipulate service availability | Misrouting, degraded health, unexpected retries | Service lifecycle tracking, observer notifications, session reconciliation | Add stronger identity and replay protections |
| TM-003 | Transport | Oversized payload or queue pressure exhausts memory | Throughput collapse or memory pressure | Deterministic memory profile, transport buffer cap, watermark telemetry, backpressure tests | Add longer release-grade endurance evidence |
| TM-004 | Release pipeline | Dependency or artifact tampering reaches packaged output | Compromised release artifact | Dependency scan, compatibility policy, release scorecard, CI quality gates | Add stronger provenance and signed-release workflow |
| TM-005 | Vulnerability handling | Private report is not remediated within expected time | Extended exposure window | Security policy, remediation SLA, tracked log, SLA report artifact | Maintain live records and release approval discipline |

## Control Mapping

- REQ-SEC-001: this document is the maintained threat-model baseline.
- REQ-SEC-002: continuous static analysis job in CI.
- REQ-SEC-003: dependency inventory and vulnerability scan job in CI.
- REQ-SEC-004: vulnerability response policy, remediation SLA, tracked log, and SLA report artifact.
- REQ-SAFE-002: bounded-memory controls reduce resource-exhaustion risk.
- REQ-COMP-001: protocol-version policy supports fail-fast mismatch handling.

## Assumptions

- Peers may be buggy or malicious until authenticated transport and identity controls are fully implemented.
- CI artifacts are the authoritative repository evidence for release review.
- Release authority rejects production releases when threat-model or remediation evidence is stale.

## Review Cadence

- Update this document for protocol, control-plane, or release-surface changes.
- Review at each release candidate.
- Record major threat-model revisions in release notes or architecture assessment updates.