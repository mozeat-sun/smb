# Traceability Guide

This guide defines how requirements are linked to code, tests, and release evidence.

## Required linkage

Each significant change should include:

- One or more requirement IDs from `docs/REQUIREMENTS_CATALOG.md`
- At least one verification artifact
- A release note or change record if behavior changes

## Recommended locations for requirement IDs

- Source comments near critical logic
- Test names or test comments
- Pull request descriptions
- Benchmark reports and release scorecards

## Example

```c
/* REQ-REL-003: verify overload handling policy when queue is saturated */
```

```c
TEST(zoo_smb_backpressure, queue_saturation_req_rel_003)
```

## CI artifact generation

Use `tools/quality/generate_traceability_matrix.sh` to generate a simple requirement coverage report from repository content.

## Review gate

A release candidate should not be approved if critical requirement IDs have no linked verification evidence.
