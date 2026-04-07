# SMB QoS Performance Review

Date: 2026-04-05
Scope: QoS policy path, negotiation path, and runtime QoS state path in SMB

## 1) Hot Path Mapping (Current State)

### 1.1 Publish with reliability enabled

Call path:
1. `zoo_smb_publish_message` checks reliability and subscriber count.
2. For each subscriber, `publisher_set_all_subscribers_qos_msg_status` sets request state.
3. Per subscriber QoS context, `zoo_smb_qos_ctx_set_state` acquires a mutex, prunes expired entries, and linearly searches request state.
4. Message is sent.

Key files:
- `src/node/zoo_smb_publisher.c`
- `src/qos/zoo_smb_qos_ctx.c`

Observed complexity:
- Per publish: O(S) subscriber iteration.
- Per subscriber update: lock + O(M) list search (+ prune pass).
- Aggregate: roughly O(S * M) in the reliability tracking layer under load.

### 1.2 Subscriber QoS negotiation/update

Call path:
1. Publisher receives SUB message.
2. `zoo_smb_qos_policy_match` computes compatibility.
3. `publisher_register_or_update_subscriber_qos_ctx` finds subscriber by name and inserts/updates QoS context.

Key files:
- `src/node/zoo_smb_publisher.c`
- `src/qos/zoo_smb_qos_policy.c`

Observed complexity:
- Subscriber lookup is O(S) linear scan by string compare.
- Update path may free and recreate policy/context objects even for frequent churn.

### 1.3 Wait/ack status queries

Call path:
1. `zoo_smb_qos_ctx_wait_for_status` loops until timeout.
2. Each loop: lock -> prune -> linear find -> timed condition wait.

Key files:
- `src/qos/zoo_smb_qos_ctx.c`

Observed complexity:
- Repeated O(M) scans under lock for each wakeup.
- Expiration pruning is list rotation O(M) and runs inside critical section.

## 2) Main Performance Findings

1. Policy object itself is not the primary bottleneck.
- `zoo_smb_qos_policy_match` is constant-size field comparison and cheap.
- Policy allocation uses pool allocator, also relatively cheap.

2. Runtime state structure is list-centric.
- Request state operations use repeated linear find in `qos_ctx->messages`.
- Expiration prune is full traversal while locked.

3. Subscriber registry is list-centric.
- Frequent lookup path uses linear scan by subscriber name.

4. Logging volume is high in policy creation/initialization.
- Policy logging prints every field and can become expensive in debug/info-heavy builds.

5. Some QoS fields are policy-only metadata today.
- Several fields participate in validation/match but do not affect runtime behavior.

## 3) Concrete Redesign for High Performance

### 3.1 Data structures

Replace list-based hot structures with indexed structures:

1. Subscriber map in publisher
- Replace `publisher->subscribers` list lookup with hash map keyed by subscriber name.
- Keep optional compact vector only for iteration when broadcasting status.

2. Request state map in qos ctx
- Replace `qos_ctx->messages` linear list with hash map keyed by `request_id`.
- Store value as state object with timestamps/status/match bits.

3. Expiration management
- Add min-heap (or timing wheel) keyed by expire timestamp.
- Prune only expired head entries instead of full list rotation.

Expected impact:
- Subscriber lookup: O(1) average.
- Set/get/remove state: O(1) average.
- Prune: O(k log N) where k is expired count, usually much smaller than N.

### 3.2 Concurrency strategy

1. Keep per-qos-ctx lock, but shorten critical sections.
2. Avoid scanning/pruning entire container while holding lock.
3. For high fanout publishers, shard subscriber map by hash bucket lock or use RW lock.

### 3.3 Memory strategy

1. Continue using memory pool for state objects.
2. Add fixed-capacity ring or slab for per-context request states (bounded by resource limits).
3. Reuse state objects to reduce alloc/free churn under burst traffic.

### 3.4 API/behavior preservation

Maintain external API signatures:
- `zoo_smb_qos_ctx_set_state`
- `zoo_smb_qos_ctx_get_state`
- `zoo_smb_qos_ctx_wait_for_status`
- `publisher_register_or_update_subscriber_qos_ctx`

Internal replacement only, preserving current semantics and error codes.

### 3.5 Logging optimization

1. Keep policy dump behind debug guard and optional compile-time macro.
2. Use structured one-line logs for hot paths.
3. Disable field-by-field policy dump on common fast path.

## 4) QoS Field Implementation Audit (Implement vs Policy-only)

Legend:
- Runtime: affects runtime behavior beyond validation/match.
- Policy-only: currently only validated, set, or matched.

1. Priority
- Status: Policy-only.
- Evidence: set/validate in policy module; no scheduling/queue priority usage found in runtime path.
- Recommendation: either wire to queue/dispatcher priority or document as negotiation metadata only.

2. Reliability kind
- Status: Partial runtime.
- Evidence: reliability enabled gate is used to track publisher/subscriber message state and waits.
- Recommendation: keep and strengthen semantics around retransmit and ack windows.

3. Reliability max_blocking_time_ms
- Status: Runtime.
- Evidence: used as effective wait timeout fallback.
- Recommendation: keep.

4. Reliability attempts/retry_interval_ms
- Status: Policy-only in QoS layer.
- Evidence: set in policy object but not used by QoS retry scheduling path.
- Recommendation: wire to retry engine or remove from QoS policy and defer to transport config.

5. History
- Status: Partial runtime.
- Evidence: affects qos ctx capacity selection for KEEP_LAST depth.
- Recommendation: keep, but ensure strict overwrite/drop policy is enforced when full.

6. Durability
- Status: Policy-only.
- Evidence: used in compatibility checks but no persistence store/late-join replay path in QoS runtime.
- Recommendation: implement durable cache and replay, or label as negotiation-only.

7. Deadline duration
- Status: Runtime (limited).
- Evidence: used as timeout fallback and compatibility check.
- Recommendation: add missed-deadline counters/events.

8. Deadline absolute time
- Status: Policy-only / likely incorrect default usage.
- Evidence: default initializer hardcodes timestamp-like value not tied to runtime clock logic.
- Recommendation: remove absolute field or compute per-message deadline from now + duration.

9. Liveliness
- Status: Policy-only.
- Evidence: validated/matched only, no lease monitor/assert path.
- Recommendation: implement heartbeat/assertion checks or mark unsupported.

10. Ownership
- Status: Policy-only.
- Evidence: compatibility checks only, no arbitration on publisher side.
- Recommendation: implement exclusive owner selection by strength or mark unsupported.

11. Destination order
- Status: Policy-only.
- Evidence: compatibility checks only, no ordering mechanism at receive pipeline.
- Recommendation: wire to reorder buffer or mark unsupported.

12. Lifespan
- Status: Runtime.
- Evidence: used in state expiration pruning.
- Recommendation: keep; move pruning to efficient expiry index.

13. Latency budget
- Status: Policy-only.
- Evidence: compatibility checks only, no scheduler budget enforcement.
- Recommendation: implement dispatch deadline hints or document as advisory-only.

14. Resource limits
- Status: Partial runtime.
- Evidence: used for qos ctx capacity and validation/match constraints.
- Recommendation: enforce strict admission control and drop policy under pressure.

## 5) Phased Implementation Plan

Phase 1: Low-risk performance wins
1. Add guarded logging for policy dumps.
2. Introduce subscriber hash map while preserving list iteration compatibility.
3. Introduce request-id hash map in qos ctx; keep old list behind feature flag for fallback.

Phase 2: Expiration and wait optimization
1. Add expiry heap/wheel.
2. Remove full-scan prune in hot methods.
3. Add metrics for lock hold time and wait-loop wakeups.

Phase 3: Semantics completion
1. Implement or de-scope policy-only fields (durability, liveliness, ownership, destination order, latency budget, retry parameters).
2. Align docs and tests with implemented behavior only.

## 6) Acceptance Criteria

1. Functional
- Existing unit/integration tests pass.
- QoS negotiation result parity is preserved for existing fields.

2. Performance
- Subscriber lookup median latency reduced by at least 50% at S>=256.
- QoS ctx set/get state operations become O(1) average and show at least 2x throughput increase at M>=1024.
- Wait path lock hold time reduced significantly (target: <=30% of baseline p99).

3. Correctness
- No leaked state entries under sustained publish/ack churn.
- Expired states are removed without full-container scans in normal path.

## 7) Summary

The current SMB QoS policy schema is rich, but high performance is limited by list-based indexing and critical-section scans in QoS runtime paths, not by policy comparison itself. Moving subscriber and request-state lookups to hash-indexed structures, adding efficient expiration indexing, and either implementing or de-scoping policy-only fields will make the design appropriate for high-throughput workloads.
