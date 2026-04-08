# ZOO Platform

ZOO is a modular C/C++ platform library for systems software.
It provides reusable runtime components for platform abstraction, buffering, dispatching,
logging, memory pools, timers, threading, sockets, and SMB (Soft Message Bus).

This repository is a monorepo: build, test, and package everything from the root.

## Repository Layout

- `src/`: implementation modules
- `include/`: public headers
- `tests/`: unit, integration, benchmark, and performance tests
- `examples/`: cross-module demos
- `docs/`: architecture, requirements, roadmap, and quality artifacts
- `docker/`: reproducible build environments
- `thirdparty/`: vendored dependencies

## Modules

Core modules in `src/`:

- `platform`
- `util`
- `log`
- `memory_pool`
- `buffer`
- `socket`
- `thread_pool`
- `dispatcher`
- `timer`
- `smb`

## Quick Start

### Configure and Build

```bash
cmake -S . -B build
cmake --build build -j"$(nproc)"
```

### Run Tests

```bash
ctest --test-dir build --output-on-failure
```

### Build With Examples Enabled

```bash
cmake -S . -B build -DZOO_BUILD_EXAMPLES=ON
cmake --build build -j"$(nproc)"
```

## Build Outputs

The root build is configured to keep only primary libraries in `stage/lib`.

Shared libraries retained:

- `libsmb.so`
- `libzoo_platform.so*`

Static libraries retained:

- `libzoo_smb.a`
- `libzoo_platform.a`

Other module libraries may be built internally but are pruned from `stage/lib` by root CMake cleanup targets.

## Examples

Examples are under `examples/`:

- `examples/client_server/`
- `examples/pub_sub/`

See `examples/README.md` for usage details.

## Benchmarks and Performance

Benchmark/performance sources are in:

- `tests/benchmark/`
- `tests/performance/`

If you run local benchmark experiments, store reports under `reporters/` (repository-local convention).

## Documentation Index

Primary project documents:

- `docs/ARCHITECTURE_REQUIREMENTS_ASSESSMENT.md`
- `docs/REQUIREMENTS_CATALOG.md`
- `docs/GRADE_ROADMAP.md`
- `docs/RELEASE_QUALITY_SCORECARD.md`
- `docs/TRACEABILITY_GUIDE.md`

Additional entry points:

- `src/README.md`
- `include/README.md`
- `tests/README.md`
- `examples/README.md`
- `docker/README.md`

## Development Notes

- Default build type is `Release` unless overridden.
- In Release builds, warnings are treated as errors (`-Werror`).
- Root CMake installs to `stage/` by default.

## Open Source Governance

- `LICENSE`
- `CONTRIBUTING.md`
- `CODE_OF_CONDUCT.md`
- `SECURITY.md`
- `CHANGELOG.md`
