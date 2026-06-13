# Changelog

All notable changes to this project will be documented in this file.

The format is inspired by Keep a Changelog, and this project follows
Semantic Versioning where practical.

## [Unreleased]

### Added
- SMB (Soft Message Bus) pub/sub message bus kernel with layered architecture
- Transport backends: TCP (client/server), UDP (unicast/broadcast), SHM (shared memory)
- QoS policy engine: reliability, durability, priority, deadline, liveliness
- Subscription session manager with lifecycle reconciliation
- Routing engine with rule manager, service discovery, and transport manager
- Runtime assurance mesh with domain profile selection (generic, industrial, automotive, military)
- Protocol serialization/deserialization with header validation
- Ring buffer with thread-safe read/write and capacity inquiry
- Port manager for dynamic port allocation
- Message framing with CRC32 integrity check
- Cross-compilation toolchains (aarch64, mingw-w64)
- Docker-based reproducible build environments
- Comprehensive test suite: 34 tests (unit, integration, fuzz, benchmark, performance)
- Quality toolchain: benchmark, soak, fuzz, fault injection, static analysis, traceability
- CI/CD: GitHub Actions workflows for build, test, and quality gates
- Open-source governance: LICENSE (MIT), CONTRIBUTING, CODE_OF_CONDUCT, SECURITY

### Changed
- Migrated orphaned unit tests to registered CTest targets (14 test executables, 81+ cases)
- Updated QoS tests from struct-based to handle-based API
- Consolidated test helpers into `test_smb_helpers.h`
- Suppressed SMB auto-init in unit tests via `test_smb_noautoinit.c` weak override
- Fixed dispatcher `zoo_stop_dispatcher` to properly signal blocking queue dequeue
- Updated README with accurate benchmark and quality tool documentation

### Fixed
- Signed/unsigned error code comparison in rule manager unit tests
- Missing memory pool initialization in subscription session manager unit tests
- Missing memory pool initialization in runtime assurance unit tests
- Dispatcher thread hang on stop due to wrong condition variable signal

## [INDUSTRIAL-RC1] — 2026-04-25

### Added
- Industrial domain profile policy enforcement
- Startup identity policy by domain profile
- Assurance policy snapshot validation by profile
- Domain policy contracts and assurance snapshot resolution
- Assurance mesh and domain profile foundation

## [M3-RC1] — 2026-04-11

### Added
- Milestone 3 release candidate
- Certification readiness package scaffolding
- Generic dispatcher integration with SMB
- SHM transport examples
- Grade 1 roadmap documentation

## [0.1.0] — 2026-04-09

### Added
- Initial platform library: buffer, dispatcher, log, memory pool, socket, thread pool, timer, util
- CMake build system with install targets and packaging
- Basic examples: client/server, pub/sub, transport SHM
