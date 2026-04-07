# ZOO SMB Threat Model

## 1. Purpose

This document defines the threat model for ZOO SMB and provides a security-analysis baseline for future industrial, automotive, and military/aerospace assurance work.

Scope:

- node API entry points
- core bus and runtime control plane
- routing and policy enforcement
- transport backends and protocol parsing
- build and release surfaces that affect deployed artifacts

Out of scope for this revision:

- hardware root of trust implementation details
- operating-system hardening outside SMB-owned components
- enterprise perimeter controls external to product deployment

## 2. Security objectives

- Preserve integrity of message routing and control flow.
- Prevent unauthorized message injection or service impersonation.
- Reduce denial-of-service risk from overload, malformed input, or transport abuse.
- Ensure protocol parsing failures are contained and observable.
- Maintain evidence suitable for progressive assurance reviews.

## 3. Assets

- service identity and endpoint metadata
- message payloads and metadata
- routing rules and dispatch decisions
- runtime role and HA peer state
- protocol headers and transport session state
- build artifacts, release packages, and dependency inventory

## 4. Trust boundaries

1. Application to node API boundary
- Untrusted or partially trusted application input enters SMB APIs.

2. Transport to protocol boundary
- Untrusted bytes from TCP, UDP, SHM, or broadcast transports enter decode paths.

3. Protocol to routing boundary
- Parsed messages influence routing, callback delivery, and forwarding behavior.

4. Runtime control boundary
- Peer health, role, timeout, and HA-related events can alter runtime state.

5. Build and release boundary
- Dependencies, generated artifacts, and build configuration affect deployed behavior.

## 5. Threat actors

- Malicious network peer on reachable transport path
- Compromised local process using public SMB APIs
- Misconfigured but non-malicious integrator deployment
- Insider or supply-chain attacker tampering with build inputs or dependencies
- Faulty peer producing malformed or replayed protocol traffic

## 6. Threat categories and scenarios

### T1. Unauthorized message injection

Scenario:
- An attacker sends forged traffic to a transport endpoint and attempts to impersonate a valid peer or service.

Potential impact:
- False request handling, unsafe state transitions, incorrect service discovery, or control-plane corruption.

Current controls:
- sender identity enforcement hook in configuration
- protocol validation and structured error model

Required follow-up:
- authenticated peer identity model
- transport-independent identity verification rules

### T2. Protocol parsing abuse

Scenario:
- Malformed headers, invalid lengths, corrupted version fields, or crafted payloads target parser weaknesses.

Potential impact:
- Crash, memory corruption, incorrect routing, or resource exhaustion.

Current controls:
- protocol header fields include magic and version
- error categorization for protocol failures
- fuzz-gate scaffolding at repository level

Required follow-up:
- dedicated fuzz targets for parser surfaces
- negative test coverage per transport and message type

### T3. Denial of service through overload

Scenario:
- Excessive ingress, send amplification, slow peer behavior, or queue saturation drives unbounded in-flight work.

Potential impact:
- Latency collapse, service starvation, dropped critical traffic, degraded recovery behavior.

Current controls:
- ingress and send-path high/low watermark controls
- backpressure counters and rejection paths

Required follow-up:
- policy profiles by message criticality
- benchmarked overload envelopes and release gates

### T4. Replay and stale-message acceptance

Scenario:
- Captured or delayed traffic is replayed to cause repeated state transitions or stale decisions.

Potential impact:
- Duplicate command execution, stale peer acceptance, invalid control-plane behavior.

Current controls:
- timeout-related policy hooks exist

Required follow-up:
- replay protection strategy
- timestamp freshness and request identity rules

### T5. Runtime role or peer-state manipulation

Scenario:
- False heartbeat, timeout, or peer-status inputs influence HA role decisions or health reporting.

Potential impact:
- unintended degraded state, failover instability, or split-brain-like behavior.

Current controls:
- explicit runtime role/state model and peer health tracking

Required follow-up:
- authenticated peer control messages
- election safety rules and audited state-transition invariants

### T6. Confidentiality exposure

Scenario:
- Sensitive message payloads traverse transports without required encryption or access control.

Potential impact:
- information disclosure, policy breach, mission or safety impact.

Current controls:
- configuration hook for enforcing encrypted messages

Required follow-up:
- transport-specific encryption model
- key lifecycle and trust-anchor design

### T7. Supply-chain and build tampering

Scenario:
- A dependency, generated file, or build configuration is altered before release.

Potential impact:
- hidden malicious logic, unreviewed behavior changes, unverifiable release provenance.

Current controls:
- dependency inventory generation and release quality process

Required follow-up:
- signed release artifacts
- stronger provenance and dependency verification policy

## 7. Risk priorities for next iteration

Highest near-term priorities:

1. parser robustness and malformed-input containment
2. overload and backpressure abuse resilience
3. identity and impersonation prevention
4. runtime health and HA control integrity

## 8. Required security evidence

- static-analysis reports
- dependency inventory and vulnerability review
- fuzz results for parser surfaces
- negative tests for identity, overload, and malformed traffic rejection
- documented remediation log for security findings

## 9. Security assumptions

- Platform thread and memory primitives behave correctly.
- Operating system or deployment network controls are not sufficient on their own.
- Untrusted inputs must be assumed at every external transport boundary.

## 10. Traceability

- REQ-SEC-001
- REQ-SEC-002
- REQ-SEC-003
- REQ-SEC-004