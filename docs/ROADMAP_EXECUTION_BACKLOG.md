# Roadmap Execution Backlog

This file contains issue-ready epics and milestones aligned with the grade roadmap.

## Usage

- Create a GitHub milestone for each phase.
- Copy each issue title and checklist into a GitHub issue.
- Link pull requests to these issues and track evidence artifacts.

## Current Repository Status (2026-04-11)

Current execution position:

- Milestone M1 is effectively complete in repository artifacts and CI scaffolding.
- Milestone M2 is complete in repository artifacts and CI gating for reliability, throughput governance, and stabilized bench6 latency enforcement.
- Milestone M3 repository-level traceability and security evidence are now in place.

Current practical milestone call:

- Late M3, with early M4 scaffolding already in place.
- M3 implementation slice is complete in repository artifacts and quality gates; remaining gaps are higher-grade follow-on work rather than Milestone M3 blockers.

## Prepared Next Step (2026-04-09)

Execution focus: Milestone M1, Issue 1 (Freeze wire protocol v1).

Reason for prioritization:

- Architecture assessment marks compatibility requirements as partial and calls out missing interop matrix/release enforcement.
- Repository quality workflow already covers soak, benchmark, static analysis, dependency scan, fuzz, and scheduled runs.
- Closing protocol-governance gaps now reduces churn for later safety/security milestones.

Issue-ready implementation slice:

1. Define and publish protocol compatibility contract
	- Create `docs/PROTOCOL_COMPATIBILITY_POLICY.md`.
	- Include wire major/minor semantics, accepted/blocked combinations, fail-fast mismatch behavior, and deprecation lifecycle.
2. Add compatibility matrix artifact and CI check
	- Add `tools/quality/protocol_compatibility_matrix.sh` to generate `artifacts/quality/protocol_compatibility_matrix.md`.
	- Add a `protocol-compatibility` job in `.github/workflows/quality-gates.yml` and upload artifact.
3. Add README compatibility/deprecation policy section
	- Add a short section in `README.md` linking policy and matrix artifact location.
4. Add release-note template statement
	- Add `docs/templates/RELEASE_NOTES_TEMPLATE.md` with mandatory compatibility declaration fields:
	  - API compatibility status
	  - Wire compatibility status
	  - Breaking changes
	  - Deprecation notices

Definition of done for this slice:

- Policy document merged and referenced in README.
- Compatibility matrix artifact generated in CI for pull requests.
- Release notes template includes required compatibility statement fields.
- `docs/ARCHITECTURE_REQUIREMENTS_ASSESSMENT.md` updated to reflect reduced gap for REQ-COMP-002 and REQ-COMP-003.

## Milestone M1: Core Hardening

### Issue 1: Freeze wire protocol v1

Goal: publish a stable protocol contract and compatibility policy.

Checklist:

- [x] Define protocol version header and backward compatibility rules
- [x] Add protocol compatibility test matrix
- [x] Add semantic versioning and deprecation policy section to README
- [x] Publish compatibility statement in release notes template

Exit evidence:

- Protocol spec PR merged
- Compatibility tests passing in CI

### Issue 2: Deterministic memory profile mode

Goal: provide bounded memory mode for runtime hot path.

Checklist:

- [x] Add runtime config option for bounded memory behavior
- [x] Remove unbounded allocations from critical message path
- [x] Add memory watermark metrics
- [x] Add long-run memory stability test

Exit evidence:

- Peak memory trend report over 24h test
- Memory watermark alerts integrated

## Milestone M2: Operational Excellence

### Issue 3: Build reliability and soak pipeline

Goal: detect long-run stability regressions before release.

Checklist:

- [x] Add soak test job in CI
- [x] Add repeated test execution until-fail gate
- [x] Archive soak logs and summary artifacts
- [x] Add weekly scheduled reliability run

Exit evidence:

- 7-day soak pipeline report
- Failure classification and MTTR trend

### Issue 4: Benchmark and performance evidence

Goal: publish reproducible performance baselines.

Checklist:

- [x] Add benchmark script for throughput and latency sampling
- [x] Persist benchmark history as CI artifacts
- [x] Define regression thresholds per message size profile
- [x] Publish benchmark methodology doc

Current status note:

- CI benchmark workflow, rebuildable benchmark target, throughput comparison script, methodology document, and calibrated per-payload throughput baseline files now exist.
- Latency percentile and jitter enforcement are now active after bench6 pub/sub negotiation stabilization and repeated successful captures.

Exit evidence:

- Baseline benchmark report
- Regression gate active in CI

## Milestone M3: Safety and Assurance

### Issue 5: Requirements traceability system

Goal: trace requirements to implementation and tests.

Checklist:

- [x] Create requirements catalog with unique IDs
- [x] Tag tests with requirement IDs
- [x] Generate requirements-to-test matrix artifact in CI
- [x] Add review gate for traceability completeness

Current status note:

- Requirements catalog, traceability matrix generation, and audit-sample gate are active in CI.
- SMB integration tests, SMB unit tests, and benchmark/performance verification sources now include requirement IDs.
- `tools/quality/traceability_gate.sh` enforces linked evidence and required test coverage for critical verification requirements.

Exit evidence:

- Requirements coverage matrix
- Audit sample verifies end-to-end trace

### Issue 6: Security and threat evidence

Goal: operationalize security case for higher grades.

Checklist:

- [x] Maintain threat model for protocol and control plane
- [x] Add continuous static analysis and dependency scan jobs
- [x] Add protocol fuzz entry points and coverage reports
- [x] Track vulnerability remediation SLA

Current status note:

- Static analysis, dependency scan, fuzz, and vulnerability-SLA workflow artifacts are active in repository CI.
- `docs/THREAT_MODEL.md` provides the maintained threat-model baseline for protocol, control plane, and release surfaces.
- `docs/VULNERABILITY_REMEDIATION_SLA.md` and `docs/VULNERABILITY_REMEDIATION_LOG.csv` provide tracked remediation evidence, and CI generates an SLA summary artifact.

Exit evidence:

- Threat model revision approved
- Security findings trend and SLA report

## Milestone M4: Certification Readiness

### Issue 7: Safety case package skeleton

Goal: prepare cert-ready structure for automotive and beyond.

Checklist:

- [ ] Define safety argument structure and evidence mapping
- [ ] Define toolchain baseline and change control policy
- [ ] Add independent review checklist for safety artifacts
- [ ] Create release evidence freeze procedure

Exit evidence:

- Safety case skeleton approved
- Controlled baseline release produced

### Issue 8: Compliance pre-assessment

Goal: validate readiness with external advisors.

Checklist:

- [ ] Run pre-assessment for target standard profile
- [ ] Record findings and remediation plan
- [ ] Re-run pre-assessment after remediation
- [ ] Publish readiness summary for stakeholders

Exit evidence:

- Pre-assessment report
- Closed corrective actions list
