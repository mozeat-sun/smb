# ZOO SMB Requirements

## 1. Purpose

This document defines the baseline requirements for the ZOO Soft Message Bus (SMB) module.
It is intended for development, test, and release validation.

## 2. Scope

In scope:
- Node-facing messaging APIs (server, client, publisher, subscriber)
- Core message bus behavior (routing, dispatch, service lifecycle)
- Transport abstractions (TCP, UDP, UDP broadcast, shared memory)
- Runtime lifecycle and HA scaffolding
- Observability hooks and quality gates required for release

Out of scope:
- External deployment orchestration
- Persistent distributed consensus implementation details
- Application-specific business message schemas

## 3. Stakeholders

- SMB library maintainers
- Integration teams using SMB node APIs
- QA and performance validation teams
- Platform/runtime maintainers

## 4. Functional Requirements

### FR-1 Node API lifecycle
- The module shall provide APIs to create and destroy node roles:
  - server
  - client
  - publisher
  - subscriber
- The module shall expose role-specific operations (request/reply, publish/subscribe).

### FR-2 Message model
- The module shall represent messages with a fixed header and optional payload.
- The message header shall include at least:
  - magic/version fields for validation
  - message type
  - request ID
  - topic and sender identity fields
- The module shall support request/reply and pub/sub message types.

### FR-3 Routing and dispatch
- The module shall route incoming messages to local handlers or outbound transports based on routing rules.
- The module shall support dynamic registration/unregistration of message observers.

### FR-4 Transport abstraction
- The module shall abstract transport details behind a transport manager.
- The module shall support at least TCP, UDP, UDP broadcast, and shared memory transport types.
- The module shall support both unicast send and broadcast send operations.

### FR-5 Service discovery and service management
- The module shall expose service management and service discovery interfaces.
- Service discovery shall be startable/stoppable during runtime.

### FR-6 Runtime lifecycle
- The module shall provide runtime create/start/stop/destroy APIs.
- The runtime shall track lifecycle states including created, starting, running, degraded, stopping, stopped, and faulted.

### FR-7 HA scaffolding
- The runtime shall expose role information (standalone, primary, secondary).
- The runtime shall provide APIs for heartbeat reporting, timeout reporting, and election trigger.

### FR-8 Flow control and protection
- The module shall support configurable high/low watermark controls for send path backpressure.
- The module shall support ingress protection controls and rejection on overload.

### FR-9 Security policy hooks
- The module shall support configuration toggles for:
  - requiring sender identity
  - enforcing encrypted messages
  - timestamp/timeout-based rejection controls

### FR-10 Error model
- The module shall return structured error codes compatible with the parent ZOO error system.
- The module shall distinguish success and error conditions using documented macros and categories.

### FR-11 Metrics and telemetry
- The module shall expose transport metrics including:
  - send success/failure counts
  - backpressure drops
  - circuit open rejections
  - in-flight send counts
- The module shall support resetting and querying metrics at runtime.

## 5. Non-Functional Requirements

### NFR-1 Language and build
- Implementation language shall be C11.
- The module shall build with CMake 3.14 or later.

### NFR-2 Portability
- Public APIs shall be C/C++ compatible via extern guards.
- Platform dependencies shall be isolated through shared platform interfaces.

### NFR-3 Reliability
- API calls shall validate input handles and pointers.
- Runtime and transport manager cleanup paths shall be idempotent where possible.

### NFR-4 Concurrency
- Shared registries and metrics paths shall be thread-safe.
- Backpressure and watermarks shall prevent uncontrolled in-flight growth.

### NFR-5 Observability
- Key error/rejection paths shall be traceable through counters and logs.
- Integration and system tests shall provide repeatable pass/fail reports.

### NFR-6 Testability
- Unit and integration tests shall be runnable through provided scripts.
- CTest integration shall be available for automated environments.

## 6. External Interfaces

### 6.1 Public header groups
- core interfaces: service/routing/runtime/dispatcher/transport manager
- node interfaces: server/client/publisher/subscriber
- transport interfaces: protocol and transport backends
- qos interfaces: policy and context
- utility interfaces: config, error, timer, ring buffer, types

### 6.2 Configuration interface
- A default SMB configuration shall be obtainable from config APIs.
- Configuration shall include machine, broadcast, multicast, bridge, log, and system sections.

## 7. Constraints and Assumptions

- The platform module is required at build time.
- Threading support is required (pthread/Threads dependency).
- Examples may require additional components and are not mandatory for core build.

## 8. Verification and Acceptance Criteria

The SMB requirements are accepted when all criteria are met:
- Build passes in a clean environment with C11 and CMake 3.14+
- Public headers compile cleanly from an external consumer test
- Unit and integration test suites pass through project scripts
- Runtime lifecycle transitions are validated by tests
- Transport metrics queries return valid values under load tests
- Security/flow-control rejection paths are exercised and validated

## 9. Traceability

Design realization: see SMB_DESIGN.md
Architecture allocation: see SMB_ARCHITECTURE.md
