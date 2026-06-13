# ZOO Communication Bus Grade Roadmap

This roadmap defines how ZOO evolves from a strong engineering project into a top-tier communication bus for embedded markets.

## Mission

Build a communication bus that progresses through three assurance levels:

1. Industrial grade
2. Automotive grade
3. Military and aerospace grade

Each level has mandatory architecture controls, process controls, verification targets, and release gates.

## Principles

- Safety and reliability are engineered early, not added later.
- Every grade has explicit, measurable exit criteria.
- Claims are backed by reproducible evidence and audit-ready artifacts.
- Upward compatibility is controlled through strict API and wire-version policy.

## Current Grade Statement

Current repository claim as of 2026-05-31:

- The repository is operating in pre-Grade-1 completion territory.
- The current implementation and evidence set support late Milestone M3 with repository-level M4 scaffolding, but do not yet satisfy the full Grade 1 Industrial exit gates.
- Grade 1 hardening progress is substantial in benchmark governance, reliability scaffolding, traceability, security workflow evidence, and repository-wide code-specification alignment in core SMB paths.
- Full repository verification currently passes in the maintained build profile: full CTest suite and local examples execution are green for the current baseline.
- Grade 1 remains unachieved until long-duration operational evidence and external pilot criteria are complete.
- Grade 2 Automotive and Grade 3 Military/Aerospace claims are not currently supportable from repository evidence.

Evidence basis for this statement:

- `docs/readiness/ROADMAP_EXECUTION_BACKLOG.md` records the current repository status as late M3 with M4 scaffolding in place.
- `docs/readiness/ARCHITECTURE_REQUIREMENTS_ASSESSMENT.md` states that the architecture is sufficient for next industrial-grade hardening iterations, but not yet sufficient for later automotive-grade and military/aerospace-grade iterations.
- Current repository validation runs confirm full CTest pass and full local example pass for the latest baseline.
- Grade 1 exit gates below still require completion of longer-duration reliability, recovery, and field-use evidence.

## Grade 1: Industrial

### Target profile

- Factory automation, robotics, smart manufacturing, edge gateways
- High uptime and deterministic behavior under noise, congestion, and partial faults

### Required standards and process baseline

- IEC 61508 informed process (initially target SIL support architecture, even if not formally certified in first cycle)
- ISO 9001 style quality management practices
- Secure development lifecycle with vulnerability response policy

### Architecture requirements

- Deterministic scheduling mode for time-sensitive message classes
- Bounded memory operation mode with no unbounded heap growth in runtime hot paths
- Configurable backpressure policies: drop, block, priority shed
- Crash-safe persistence layer for durable channels
- Unified health model: liveness, readiness, degraded state

### Reliability targets

- 30-day soak test with no memory growth beyond defined threshold
- Mean time between service-impacting faults greater than 90 days in reference deployment
- Recovery time objective under 5 seconds for single-process crash restart
- Packet loss tolerance and congestion behavior validated under fault injection

### Test and verification stack

- Unit, integration, and system tests as required checks
- Deterministic replay tests for protocol parser and routing core
- Fault injection suite: network jitter, packet drop, disk full, clock skew
- Continuous fuzzing for protocol and parser surfaces

### Grade 1 exit gates

- Public benchmark report with p50, p95, p99 latency and throughput curves
- Signed release checklist including reliability and security evidence
- Production pilot with at least two external industrial users

## Grade 2: Automotive

### Target profile

- In-vehicle gateways, zonal controllers, central compute communication fabric

### Required standards and process baseline

- ASPICE process capability targets for software lifecycle
- ISO 26262 safety lifecycle integration
- UNECE R155 and R156 aligned cybersecurity and update governance

### Architecture requirements

- Safety partitioning model for mixed criticality traffic
- Freedom-from-interference controls between safety domains
- Deterministic transport profile with bounded worst-case latency for safety channels
- Time synchronization strategy (PTP or equivalent profile)
- Secure boot and signed artifact chain for deployable bus components

### Safety and security requirements

- Hazard analysis and risk assessment for communication failure modes
- Safety goals and technical safety requirements mapped to architecture
- Threat analysis and risk assessment for protocol, control plane, and update path
- Cryptographic identity per node, mutual authentication by default

### Verification targets

- Requirements traceability from system requirements to tests
- Structural coverage strategy appropriate to assigned ASIL targets
- Tool qualification plan where required by safety case
- Formal verification or model checking on critical protocol state transitions

### Grade 2 exit gates

- Safety case package suitable for OEM and assessor review
- Cybersecurity case package with penetration test and red-team results
- Vehicle-like hardware-in-the-loop validation completed
- Controlled API and protocol compatibility policy with deprecation lifecycle

## Grade 3: Military and Aerospace

### Target profile

- Mission systems, avionics-adjacent compute, tactical communication middleware

### Required standards and process baseline

- DO-178C process alignment for airborne software pathways where applicable
- DO-330 tool qualification strategy for verification tools as needed
- ARP4754A and ARP4761 compatible systems engineering interfaces where needed
- Defense cybersecurity framework alignment (for example NIST 800-53 derived controls)

### Architecture requirements

- High-assurance partitioning and secure isolation model
- Multi-level security capable policy enforcement model
- Deterministic degraded modes under contested and denied environments
- Anti-tamper and key lifecycle controls with offline recovery procedures
- Long-term support branch strategy with reproducible builds

### Resilience and assurance requirements

- Byzantine and adversarial fault model testing for control path hardening
- Deterministic behavior under timing stress and communication denial
- Provenance and supply-chain assurance for dependencies
- Configuration baseline freeze and strict change control for certified baselines

### Verification and validation

- Full requirements verification matrix with independent review workflow
- Worst-case execution and timing analysis for critical paths
- Environmental test evidence on representative target hardware
- Independent security and safety assessment reports

### Grade 3 exit gates

- Audit-ready certification evidence set by profile
- Independent assessment sign-off by qualified third party
- Operational readiness demonstration in mission-like scenarios

## Cross-grade technical roadmap

## Phase A: Core hardening

- Stabilize core protocol and routing invariants
- Add explicit wire protocol versioning and compatibility tests
- Introduce deterministic memory profile and telemetry baseline

### Transport Auto-Choose Architecture (Same Host -> SHM)

Objective:

- When two processes communicate on the same machine, automatically select SHM transport for lower latency and lower kernel/network overhead.
- Preserve compatibility and fail-open behavior by falling back to network transport when SHM is unavailable.

Design principles:

- Deterministic selection precedence with explicit override support.
- Backward-compatible discovery payload extension.
- No in-session transport migration in initial implementation.
- Strong observability for selection and fallback decisions.

Selection precedence:

1. Explicit node/service transport setting (manual override) wins.
2. If auto-select is disabled, keep current transport behavior.
3. If auto-select is enabled and peer is same-host and SHM-capable, choose SHM.
4. Otherwise choose discovered/default network transport.

Required metadata and protocol extensions:

- Add host fingerprint to service-discovery metadata.
- Add transport capability bitmask to service-discovery metadata.
- Add optional SHM endpoint/channel hint.
- Keep old discovery payloads valid by treating new fields as optional.

Resolver integration points:

- Add a transport resolver stage in transport manager before transport creation.
- Resolver input: node intent, service metadata, global config, and local host fingerprint.
- Resolver output: selected transport type, fallback candidate, and decision reason.

Failure and fallback policy:

- If SHM attach/register fails, fall back automatically to configured network transport.
- Apply cooldown before retrying SHM to avoid oscillation.
- Record fallback reason and retry counters.

Telemetry and evidence:

- Selection counters by transport type.
- Fallback counters by reason.
- Selection decision latency metric.
- Quality artifact summarizing auto-select decisions in CI verification runs.

## Phase B: Operational excellence

- Build observability package: metrics, traces, event logs, health APIs
- Add performance lab with reproducible benchmark scripts
- Introduce release train with quality gates and rollback playbooks

## Phase C: Safety and assurance

- Build requirements and traceability system
- Add safety analysis artifacts and hazard controls
- Expand test evidence for safety and security claims

## Phase D: Certification readiness

- Prepare compliance evidence packages by grade
- Lock toolchain and dependency provenance process
- Run pre-assessment with external certification advisors

## KPI dashboard

Track these KPIs continuously:

- Availability percentage by deployment profile
- p99 latency and jitter by message class
- Recovery time objective and failover success rate
- Security vulnerability mean time to remediate
- Test pass rate, coverage quality, and escaped defect rate
- API and protocol breakage rate per release

## Governance model

- Technical Steering Committee for architecture and compatibility decisions
- Safety Review Board for safety requirements and evidence acceptance
- Security Review Board for threat model and vulnerability response
- Release Authority role for grade gate approvals

## Suggested timeline

- Industrial grade: 12 to 18 months
- Automotive grade: additional 18 to 30 months
- Military and aerospace grade: additional 24 to 36 months

Timelines assume a dedicated cross-functional team and disciplined process adoption.

## Immediate next 90 days

- Freeze v1 protocol core and publish compatibility contract
- Build reliability and fault injection test harness
- Launch benchmark and soak test pipeline in CI
- Publish public quality scorecard for each release
- Start formal requirements traceability repository

## Execution artifacts in this repository

- Roadmap backlog: `docs/readiness/ROADMAP_EXECUTION_BACKLOG.md`
- Release scorecard template: `docs/release/process/RELEASE_QUALITY_SCORECARD.md`
- Audit evidence checklist: `docs/assurance/AUDIT_ARTIFACT_CHECKLIST.md`
- Requirements catalog: `docs/requirements/REQUIREMENTS_CATALOG.md`
- Traceability guide: `docs/assurance/TRACEABILITY_GUIDE.md`
- Benchmark baseline process: `docs/readiness/BENCHMARK_BASELINE_PROCESS.md`
- Quality scripts: `tools/quality/`
- CI quality gates: `.github/workflows/quality-gates.yml`
