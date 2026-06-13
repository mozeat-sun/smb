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

To include extended SMB integration tests:

```bash
cmake -S . -B build -DZOO_ENABLE_EXTENDED_INTEGRATION_TESTS=ON
cmake --build build -j"$(nproc)"
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
- `examples/transport_shm/`

See `examples/README.md` for usage details.

## Benchmarks and Performance

Benchmark/performance sources are in:

- `tests/benchmark/`
- `tests/performance/`

Run benchmarks with:

```bash
bash tools/quality/benchmark.sh
```

Generated benchmark artifacts are written to `artifacts/quality/` and `reporters/`
(both directories are git-ignored; create them locally as needed).

Key quality scripts (all under `tools/quality/`):

| Script | Purpose |
|--------|---------|
| `benchmark.sh` | Run performance benchmarks, compare against baseline |
| `soak_test.sh` | Long-duration soak testing |
| `reliability_until_fail.sh` | Repeated reliability gate |
| `recovery_rto.sh` | Recovery RTO evidence generation |
| `fault_injection.sh` | Fault injection suite |
| `fuzz_protocol.sh` | Protocol fuzzing gate |
| `static_analysis.sh` | cppcheck static analysis |
| `traceability_gate.sh` | Enforce requirements-to-test traceability |
| `protocol_compatibility_matrix.sh` | Wire protocol compatibility matrix |
| `grade1_exit_gate.sh` | Generate Grade 1 release exit status |

## Compatibility And Deprecation

API releases follow semantic versioning.

Wire protocol compatibility rules, fail-fast mismatch behavior, and the deprecation lifecycle are defined in `docs/requirements/PROTOCOL_COMPATIBILITY_POLICY.md`.

The CI-generated compatibility matrix is published as the `protocol-compatibility-artifacts` workflow artifact.

Use `docs/templates/RELEASE_NOTES_TEMPLATE.md` for release declarations covering API compatibility status, wire compatibility status, breaking changes, and deprecation notices.

## Pre-Assessment Readiness

Use `tools/quality/pre_assessment_readiness_gate.sh` to validate `docs/readiness/PRE_ASSESSMENT_FINDINGS_LOG.csv` and generate readiness artifacts.

Use `tools/quality/pre_assessment_remediation_plan.sh` to generate remediation tracking artifacts.

Pre-assessment outputs are published by CI in the `pre-assessment-readiness-artifacts` bundle.

## Documentation Index

Primary project documents:

- `docs/architecture/ARCHITECTURE_DESIGN_SPECIFICATION.md`
- `docs/readiness/ARCHITECTURE_REQUIREMENTS_ASSESSMENT.md`
- `docs/assurance/SAFETY_CASE_SKELETON.md`
- `docs/requirements/PROTOCOL_COMPATIBILITY_POLICY.md`
- `docs/requirements/TOOLCHAIN_BASELINE_AND_CHANGE_CONTROL.md`
- `docs/requirements/REQUIREMENTS_CATALOG.md`
- `docs/readiness/GRADE_ROADMAP.md`
- `docs/release/process/RELEASE_QUALITY_SCORECARD.md`
- `docs/release/process/RELEASE_EVIDENCE_FREEZE_PROCEDURE.md`
- `docs/assurance/TRACEABILITY_GUIDE.md`
- `docs/assurance/GRADE1_EXIT_CHECKLIST.md`

Additional entry points:

- `src/README.md`
- `include/README.md`
- `tests/README.md`
- `examples/README.md`
- `docker/README.md`
- `docs/assurance/INDEPENDENT_SAFETY_REVIEW_CHECKLIST.md`
- `docs/readiness/PRE_ASSESSMENT_PLAN.md`
- `docs/readiness/PRE_ASSESSMENT_FINDINGS_LOG.csv`
- `docs/readiness/PRE_ASSESSMENT_READINESS_SUMMARY.md`
- `docs/release/records/GRADE1_LONG_SOAK_EVIDENCE.json`
- `docs/release/records/GRADE1_PRODUCTION_PILOT_EVIDENCE.md`
- `docs/templates/RELEASE_NOTES_TEMPLATE.md`

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
