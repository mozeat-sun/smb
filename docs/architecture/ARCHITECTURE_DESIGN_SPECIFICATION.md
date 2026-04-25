# Architecture Design Specification

Date: 2026-04-11
Scope: Repository-wide architecture design rules and current baseline for ZOO SMB

## 1. Purpose

This document defines the architecture design baseline for ZOO SMB and the required constraints for future design changes.

Goals:
- Keep architecture decisions explicit, reviewable, and traceable.
- Enforce deterministic behavior boundaries for reliability and safety.
- Ensure module responsibilities and interfaces stay stable and testable.
- Align implementation evidence with requirements and release governance.

## 2. Applicability

This specification applies to:
- Runtime architecture in src and include trees.
- Module interaction design across core, node, transport, qos, and utility layers.
- Build and runtime integration points that impact architecture behavior.
- Architecture-relevant tests, benchmarks, and quality artifacts.

## 3. Architecture Principles

1. Clear boundaries: each module must have a single primary responsibility.
2. API-first contracts: public behavior is owned by include/kernal public headers and domain/assurance contracts.
3. Deterministic failure semantics: recoverable and non-recoverable paths must be explicit.
4. Resource boundedness: hot-path memory and queue growth must be bounded by configuration.
5. Observable operation: architecture-critical states must be measurable through logs and metrics.
6. Backward compatibility discipline: wire and API compatibility must follow documented policy.

## 4. Layered Architecture

The runtime source tree is organized into four top-level architecture folders:
- src/domain
- src/platform
- src/assurance
- src/kernal

The SMB architecture is implemented inside src/kernal and organized into the following layers.

1. Core layer
- Files: include/kernal/core, src/kernal/core
- Responsibilities: bus lifecycle, routing, service management, message framing, compatibility checks.

2. Node layer
- Files: include/kernal/node, src/kernal/node
- Responsibilities: role-specific behavior for client, server, publisher, subscriber.
- Rule: node modules should orchestrate role behavior and avoid embedding unrelated subsystem logic.

3. QoS layer
- Files: include/kernal/qos, src/kernal/qos
- Responsibilities: policy representation, compatibility evaluation, reliability state tracking.

4. Transport layer
- Files: include/kernal/transport, src/kernal/transport
- Responsibilities: protocol send/receive, transport-specific connectivity and buffering.

5. Utility and platform layer
- Files: include/kernal/utility, src/kernal/utility, shared utility modules under src/platform
- Responsibilities: configuration, common helpers, platform abstractions.

Dependency direction must remain top-down:
- node -> core/qos/transport/utility
- core -> qos/transport/utility
- transport -> utility/platform

Reverse dependencies between lower and higher layers are not allowed.

## 5. Component Responsibilities

1. Service discovery and service manager
- Detect service lifecycle transitions.
- Publish state changes through observer callbacks.
- Keep a deterministic online/offline model for dependent modules.

2. Routing engine
- Route validated messages to target consumers.
- Ensure async context lifetime safety for routed metadata.
- Preserve request correlation identifiers.

3. Subscriber architecture
- Maintain API-level subscribe and unsubscribe behavior.
- Delegate per-subscription lifecycle to session entities.
- Delegate multi-session orchestration and reconciliation to session manager.

4. Session manager model
- Manage multiple sessions with explicit states.
- Process service-change and suback events.
- Reconcile intent to active state with bounded retries and backoff.

## 6. State and Lifecycle Design

Architecture that models asynchronous negotiation or recovery must define explicit states.

Current subscription session baseline includes:
- INIT
- PENDING_SERVICE
- WAITING_SUBACK
- ACTIVE
- DEGRADED
- FAILED
- CANCELLED

Design constraints:
1. State transitions must be event-driven and deterministic.
2. Retry scheduling must be bounded by policy values.
3. Service offline events must degrade pending runtime state safely.
4. Recovery transitions must be observable in logs.

## 7. API and Header Rules

1. Public kernel architecture interfaces must live in include/kernal.
2. Internal-only helpers must remain local to src modules.
3. Public headers must not leak private storage layouts unless intentionally part of contract.
4. New reusable architecture building blocks should be promoted to include/kernal when cross-node reuse is expected.

## 8. Error Semantics and Recovery

1. API boundaries must validate inputs and return module-standard error codes.
2. Recoverable unavailability must be represented as recoverable errors, not parameter errors.
3. Startup validation failures for deterministic profiles must fail fast with actionable diagnostics.
4. Retry loops must not spin without delay.

## 9. Concurrency and Synchronization

1. Shared mutable state must use explicit lock discipline.
2. Reconcile loops and callbacks must avoid lock inversion.
3. Long-running actions should execute outside critical sections when possible.
4. Asynchronous contexts must own required memory until completion.

## 10. Memory and Resource Constraints

1. Architecture-critical paths must use approved allocators.
2. Payload construction must respect configured transport budget.
3. Deterministic memory profile must validate startup invariants.
4. Metrics must expose memory pressure indicators and configured watermarks.

## 11. Observability and Quality Evidence

Required architecture evidence:
- Integration tests for API behavior and lifecycle transitions.
- Reliability artifacts from soak and until-fail workflows.
- Compatibility matrix artifact for protocol governance.
- Traceability links from requirements to tests and artifacts.

At minimum, architecture changes should update one or more of:
- docs/readiness/ARCHITECTURE_REQUIREMENTS_ASSESSMENT.md
- docs/assurance/TRACEABILITY_GUIDE.md
- docs/release/process/RELEASE_QUALITY_SCORECARD.md

## 12. Requirement Mapping

Architecture design decisions should map to requirement IDs in docs/requirements/REQUIREMENTS_CATALOG.md.

Current primary mappings:
- REQ-REL-001, REQ-REL-002, REQ-REL-003, REQ-REL-004: startup, recovery, backpressure, health states.
- REQ-PERF-001, REQ-PERF-002, REQ-PERF-003: benchmark evidence and regression governance.
- REQ-SAFE-001, REQ-SAFE-002, REQ-SAFE-003: deterministic and bounded-memory behavior with traceability.
- REQ-SEC-001, REQ-SEC-002, REQ-SEC-003, REQ-SEC-004: threat model and continuous assurance workflow.
- REQ-COMP-001, REQ-COMP-002, REQ-COMP-003: protocol versioning, compatibility policy, release declarations.

## 13. Design Review Checklist

Every architecture-impacting change should review:
1. Boundary correctness and layering compliance.
2. State model completeness and failure mode coverage.
3. Concurrency safety and lock ordering.
4. Memory ownership and boundedness.
5. Backward compatibility impact and release declaration needs.
6. Test and artifact evidence updates.

## 14. Revision History

- 2026-04-11: Initial architecture design specification baseline.
- 2026-04-25: Updated to four-level assurance/domain/kernal/platform layout and header ownership rules.
