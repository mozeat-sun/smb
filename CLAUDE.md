# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Configure and build (Release by default)
cmake -S . -B build
cmake --build build -j"$(nproc)"

# Debug build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j"$(nproc)"

# Build with examples
cmake -S . -B build -DZOO_BUILD_EXAMPLES=ON
cmake --build build -j"$(nproc)"

# Build with Ninja (faster)
cmake -S . -B build -G Ninja
cmake --build build -j"$(nproc)"

# Cross-compile (see docker/README.md for Docker-based cross environments)
cmake -S . -B build-arm64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake
cmake -S . -B build-mingw -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake
```

## Testing

```bash
# Run all tests (requires Unity — auto-disabled if not found)
ctest --test-dir build --output-on-failure

# Run a single test by name
ctest --test-dir build -R test_zoo_smb_integration --output-on-failure

# Run tests by label
ctest --test-dir build -L "smb" --output-on-failure
ctest --test-dir build -L "unit" --output-on-failure
ctest --test-dir build -L "integration" --output-on-failure

# Run extended SMB integration tests (transport backends, discovery, backpressure)
cmake -S . -B build -DZOO_ENABLE_EXTENDED_INTEGRATION_TESTS=ON
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure

# Run a single test binary directly from build directory
./build/tests/unit/smb/test_zoo_smb_subscription_session_manager_unit
./build/tests/integration/test_zoo_smb_integration
```

Test labels used: `unit`, `integration`, `smb`, `node`, `assurance`, `domain`, `runtime`.

Tests use the **Unity** test framework (vendored at `thirdparty/unity/`). The integration tests vendor Unity directly; unit tests use `find_package(Unity)`.

## Static Analysis / Linting

```bash
# cppcheck (CI runs this — requires cppcheck installed)
bash tools/quality/static_analysis.sh

# Dependency vulnerability scan (requires trivy)
bash tools/quality/dependency_scan.sh
```

There is no clang-format or clang-tidy configuration in the repo. Follow existing code style: C11, `snake_case` function names, `ZOO_CAPITAL_CASE` macros/types, Doxygen `/** */` doc comments, file header blocks with version history.

## Architecture

### Top-Level Module Layout

The codebase has four top-level architecture directories under `src/`:

| Directory | Purpose |
|-----------|---------|
| `src/platform/` | Cross-platform foundation: error codes, types, buffer, log, memory_pool, socket, thread_pool, dispatcher, timer, util |
| `src/domain/` | Domain profile selection — encodes policies that vary by deployment context |
| `src/assurance/` | Runtime assurance mesh policies (validated snapshots, identity checks) |
| `src/kernal/` | **SMB (Soft Message Bus)** — the primary product: a pub/sub message bus |

Parallel structure exists in `include/` with matching subdirectories.

### SMB Internal Layers (src/kernal/)

The SMB kernel follows a strict layered architecture with top-down dependencies only:

1. **Core** (`core/`) — Bus lifecycle, routing engine, service discovery/manager, message framing, reactor, transport manager, rule manager, metrics
2. **Node** (`node/`) — Role-specific behavior: client, server, publisher, subscriber, subscription sessions, session manager
3. **QoS** (`qos/`) — Policy representation, compatibility evaluation, reliability state tracking
4. **Transport** (`transport/`) — Protocol send/receive, transport-specific backends: TCP, UDP (unicast + broadcast), SHM (shared memory)
5. **Utility** (`utility/`) — Config, error codes, ring buffer

Dependency direction: `node → core/qos/transport/utility`, `core → qos/transport/utility`, `transport → utility/platform`. Reverse dependencies are forbidden.

### Domain Profiles

The build accepts `-DZOO_DOMAIN_PROFILE=<profile>` with four allowed values:
- `generic` (default)
- `industrial`
- `automotive`
- `military`

The profile selects domain-specific policy implementations at compile time via `ZOO_DOMAIN_PROFILE_<profile>` preprocessor defines. The `src/domain/` module provides `zoo_domain_policy.c` and `zoo_domain_profile.c`; the assurance module (`src/assurance/`) provides runtime assurance mesh checks that validate policy snapshots and startup identity.

### Platform Modules (src/platform/)

Each platform sub-module builds both a static (`.a`) and shared (`.so`) library target, e.g., `zoo_buffer_static` / `zoo_buffer_shared`. These are linked by the SMB module.

### Build Output Pruning

The root CMakeLists prunes non-primary libraries from `stage/lib/`. Only these survive:
- **Shared**: `libsmb.so`, `libzoo_platform.so*`
- **Static**: `libzoo_smb.a`, `libzoo_platform.a`

Other libraries (buffer, dispatcher, log, memory_pool, socket, thread_pool, timer, util) are redirected to `build/hidden_shared_libs/` and `build/hidden_static_libs/`. Test binaries need RPATH/LD_LIBRARY_PATH pointing at both `stage/lib` and `build/hidden_shared_libs` — the CMake test targets handle this automatically.

When `ZOO_BUILD_TESTS=ON`, pruning is automatically disabled.

### Key Dependencies

- **CMake** 3.16+
- **C11** / **C++14** (C++ only enabled at project level; all source is C)
- **POSIX** (`_POSIX_C_SOURCE=200809L`, `_GNU_SOURCE`)
- **Threads**, **atomic** (link-time)
- **Unity** test framework (auto-detected; tests disabled if not found)

No external library dependencies beyond libc, pthreads, and atomic.

### Coding Conventions

- C11 with `snake_case` functions, `PascalCase` types via `typedef struct`, `UPPER_SNAKE_CASE` macros
- Opaque handle pattern: public headers expose `typedef struct ZOO_FOO_STRUCT* ZOO_FOO_HANDLE`; struct definition is in the `.c` file
- `IN` / `OUT` parameter direction annotations
- `ZOO_ERROR_TYPE` return for fallible operations, `void` for infallible
- `ZOO_BOOL` / `ZOO_TRUE` / `ZOO_FALSE` for booleans
- `(void)` casts for unused parameters
- File header blocks with copyright, module, component ID, description, and change history
- Doxygen `@brief` / `@param` / `@return` on all public functions
- `-Wall -Wextra -Wpedantic` always; `-Werror` in Release builds only

## Quality Tools

All under `tools/quality/`. Key ones:

| Script | Purpose |
|--------|---------|
| `benchmark.sh` | Run performance benchmarks, compare against baseline |
| `soak_test.sh` | Long-duration soak testing |
| `reliability_until_fail.sh` | Repeated reliability gate |
| `recovery_rto.sh` | Recovery RTO (recovery time objective) evidence |
| `fault_injection.sh` | Fault injection suite |
| `fuzz_protocol.sh` | Protocol fuzzing gate |
| `static_analysis.sh` | cppcheck static analysis |
| `traceability_gate.sh` | Enforce requirements-to-test traceability |
| `protocol_compatibility_matrix.sh` | Wire protocol compatibility matrix |
| `grade1_exit_gate.sh` | Generate Grade 1 release exit status |
| `dependency_scan.sh` | Trivy-based dependency vulnerability scan |
| `vulnerability_sla_report.sh` | Vulnerability remediation SLA tracking |
| `pre_assessment_readiness_gate.sh` | Validate pre-assessment findings log |

CI (`.github/workflows/`) has three workflows:
- **ci.yml** — build + test on push/PR
- **quality-gates.yml** — all quality tools above, runs on PR, push to main, weekly schedule
- **cmake-matrix.yml** — multi-config build matrix

## Key Documentation

- [ARCHITECTURE_DESIGN_SPECIFICATION.md](docs/architecture/ARCHITECTURE_DESIGN_SPECIFICATION.md) — architecture rules, layers, state models, constraints
- [REQUIREMENTS_CATALOG.md](docs/requirements/REQUIREMENTS_CATALOG.md) — requirement IDs mapped to architecture
- [PROTOCOL_COMPATIBILITY_POLICY.md](docs/requirements/PROTOCOL_COMPATIBILITY_POLICY.md) — wire protocol versioning and deprecation
- [SAFETY_CASE_SKELETON.md](docs/assurance/SAFETY_CASE_SKELETON.md) — safety argument structure for higher-assurance reviews
- [GRADE_ROADMAP.md](docs/readiness/GRADE_ROADMAP.md) — release grade progression plan
- [RELEASE_QUALITY_SCORECARD.md](docs/release/process/RELEASE_QUALITY_SCORECARD.md) — release quality metrics
- [TRACEABILITY_GUIDE.md](docs/assurance/TRACEABILITY_GUIDE.md) — requirements-to-evidence traceability
