# Architecture Requirements Assessment

This document assesses whether the current ZOO SMB software architecture is sufficient to support the next planned iterations defined in the repository roadmap and requirements catalog.

Assessment scale:

- Present: architecturally supported now
- Partial: basic structure exists, but important controls or guarantees are missing
- Missing: not yet represented as a clear architectural capability

Overall conclusion:

- The current architecture is sufficient to support the next industrial-grade hardening iterations without a rewrite.
- The current architecture remains partial for later automotive-grade and military/aerospace-grade iterations, but now includes runtime-selectable profile composition and admission-time assurance checks.
- The repository is currently best characterized as late Milestone M3 with early Milestone M4 scaffolding in place.
- Current maintained repository verification baseline is green: full build, full CTest, and full local example matrix pass.

## Requirement Matrix

| Requirement ID | Status | Assessment | Evidence in current architecture | Gap for next iterations |
|---|---|---|---|---|
| REQ-REL-001 | Partial | Startup lifecycle exists, but bounded startup time is not defined or enforced as an architectural contract. | Runtime create/start APIs and ready checks exist in SMB runtime and core interfaces. | Need startup SLA, timeout budget, readiness criteria, and validation under load. |
| REQ-REL-002 | Partial | Restart scaffolding now has an architecture baseline for persistence and recovery, but there is still no implemented recovery manager, WAL, checkpoint store, or measured restart guarantee. | Runtime lifecycle and HA state model exist, and recovery/persistence architecture now defines durable-state scope and restart sequence. | Need implementation of recovery components plus measured RTO evidence. |
| REQ-REL-003 | Present | The architecture already includes configurable backpressure and overload control. | Flow-control watermarks, ingress protection, and transport manager backpressure metrics are already defined. | Need policy completeness across all message classes and release evidence. |
| REQ-REL-004 | Partial | Health exists conceptually and in runtime state, but unified repository-level liveness, readiness, and degraded semantics are not yet fully standardized. | Runtime degraded state and ready APIs exist; transport health tracking is documented. | Need one consistent health contract across runtime, transport, routing, and service layers. |
| REQ-PERF-001 | Partial | The architecture is now observable enough to publish reproducible throughput and pub/sub latency benchmarks, though broader end-to-end performance coverage still needs additional scenarios beyond bench6. | Metrics and observability are called out in the SMB architecture; `tools/quality/benchmark.sh` now generates rebuildable throughput artifacts and stabilized bench6 latency captures from isolated benchmark modes. | Need broader scenario coverage and controlled-environment capture discipline for higher-assurance grades. |
| REQ-PERF-002 | Partial | Throughput regression enforcement and bench6 p99/jitter regression enforcement now exist, but p50/p95 publication is still informational rather than separately threshold-gated. | Telemetry and metrics structures exist; `docs/readiness/BENCHMARK_BASELINE_PROCESS.md` and the benchmark gate now define and enforce the approved throughput plus bench6 latency capture workflow. | Need explicit policy on whether p50/p95 should remain informational or become separately gated. |
| REQ-PERF-003 | Present | Regression detection is now active for aggregate throughput, per-message-size throughput, and stabilized bench6 latency metrics. | Repository includes benchmark schema, calibrated throughput and latency baselines, and comparison tooling in benchmark quality scripts and config baselines. | Continue periodic controlled-environment rebaseline discipline as hardware and runtime behavior evolve. |
| REQ-SAFE-001 | Partial | Deterministic memory profile enforcement now exists for startup budget validation and bounded payload sizing, but message-class mapping and timing evidence are still missing. | Deterministic profile config, startup budget checks, and memory telemetry are implemented in SMB config/runtime and metrics reporting. | Need message-class mapping and worst-case timing evidence. |
| REQ-SAFE-002 | Partial | Bounded-memory behavior is now enforced for configured transport payload budgets and memory-pool watermarks, but full release-grade endurance evidence is still missing. | Deterministic bounded-memory profile validates queue/buffer budgets at startup, caps message payload allocation size, and exposes memory-pool watermark metrics. | Need sustained endurance evidence and release-gate enforcement. |
| REQ-SAFE-003 | Present | Repository-level requirement-to-test traceability is now maintained with CI enforcement and audit artifacts. | Requirements catalog, traceability matrix generation, audit-sample gate, and requirement-tagged SMB and performance verification sources are present in repository CI and test trees. | Continue broadening requirement IDs in lower-priority modules as routine maintenance. |
| REQ-SEC-001 | Present | A maintained threat-model baseline now exists for protocol, control plane, and release surfaces. | `docs/security/THREAT_MODEL.md` defines assets, trust boundaries, threat scenarios, and current controls tied to repository quality evidence. | Need stronger identity, replay-protection, and secure-update architecture decisions for higher assurance grades. |
| REQ-SEC-002 | Present | Continuous static analysis is now supported at the repository quality-gate level. | Quality workflow includes static-analysis automation. | Need tuning and policy thresholds so results become release-gating evidence. |
| REQ-SEC-003 | Present | Dependency and component inventory scanning is now supported at the repository quality-gate level. | Quality workflow includes dependency inventory generation. | Need stronger provenance and vulnerability correlation for higher assurance grades. |
| REQ-SEC-004 | Present | Vulnerability reporting and tracked remediation workflow now exist with explicit SLA evidence. | `SECURITY.md`, `docs/security/VULNERABILITY_REMEDIATION_SLA.md`, `docs/security/VULNERABILITY_REMEDIATION_LOG.csv`, and `tools/quality/vulnerability_sla_report.sh` provide tracked remediation policy and CI artifacts. | Need live operational discipline as findings volume grows. |
| REQ-COMP-001 | Partial | Protocol versioning now has a bus-wide policy baseline, but transport-wide implementation and interop tests are still missing. | Protocol compatibility policy now defines wire major/minor version rules and fail-fast mismatch behavior. | Need unified implementation across transports and version-interop tests. |
| REQ-COMP-002 | Partial | Compatibility policy is explicit and a repository quality gate now generates the compatibility matrix artifact, but transport-level interoperability testing is still missing. | Protocol compatibility policy now defines scope, rules, and release requirements, and CI generates `artifacts/quality/protocol_compatibility_matrix.md`. | Need unified implementation across transports and version-interop tests. |
| REQ-COMP-003 | Partial | Deprecation and compatibility governance are now integrated into the release template, but completed per-release declarations and migration reporting still depend on release execution discipline. | Protocol compatibility policy now requires deprecation cycle and release declarations, and the release notes template captures mandatory compatibility fields. | Need release-time completion checks and migration reporting. |

## Summary by category

### Strong enough now

- Layered separation of API, core runtime, routing/policy, transport/protocol, and platform
- Flow-control and overload-protection foundations
- Runtime lifecycle and degraded-state scaffolding
- Transport metrics and observability hooks
- Memory-pool-oriented implementation direction in core code paths

### Good foundation but not yet sufficient

- Unified health contract
- Broader latency scenario coverage beyond the current stabilized bench6 path
- Mixed-criticality policy separation at channel/service granularity (current profile override is runtime-instance scoped)

### Still missing as architectural capabilities

- Automotive-grade mixed-criticality partitioning and freedom-from-interference controls
- High-assurance security architecture needed for military/aerospace progression

## Decision

The current architecture meets the needs of the next subsequent iterations if those iterations are focused on:

- protocol freeze
- backpressure hardening
- observability
- benchmark and soak evidence
- security workflow maturation

The remaining short-term release blocker is no longer the basic latency percentile benchmark path from M2; that path is now stabilized and regression-gated.

In practical repository terms, this means the current codebase can complete the remaining M2 benchmark-governance work and the early M3 traceability rollout without architectural rework.

The current architecture does not yet meet the needs of later iterations if those iterations require:

- deterministic safety channels with bounded worst-case latency
- formally bounded memory behavior
- durable recovery guarantees
- full compatibility governance across transports
- certification-oriented safety and security assurance

Repository synchronization note (2026-05-31):

- Core SMB source paths have been aligned with repository code specifications in current active development slices.
- The architecture and readiness posture statements remain unchanged at grade level, but confidence in repository baseline stability is higher due to current full verification pass.

## Recommended architecture priorities

1. Implement the bus-wide wire protocol version and compatibility contract.
2. Implement the deterministic and bounded-memory operating profile.
3. Standardize health semantics across runtime, service, routing, and transport.
4. Implement the recovery and persistence architecture for restart-sensitive deployments.
5. Integrate compatibility, recovery, and deterministic-profile checks into release gates.
6. Extend runtime profile override into per-channel/per-service policy mapping for mixed-criticality deployments.