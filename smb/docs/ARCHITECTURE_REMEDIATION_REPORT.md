# SMB Bottom-up Remediation Report

## Scope completed in this round

### 1) Transport foundation hardening
- Added transport-level observer synchronization lock in transport base structure.
- Converted observer dispatch in TCP/UDP/UDP-broadcast/SHM client/SHM server to snapshot-based delivery to avoid iterator instability during concurrent register/unregister.
- Unified transport send retry in generic transport send path using configured `max_send_attempts` and `retry_interval_s`.
- Improved TCP send reliability with full-write retry (`tcp_write_with_retry`) and readiness waiting.
- Improved UDP send reliability with bounded retry for transient errors (`EINTR`/`EAGAIN`).

### 2) Transport manager resilience
- Added per-transport send-health tracking in transport index entries:
  - consecutive failures
  - total successes/failures
  - circuit-open cooldown timestamp
- Added lightweight circuit-breaker behavior in manager send path:
  - open after threshold consecutive send failures
  - temporary cooldown before accepting new sends
- Routed broadcast path through manager send API to reuse unified send policy.

### 3) Routing/rule manager performance and robustness
- Added hashed rule index to rule manager for fast rule lookup by name.
- Added manager mutex to guard rule list/index consistency.
- Integrated index upsert/remove with add/remove/make/get paths.
- Hardened routing engine ingress path with null checks and enqueue-failure cleanup.
- Optimized observer loop in routing engine by caching observer count.

## Why this is a bottom-up improvement
- Lowest layer first: transport internals, send semantics, observer safety.
- Middle layer: transport manager policy (send health/circuit breaking).
- Upper core layer: routing/rule lookup and ingress reliability.

## Current architecture status after remediation
- Better reliability under transient network failure.
- Better concurrency safety on observer dispatch paths.
- Better lookup complexity in rule management.
- Better pressure handling behavior in routing ingress.

## Remaining gaps to reach “industry-best”
- Cross-platform event backend abstraction still fragmented (epoll/select/usleep paths).
- Missing durable HA control plane (membership/leader election/state replication/WAL).
- End-to-end flow control still lacks explicit per-link queue watermarks and drop policies.
- System-wide SLO telemetry is not yet complete (p99 latency, queue depth heatmaps, retry reason cardinality).

## Recommended next execution package
1. Event backend unification (reactor abstraction for Linux/Windows/RTOS)
2. Per-transport send queue + watermark backpressure
3. Runtime-level health state propagation from transport manager metrics
4. Durable metadata and HA failover control plane
