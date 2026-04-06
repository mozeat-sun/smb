# ZOO Code Specifications

Date: 2026-04-05
Scope: Repository-wide coding specifications for ZOO modules

## 1. Purpose

This document defines mandatory coding specifications for all modules in the ZOO workspace.
The goals are:
- Maintainable and consistent code style.
- Predictable runtime behavior.
- Safe memory and thread usage.
- Testable, observable, and performance-aware implementations.

## 2. Applicability

These specifications apply to:
- C source files in `*/src`.
- Public and private headers in `*/inc` and `*/src`.
- Tests in `*/tests`.
- Build integration through CMake files.

## 3. Language and File Conventions

1. Use C11-compatible code unless module toolchain requires a stricter subset.
2. Source files use `.c`; header files use `.h`.
3. Public APIs must be declared in module `inc` headers.
4. Internal-only declarations should stay in module-private headers/source.
5. Prefer ASCII text in source and docs unless non-ASCII is required.

## 4. Naming Conventions

1. Public symbols must use module prefix naming (example: `zoo_smb_*`).
2. Types:
- Struct typedefs: `ZOO_<MODULE>_<NAME>_STRUCT`.
- Enum typedefs: `ZOO_<MODULE>_<NAME>_ENUM`.
- Handle typedefs: pointer typedef with `_HANDLE` suffix.
3. Functions:
- Verb-first action names (`create`, `destroy`, `set`, `get`, `init`, `register`, `publish`).
4. Macros:
- Uppercase with module prefix.
5. Static/internal helpers:
- Lower snake case and `static` scope.

## 5. Comments Specifications

1. Comments must explain intent, constraints, or non-obvious behavior, not restate code.
2. Public API declarations must include concise comment blocks describing:
- Purpose.
- Parameters and ownership expectations.
- Return value and error behavior.
3. Complex logic blocks (state machines, retry logic, lock-sensitive flows) should include short rationale comments.
4. Keep comments accurate during refactors; outdated comments are treated as defects.
5. Use complete, direct sentences and avoid ambiguous wording.
6. Do not leave commented-out code in committed source; use version history instead.
7. Use TODO/FIXME tags only with a clear action and scope.
8. File header comments should include module/component metadata when required by module conventions.
9. Performance-sensitive paths should annotate important invariants (for example, lock ordering or amortized complexity assumptions).
10. Security- or safety-relevant checks should include short comments where misuse risk is high.

## 6. API Design Rules

1. Validate all external inputs at API boundary.
2. Return repository-standard error codes for recoverable failures.
3. Return `NULL` only for pointer-return APIs where failure is expected/defined.
4. Do not leak implementation details through public headers.
5. Keep API behavior deterministic for same inputs and state.

## 7. Error Handling

1. Check all allocations and external calls.
2. Use single-exit cleanup pattern when multiple resources are acquired.
3. Log actionable errors once at failure boundary.
4. Never ignore function results unless explicitly safe; when ignored, cast to `(void)`.
5. Map low-level errors to module-level error codes before returning to callers.

## 8. Memory Management

1. Use project-approved allocators (for example memory pool APIs) in module code.
2. Every allocation must have exactly one corresponding release path.
3. On partial initialization failure, free all previously acquired resources.
4. Avoid double-free by nulling handles after destroy in mutable contexts.
5. Ownership transfer must be documented in function comments.

## 9. Concurrency and Synchronization

1. Shared mutable state must be protected by mutex/lock discipline.
2. Keep critical sections as short as possible.
3. Do not perform unbounded scans while holding locks in hot paths.
4. Condition variable waits must always be in predicate loops.
5. Use consistent lock acquisition ordering to avoid deadlocks.

## 10. Performance Specifications

1. Hot-path operations should target O(1) average lookup where practical.
2. Avoid repeated linear scans in per-message/per-request paths.
3. Gate verbose logs out of hot paths.
4. Apply bounded data structures using policy/resource limits.
5. Add metrics and benchmarks for changes that affect critical paths.

## 11. Logging and Observability

1. Use project logging macros only.
2. Log levels:
- ERROR: operation failure requiring action.
- WARN: recoverable anomaly.
- INFO: lifecycle milestones.
- DEBUG: diagnostic detail.
3. Do not log sensitive payload content by default.
4. Include correlation identifiers (request IDs, message IDs) when available.

## 12. QoS/Policy Specifications

1. QoS fields must be either:
- Runtime-enforced, or
- Explicitly documented as negotiation/advisory-only.
2. Validation must reject invalid ranges and incompatible combinations.
3. Compatibility checks must produce deterministic mismatch reasons.
4. Time-based policies must use monotonic or consistent timestamp source.
5. Default policy values must be meaningful and not stale constants.

## 13. Testing Requirements

1. New features require unit tests for success and failure paths.
2. Bug fixes require a regression test reproducing prior behavior.
3. Performance-sensitive changes require benchmark or measurable comparison.
4. Concurrency changes require stress or race-oriented tests where applicable.
5. Public API changes require compatibility validation in examples/integration tests.

## 14. CMake and Build Integration

1. Each module must expose clear `CMakeLists.txt` targets.
2. Public include directories must be explicit and minimal.
3. Avoid global compile definitions unless required.
4. Warnings should be enabled and treated consistently across modules.
5. New module/test targets must be discoverable from parent build scripts.

## 15. Documentation Requirements

1. Each module should provide:
- Requirements document.
- Design document.
- Architecture document.
2. Public APIs need concise comments for parameters, ownership, and errors.
3. Behavior impacting performance or reliability must be documented.
4. Keep docs synchronized with code changes in the same PR/commit.

## 16. Code Review Checklist

1. API contract correctness.
2. Error/cleanup correctness.
3. Memory ownership correctness.
4. Locking correctness and contention impact.
5. Test coverage for normal/error/edge cases.
6. Logging usefulness and noise level.
7. Performance impact on hot paths.
8. Documentation updated.

## 17. Compliance Policy

1. These specifications are normative for new and modified code.
2. Existing legacy code may be improved incrementally when touched.
3. Deviations must be justified in review notes and approved.
4. Repeated violations should trigger module-level remediation tasks.

## 18. Revision History

- 2026-04-05: Initial repository-level code specifications.
- 2026-04-05: Added repository-wide comments specifications section.
