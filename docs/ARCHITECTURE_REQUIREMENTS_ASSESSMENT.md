# Architecture Requirements Assessment

This document assesses whether the current ZOO SMB software architecture is sufficient to support the next planned iterations defined in the repository roadmap and requirements catalog.

Assessment scale:

- Present: architecturally supported now
- Partial: basic structure exists, but important controls or guarantees are missing
- Missing: not yet represented as a clear architectural capability

Overall conclusion:

- The current architecture is sufficient to support the next industrial-grade hardening iterations without a rewrite.
- The current architecture is not yet sufficient for later automotive-grade and military/aerospace-grade iterations without structural additions.

## Requirement Matrix

| Requirement ID | Status | Assessment | Evidence in current architecture | Gap for next iterations |
|---|---|---|---|---|
| REQ-REL-001 | Partial | Startup lifecycle exists, but bounded startup time is not defined or enforced as an architectural contract. | Runtime create/start APIs and ready checks exist in SMB runtime and core interfaces. | Need startup SLA, timeout budget, readiness criteria, and validation under load. |
| REQ-REL-002 | Partial | Restart scaffolding now has an architecture baseline for persistence and recovery, but there is still no implemented recovery manager, WAL, checkpoint store, or measured restart guarantee. | Runtime lifecycle and HA state model exist, and recovery/persistence architecture now defines durable-state scope and restart sequence. | Need implementation of recovery components plus measured RTO evidence. |
| REQ-REL-003 | Present | The architecture already includes configurable backpressure and overload control. | Flow-control watermarks, ingress protection, and transport manager backpressure metrics are already defined. | Need policy completeness across all message classes and release evidence. |
| REQ-REL-004 | Partial | Health exists conceptually and in runtime state, but unified repository-level liveness, readiness, and degraded semantics are not yet fully standardized. | Runtime degraded state and ready APIs exist; transport health tracking is documented. | Need one consistent health contract across runtime, transport, routing, and service layers. |
| REQ-PERF-001 | Partial | The architecture is observable enough to publish benchmarks, but benchmark publication is still process tooling rather than a product-level architecture contract. | Metrics and observability are called out in the SMB architecture and examples include performance benchmarking. | Need approved benchmark profiles and release-quality publication discipline. |
| REQ-PERF-002 | Partial | The architecture can support latency metrics, but p50/p95/p99 definitions are not yet formalized in the transport or runtime contract. | Telemetry and metrics structures exist; quality-gate scripts now provide benchmark scaffolding. | Need canonical benchmark metrics schema and sampling method. |
| REQ-PERF-003 | Partial | Regression detection is now scaffolded in CI, but the architecture does not yet define stable baselines per deployment profile. | Repository includes baseline framework and comparison tooling. | Need approved baselines and environment control for trustworthy comparisons. |
| REQ-SAFE-001 | Partial | Deterministic operation now has an architecture baseline, but the profile is not yet enforced in implementation and there is no worst-case timing evidence. | Deterministic bounded-memory profile now defines deterministic rules, budgets, and allowed behavior. | Need profile enforcement, message-class mapping, and timing evidence. |
| REQ-SAFE-002 | Partial | Bounded-memory behavior now has an architecture baseline, but startup validation and hot-path enforcement are not yet implemented. | Deterministic bounded-memory profile defines budget rules, bounded replay, and disallowed unbounded behavior. | Need implementation of startup budget validation and enforcement tests. |
| REQ-SAFE-003 | Partial | Repository-level traceability scaffolding now exists, but the current architecture and implementation are not yet systematically tagged to requirements. | Requirements catalog and traceability tooling have been added. | Need requirement IDs embedded in tests, design decisions, and critical code paths. |
| REQ-SEC-001 | Partial | A maintained threat-model baseline now exists, but identity, replay protection, and update-trust controls are still not fully designed into the product architecture. | SMB threat model and security architecture now define threats, boundaries, and control allocation. | Need concrete identity, replay-protection, and secure-update architecture decisions. |
| REQ-SEC-002 | Present | Continuous static analysis is now supported at the repository quality-gate level. | Quality workflow includes static-analysis automation. | Need tuning and policy thresholds so results become release-gating evidence. |
| REQ-SEC-003 | Present | Dependency and component inventory scanning is now supported at the repository quality-gate level. | Quality workflow includes dependency inventory generation. | Need stronger provenance and vulnerability correlation for higher assurance grades. |
| REQ-SEC-004 | Partial | Vulnerability reporting exists, but tracked remediation workflow is still more governance than architecture. | Root security policy and release scorecard process exist. | Need SLA tracking, remediation evidence, and link to release authority gates. |
| REQ-COMP-001 | Partial | Protocol versioning now has a bus-wide policy baseline, but transport-wide implementation and interop tests are still missing. | Protocol compatibility policy now defines wire major/minor version rules and fail-fast mismatch behavior. | Need unified implementation across transports and version-interop tests. |
| REQ-COMP-002 | Partial | Compatibility policy is now explicit, but not yet enforced through release automation and compatibility matrices. | Protocol compatibility policy now defines scope, rules, and release requirements. | Need compatibility matrix generation and release-gate enforcement. |
| REQ-COMP-003 | Partial | Deprecation and compatibility governance are now defined at policy level, but not yet fully integrated into release workflow. | Protocol compatibility policy now requires deprecation cycle and release declarations. | Need release-note enforcement and migration reporting. |

## Summary by category

### Strong enough now

- Layered separation of API, core runtime, routing/policy, transport/protocol, and platform
- Flow-control and overload-protection foundations
- Runtime lifecycle and degraded-state scaffolding
- Transport metrics and observability hooks
- Memory-pool-oriented implementation direction in core code paths

### Good foundation but not yet sufficient

- Unified health contract
- Traceability coverage across code and tests

### Still missing as architectural capabilities

- Automotive-grade mixed-criticality partitioning and freedom-from-interference controls
- High-assurance security architecture needed for military/aerospace progression

## Decision

The current architecture meets the needs of the next subsequent iterations if those iterations are focused on:

- protocol freeze
- backpressure hardening
- observability
- benchmark and soak evidence
- traceability rollout
- security workflow maturation

The current architecture does not yet meet the needs of later iterations if those iterations require:

- deterministic safety channels with bounded worst-case latency
- formally bounded memory behavior
- durable recovery guarantees
- full compatibility governance across transports
- certification-oriented safety and security assurance

## Recommended architecture priorities

1. Implement the bus-wide wire protocol version and compatibility contract.
2. Implement the deterministic and bounded-memory operating profile.
3. Standardize health semantics across runtime, service, routing, and transport.
4. Implement the recovery and persistence architecture for restart-sensitive deployments.
5. Integrate compatibility, recovery, and deterministic-profile checks into release gates.