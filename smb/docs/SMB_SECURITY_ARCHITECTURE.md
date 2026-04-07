# ZOO SMB Security Architecture

## 1. Purpose

This document describes how security responsibilities are allocated across the current ZOO SMB architecture and what additional controls are required for later assurance grades.

## 2. Architectural security principles

- Treat all external transport input as untrusted until validated.
- Enforce policy at explicit boundaries instead of relying on caller discipline.
- Prefer rejection with telemetry over silent acceptance of unsafe input.
- Separate transport mechanics from identity, policy, and runtime-governance decisions.
- Keep security controls observable so they can become release evidence.

## 3. Security control allocation by layer

### 3.1 Node API layer

Responsibilities:

- validate API handles and basic caller input
- avoid exposing privileged internal state transitions directly
- attach policy-relevant metadata to outbound operations

Required evolution:

- role-based usage guidance
- stronger input validation for safety-critical command paths

### 3.2 Core bus and runtime layer

Responsibilities:

- protect lifecycle and HA state transitions
- contain faults and drive degraded-state behavior
- surface health and error signals for operational response

Required evolution:

- authenticated control-plane messages
- explicit state-transition guards for failover and peer election

### 3.3 Routing and policy layer

Responsibilities:

- enforce sender identity and encryption policy hooks
- reject traffic that violates overload or policy constraints
- maintain auditable rejection reasons and counters

Required evolution:

- policy profiles by criticality level
- replay and freshness validation rules
- stronger security-policy decision logging

### 3.4 Transport and protocol layer

Responsibilities:

- validate protocol headers and message framing
- isolate parser failures from service logic
- provide per-transport health and rejection telemetry

Required evolution:

- unified bus-wide wire-version contract
- fuzz-tested parser boundaries for every transport
- authenticated and encrypted transport profiles where required

### 3.5 Platform and utility layer

Responsibilities:

- provide stable primitives for memory, queues, timers, sockets, and threading
- support bounded-resource behavior where configured

Required evolution:

- explicit bounded-memory operating mode
- support for cryptographic integration and secure configuration storage

## 4. Current security-aligned capabilities

- flow control and backpressure hooks
- ingress rejection reasons and telemetry counters
- runtime degraded state and health-related signaling
- protocol magic and version fields in message headers
- config hooks for enforcing encrypted messages and sender identity
- repository-level static analysis and dependency inventory automation

## 5. Current security architecture gaps

- no product-level threat model existed before this document
- no unified identity and authentication architecture across transports
- no replay-protection architecture
- no cryptographic key-management architecture
- no secure update or signed-artifact architecture for deployed SMB components
- no mixed-criticality or freedom-from-interference model for higher-grade targets

## 6. Security decisions for next iteration

### SD-1 Untrusted-input default

Decision:
- Treat all inbound transport data and all public API payloads as untrusted.

Reason:
- Prevents accidental trust leakage from deployment assumptions.

### SD-2 Policy enforcement at routing boundary

Decision:
- Keep identity, encryption, freshness, and overload rejection decisions in the policy-capable path between protocol decode and service delivery.

Reason:
- Centralizes enforcement and keeps transport adapters simpler.

### SD-3 Security telemetry is mandatory

Decision:
- Every major rejection path should map to observable counters or logs.

Reason:
- Needed for operations, incident response, and audit evidence.

### SD-4 Compatibility and security must be linked

Decision:
- Protocol version governance and security policy governance must evolve together.

Reason:
- Wire compatibility without security compatibility can preserve unsafe behavior.

## 7. Roadmap-aligned control additions

### Industrial-grade additions

- parser fuzz targets and malformed-input regression suite
- policy-tested overload rejection
- documented identity and encryption policy behavior
- security findings remediation tracking in release process

### Automotive-grade additions

- authenticated node identity by default
- bounded-latency safety-channel policy profiles
- freedom-from-interference security controls between traffic classes
- update and artifact trust chain

### Military and aerospace-grade additions

- high-assurance partitioning and isolation model
- multi-level security policy framework
- anti-tamper and key-lifecycle controls
- configuration baseline freeze and reproducible secure build chain

## 8. Verification expectations

- unit tests for policy rejection logic
- integration tests for overload and malformed-input rejection
- fuzzing for parser and message-boundary surfaces
- static analysis and dependency review in release gates
- security review sign-off for major protocol or runtime changes

## 9. Traceability

- REQ-SEC-001
- REQ-SEC-002
- REQ-SEC-003
- REQ-SEC-004