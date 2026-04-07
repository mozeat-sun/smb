# ZOO SMB Architecture Document

## 1. Purpose

This document defines the high-level architecture of the ZOO SMB module, including layers, boundaries, and principal runtime flows.

## 2. Architectural Drivers

- Consistent node-level API for service and pub/sub communication
- Pluggable multi-transport backend support
- Predictable behavior under concurrency and overload
- Runtime evolvability toward high availability
- Observable transport and routing operations

## 3. System Context

Inputs:
- application API calls from SMB clients/servers/publishers/subscribers
- inbound bytes/events from transport backends
- runtime policy and configuration

Outputs:
- outbound messages across selected transports
- local callback deliveries to registered handlers
- runtime and transport metrics for observability

## 4. Layered Architecture

1. Application and Node API layer
- server, client, publisher, subscriber interfaces

2. Core bus and runtime layer
- message dispatcher
- service manager and service discovery
- runtime lifecycle and HA role management

3. Routing and policy layer
- routing engine and rule manager
- qos and safety policy enforcement

4. Transport and protocol layer
- transport manager
- transport backends (TCP, UDP, UDP broadcast, SHM)
- protocol encode/decode
- reactor/event handling

5. Platform and utility layer
- thread/mutex primitives
- ring buffer and queue infrastructure
- timers, memory, and shared utility types

## 5. Component Interaction View

Inbound flow:
1. Transport backend receives data.
2. Reactor surfaces readable event.
3. Protocol decodes message bytes.
4. Dispatcher submits to routing engine.
5. Routing decides local delivery and/or forwarding.
6. Policy checks and flow control gates are applied.

Outbound flow:
1. Node API produces a message operation.
2. Dispatcher packages metadata for routing.
3. Routing selects destination service/transport.
4. Protocol serializes if required.
5. Transport manager sends via selected backend.
6. Metrics and health counters are updated.

## 6. Key Architectural Decisions

### AD-1 Layered separation
Decision:
- Keep node API, core dispatch, routing/policy, and transport concerns in distinct layers.

Rationale:
- Enables independent evolution and focused testing.

### AD-2 Manager-based concurrency boundaries
Decision:
- Centralize mutable shared state in manager modules (runtime, rule, transport, service).

Rationale:
- Reduces race risk and simplifies synchronization strategy.

### AD-3 Configurable flow control
Decision:
- Use high/low watermarks and ingress controls from config.

Rationale:
- Provides predictable overload behavior without hard-coded limits.

### AD-4 Runtime HA scaffolding
Decision:
- Expose role/state/peer APIs before full HA control plane is complete.

Rationale:
- Supports incremental implementation and integration safety.

## 7. Deployment and Build View

- Build system: CMake with C11 compiler requirements.
- Primary library and test targets are produced from src and tests trees.
- Platform dependency headers are required during configuration.
- Examples are optional and may depend on additional modules.

## 8. Quality Attributes and Tactics

Reliability:
- explicit error-code model
- guarded lifecycle transitions
- runtime/transport cleanup APIs

Performance:
- transport reuse options
- flow-control watermarks
- per-channel telemetry for hotspot detection

Scalability:
- multiple transport options and routing indirection
- configurable thread and queue parameters

Observability:
- transport manager metrics APIs
- test harness scripts for regression and load checks

## 9. Risks and Mitigations

Risk:
- Broad API surface may lead to inconsistent integration usage.
Mitigation:
- Provide role-specific integration examples and API usage guidance.

Risk:
- Mixed transport behavior can complicate operational debugging.
Mitigation:
- Require metrics collection and structured test runs in release gates.

Risk:
- HA behavior divergence across deployments.
Mitigation:
- Validate runtime state transitions and peer-status flows in integration tests.

## 10. Traceability and Related Docs

- Requirements: SMB_REQUIREMENTS.md
- Design: SMB_DESIGN.md
- Existing combined architecture/design background: SMB_ARCHITECTURE_DESIGN.md
