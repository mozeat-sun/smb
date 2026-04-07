# ZOO SMB Design Document

## 1. Purpose

This document describes how ZOO SMB implements the required capabilities defined in SMB_REQUIREMENTS.md.
It focuses on component responsibilities, internal interactions, and major design decisions.

## 2. Design Principles

- Keep node APIs simple and role-oriented for application teams.
- Separate message semantics from transport mechanics.
- Isolate concurrency-sensitive internals behind manager components.
- Use policy-driven controls (QoS, flow control, security) instead of hard-coded behavior.
- Preserve forward compatibility for HA and runtime governance.

## 3. Component Design

### 3.1 Node layer
Responsibilities:
- Expose role-specific creation/destruction and messaging APIs.
- Bind user callbacks for server request handling and subscriber notifications.

Key types:
- server handle
- client handle
- publisher handle
- subscriber handle

### 3.2 Core bus and dispatch
Responsibilities:
- Normalize ingress messages.
- Resolve local observer dispatch versus forwarding path.
- Coordinate with routing and transport manager.

Design notes:
- Dispatcher is the handoff boundary between protocol/transport input and message semantics.
- Observer registration and lookup are encapsulated in dedicated observer and service structures.

### 3.3 Routing and rule management
Responsibilities:
- Evaluate routing rules based on message metadata.
- Support rule insertion, matching, and cleanup.
- Maintain consistent behavior under concurrent access.

Design notes:
- Rule management is separated from message dispatch to reduce coupling and simplify testability.
- Routing telemetry counters support overload and policy diagnostics.

### 3.4 Transport manager
Responsibilities:
- Create/reuse transport objects for target services.
- Start/stop transport endpoints.
- Send unicast and broadcast messages.
- Enforce send-path watermarks and surface transport health metrics.

Design notes:
- Transport manager is the primary backpressure control boundary.
- Metrics are exposed both globally and per-transport channel.

### 3.5 Runtime and HA scaffolding
Responsibilities:
- Own SMB runtime state machine.
- Expose runtime role and epoch views.
- Process peer heartbeat/timeout events.
- Notify state observers of transitions.

Design notes:
- Runtime HA support is scaffolded to enable staged evolution without API breakage.
- Election and peer-status APIs are intentionally explicit for controllability.

### 3.6 Protocol and message model
Responsibilities:
- Validate and transform wire bytes to internal message objects.
- Serialize internal messages for outbound transmission.

Design notes:
- Message header includes magic and version to protect decode paths.
- Message type enum cleanly separates request/reply and pub/sub flows.

### 3.7 QoS and policy controls
Responsibilities:
- Carry operational policies such as timeout and retry semantics.
- Integrate with backpressure and ingress controls.

Design notes:
- Policy objects are passed into node creation to allow role-specific tuning.

## 4. Data and State Design

### 4.1 Runtime states
- created
- starting
- running
- degraded
- stopping
- stopped
- faulted

### 4.2 Runtime roles
- standalone
- primary
- secondary

### 4.3 Transport metrics model
Global and per-channel metrics include:
- total send success
- total send failures
- backpressure drops
- circuit open rejections
- in-flight sends
- high/low watermarks
- backpressure active flag
- circuit open flag (per channel)

## 5. Threading and Concurrency

- Manager components guard mutable shared state.
- Observer/rule containers are designed for concurrent registration and dispatch safety.
- Watermark-based admission control limits in-flight send amplification under contention.

## 6. Error Handling Strategy

- APIs return ZOO_ERROR_TYPE values.
- Success and failure checks use SMB error helper macros.
- Errors are categorized (general, memory, thread, network, protocol, transport, routing, security, and others).

## 7. Build and Integration Design

- Core build uses CMake and C11.
- Source is split into core, node, transport, qos, and utility subdirectories.
- Tests are organized into unit, integration, and performance groups.
- A shell test orchestrator script provides suite selection and execution modes.

## 8. Trade-offs

- Rich API surface improves extensibility but increases onboarding complexity.
- Multiple transports increase deployment flexibility but require stronger observability.
- HA is scaffolded first to reduce delivery risk before introducing full control-plane persistence.

## 9. Validation Strategy

- Unit tests validate local component behavior and guard conditions.
- Integration tests validate end-to-end role interactions and routing behavior.
- Performance and optional analysis suites validate scalability and memory/runtime behavior.

## 10. Cross References

Requirements baseline: SMB_REQUIREMENTS.md
System decomposition and flows: SMB_ARCHITECTURE.md
