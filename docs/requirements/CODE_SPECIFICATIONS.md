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
1. Use C11-compatible code unless the module toolchain requires a stricter subset.
2. Source files use `.c`; header files use `.h`.
3. Public APIs must be declared in module `inc` headers.
4. Internal-only declarations should remain in module-private headers or source files.
5. Prefer ASCII text in source and documentation unless non-ASCII is required.

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
	- Lower snake case with `static` scope.

## 5. Comments Specifications
1. Comments must explain intent, constraints, or non-obvious behavior, not restate code.
2. Public API declarations must include concise comment blocks describing:
	- Purpose.
	- Parameters and ownership expectations.
	- Return value and error behavior.
3. Public API comments must match the current function signature exactly (parameter names, directions, and semantics). Signature/comment mismatches are defects.
4. Function comments must describe the behavioral contract, not just summary wording. At minimum, document:
	- Blocking vs non-blocking behavior.
	- State changes and side effects.
	- Async/deferred execution behavior when applicable.
	- Handle/identifier reuse semantics when applicable.
5. For APIs with retries, state transitions, or reconciliation loops, comments must state transition triggers, terminal conditions, and retry/backoff intent.
6. For lock-sensitive or thread-facing APIs, comments must state required thread-safety assumptions (e.g., caller-held lock expectations or callback threading model).
7. Return-value comments must include success condition and principal error classes/codes expected by callers.
8. Complex logic blocks (state machines, retry logic, lock-sensitive flows) should include short rationale comments.
9. Keep comments accurate during refactors; outdated comments are treated as defects.
10. Use complete, direct sentences and avoid ambiguous wording.
11. Do not leave commented-out code in committed source; use version history instead.
12. Use TODO/FIXME tags only with a clear action and scope.
13. File header comments should include module/component metadata when required by module conventions.
14. Performance-sensitive paths should annotate important invariants (e.g., lock ordering or amortized complexity assumptions).
15. Security- or safety-relevant checks should include short comments where misuse risk is high.
16. Review gate: comments such as "registers intent" or "handles message" are insufficient unless the function contract clarifies timing, side effects, and failure behavior.

## 6. API Design Rules
1. Validate all external inputs at API boundary.
2. Return repository-standard error codes for recoverable failures.
3. Return `NULL` only for pointer-return APIs where failure is expected/defined.
4. Do not leak implementation details through public headers.
5. Keep API behavior deterministic for identical inputs and state.

## 7. Error Handling
1. Check all allocations and external calls.
2. Use single-exit cleanup pattern when multiple resources are acquired.
3. Log actionable errors once at the failure boundary.
4. Never ignore function results unless explicitly safe; when ignored, cast to `(void)`.
5. Map low-level errors to module-level error codes before returning to callers.

## 8. Memory Management
1. Use project-approved allocators (e.g., memory pool APIs) in module code.
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
4. Time-based policies must use monotonic or consistent timestamp sources.
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
- 2026-04-11: Strengthened comment contract rules for API behavior, async semantics, and review gate criteria.
- 2026-05-31: Added unified function coding standards (SRP, size, nesting, readability, refactoring).

## 19. Function Coding Standards

### 19.1 Core Principles

1. Adhere to the **Single Responsibility Principle**.
2. Each function shall perform only one independent task with clear logical boundaries.
3. Mixed functionalities in a single function are prohibited.

### 19.2 Code Line Limits

1. The effective lines of a single function **shall not exceed 100 lines** in principle.
2. Blank lines, comments, and curly braces are excluded from line count.
3. If extra lines are unavoidable due to complex business logic, add explanatory notes.
4. Refactor into multiple sub-functions whenever possible.

### 19.3 Nesting Rules

1. The maximum nesting level of `if/else`, `for`, `while`, `switch`, and other control statements is **limited to 3 layers**.
2. Flatten deep nested logic by using guard clauses, function splitting, enumerations, and similar methods.
3. Avoid nested code hell by prioritizing readability and linear control flow.

### 19.4 Readability Requirements

1. Use explicit, descriptive function names that clearly indicate functionality.
2. Arrange logic in natural, human-readable order to ensure smooth and coherent code.
3. Add concise comments for complex logic.
4. Redundant comments are prohibited to keep code clean and neat.

### 19.5 Supplementary Rules

1. Do not create monolithic, all-purpose functions.
2. Split and refactor code promptly when tight coupling occurs.
3. Simplify conditional judgments.
4. Extract complex conditions into separate boolean helper functions.
5. Follow unified indentation and blank line rules.
6. Use blank lines properly to separate logical blocks and improve visual hierarchy.

---

## 20. Formatting and Style Standards

### 20.1 Indentation and Spacing

1. Use **4 spaces** for indentation; do not use tabs.
2. Place one space after keywords: `if`, `for`, `while`, `switch`, `return`.
3. Place one space around binary operators: `=`, `+`, `-`, `*`, `/`, `==`, `!=`, `<`, `>`, `&&`, `||`.
4. Do not add extra spaces inside parentheses `()` or brackets `[]`.

### 20.2 Braces and Line Breaks

1. Use **Allman style** braces: each brace on its own line.
2. Single-line `if`/`for` statements still require braces.
3. Avoid multiple statements on the same line.

### 20.3 Line Length

1. Maximum line length: **120 characters**.
2. Wrap long lines logically to preserve readability.

### 20.4 Variable Declaration

1. Declare variables as close to usage as possible.
2. Initialize variables at declaration unless required otherwise.
3. Group related variables for readability.

### 20.5 Include Order

1. Standard library headers.
2. System headers.
3. Project/module public headers.
4. Module private headers.
5. Use alphabetical order within groups.

## 21. Type and Const Safety

1. Use `stdint.h` fixed-size types: `uint8_t`, `int32_t`, etc.
2. Prefer `const` for read-only data, pointers, and function parameters.
3. Avoid implicit type casts; use explicit casting only when necessary.
4. Minimize use of `void*` in public APIs.

## 22. Defensive Programming

1. Validate all input parameters at function entry.
2. Handle all edge cases and boundary values.
3. Use guard clauses at function start for fast failure.
4. Assume external inputs are untrusted.

## 23. Portability and Compatibility

1. Avoid compiler-specific extensions unless approved.
2. Do not hardcode platform-specific paths or values.
3. Use endian-safe and alignment-safe accessors.
4. Document non-portable code segments.

## 24. Security Best Practices

1. Validate and sanitize all external inputs.
2. Avoid buffer overflows; use bounded string functions.
3. Do not hardcode keys, secrets, or credentials.
4. Clear sensitive data from memory after use.
5. Follow least-privilege principles for resource access.

## 25. Deprecation and Evolution

1. Mark deprecated APIs with project-standard macros.
2. Document replacement APIs and migration steps.
3. Maintain backward compatibility for public APIs.
4. Remove deprecated code only after approved deprecation period.
