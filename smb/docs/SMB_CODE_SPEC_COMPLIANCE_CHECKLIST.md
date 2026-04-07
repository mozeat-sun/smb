# SMB Code Specification Compliance Checklist

Date: 2026-04-05
Specification baseline: ../../docs/CODE_SPECIFICATIONS.md
Scope: smb source and public headers (excluding build artifacts and third-party dependencies)

## Summary

Overall status: Partially compliant with major high-severity gaps remediated in this pass.

## Checklist

1. Memory allocator policy in module code
- Status: PASS (for current searched source paths)
- Evidence:
  - `src/core/zoo_smb.c` now uses `zoo_allocate_from_pool` and `zoo_free_to_pool`.
  - No `malloc/calloc/realloc/free` matches found under `smb/src/**` during audit search.

2. Public API comments quality
- Status: PASS (for placeholder comment patterns audited)
- Evidence:
  - Placeholder forms like `@param ...`, `please specify`, and `Add parameter descriptions` were removed from public headers.
  - Updated headers include:
    - `inc/qos/zoo_smb_qos_policy.h`
    - `inc/core/zoo_smb_service.h`
    - `inc/core/zoo_smb_service_manager.h`
    - `inc/core/zoo_smb_transport_manager.h`
    - `inc/transport/zoo_smb_protocol.h`
    - `inc/node/zoo_smb_node_observer.h`
    - `inc/core/zoo_smb_message.h`

3. Commented-out code in active source
- Status: PASS
- Evidence:
  - Removed commented-out timer fields and calls in `src/qos/zoo_smb_qos.c`.

4. Unsafe unbounded string formatting
- Status: PASS (for known call sites remediated)
- Evidence:
  - Replaced `sprintf` with `snprintf` in:
    - `src/transport/zoo_smb_consumer.c`
    - `src/node/zoo_smb_node_observer.c`
    - `src/core/zoo_smb_routing_rule.c`

5. TODO/FIXME usage clarity
- Status: PASS (with open work items)
- Evidence:
  - TODOs include explicit scope labels and concrete action:
    - `src/transport/zoo_smb_transport_shm_common.c`
    - `src/utility/zoo_smb_config.c`

6. QoS default-policy sanity
- Status: PASS
- Evidence:
  - Replaced stale absolute deadline default with neutral timestamp value in `src/qos/zoo_smb_qos_policy.c`.

## Remaining Gaps and Risks

1. Performance conformance is still incomplete for hot paths.
- Request-state and subscriber management still rely on repeated list scans and lock-held searches in QoS/publisher paths.
- This remains a compliance gap against O(1)-target and lock-contention guidance.

2. Allocation dependency risk outside auto-init mode.
- `src/core/zoo_smb.c` now uses memory-pool allocation in both auto-init and non-auto-init builds.
- If pool initialization is not completed before SMB init in non-auto-init integrations, initialization can fail.

## Recommended Follow-up

1. Implement hash-indexed lookup for QoS state and subscriber registry.
2. Add explicit startup validation/logging for memory-pool readiness in non-auto-init flow.
3. Add CI compliance checks for placeholder comments, unbounded formatting calls, and banned allocators.
