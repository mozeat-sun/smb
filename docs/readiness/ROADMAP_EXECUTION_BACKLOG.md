# Roadmap Execution Backlog

This file contains issue-ready epics and milestones aligned with the grade roadmap.

## Usage

- Create a GitHub milestone for each phase.
- Copy each issue title and checklist into a GitHub issue.
- Link pull requests to these issues and track evidence artifacts.

## Current Repository Status (2026-04-16)

Current execution position:

- Milestone M1 is effectively complete in repository artifacts and CI scaffolding.
- Milestone M2 is complete in repository artifacts and CI gating for reliability, throughput governance, and stabilized bench6 latency enforcement.
- Milestone M3 repository-level traceability and security evidence are now in place.
- Milestone M4 repository-side certification-readiness scaffolding is now in place for safety case packaging, change control, review workflow, and evidence freeze procedure.
- Milestone M4 Issue 8 now includes a repository-enforced readiness reporting gate that validates findings-log quality and emits assessor-facing readiness artifacts.
- Grade 1 closure evidence now includes a recovery-RTO gate artifact and a multi-scenario latency matrix artifact generated in quality automation.
- Grade 1 closure tracking now includes explicit intake files for independently operated 30-day soak evidence and external-user pilot evidence.

Current practical milestone call:

- Late M3, with repository-level M4 scaffolding in place.
- M3 implementation slice is complete in repository artifacts and quality gates; remaining gaps are higher-grade follow-on work rather than Milestone M3 blockers.
- M4 still requires execution-time assessor engagement, findings closure, and controlled-baseline discipline beyond repository-side templates and gates.

## Prepared Next Step (2026-04-16)

Execution focus: Grade 1 closure and evidentiary gate tracking, while keeping Milestone M4 Issue 8 in controlled progress.

Reason for prioritization:

- Grade 1 remains the required predecessor before any Grade 2 claim.
- Repository-side Grade 1 automation is now broad enough to support objective gate tracking.
- Remaining blockers are now concentrated in long-duration operational evidence and external pilot execution.

Issue-ready implementation slice:

1. Add Grade 1 exit status automation
	- Add `tools/quality/grade1_exit_gate.sh`.
	- Generate `artifacts/quality/grade1_exit_report.md` and `artifacts/quality/grade1_exit_summary.json`.
2. Add Grade 1 checklist baseline
	- Add `docs/assurance/GRADE1_EXIT_CHECKLIST.md` with gate IDs and required evidence paths.
3. Wire Grade 1 status into CI artifacts
	- Add `grade1-exit-status` job in `.github/workflows/quality-gates.yml`.
	- Publish Grade 1 status artifacts on pull requests and protected-branch builds.

Definition of done for this slice:

- Grade 1 status report is generated from repository evidence artifacts.
- Grade 1 checklist maps every exit gate to explicit evidence locations.
- CI publishes Grade 1 status artifacts for release and assessor review.

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
- Benchmark quality artifacts now include a latency scenario matrix covering both bench5 RPC round-trip and bench6 pub/sub one-way paths.

Exit evidence:

- Baseline benchmark report
- Regression gate active in CI

Additional evidence now produced:

- `artifacts/quality/latency_scenario_matrix.md`
- `artifacts/quality/latency_scenario_matrix.json`

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
- `docs/security/THREAT_MODEL.md` provides the maintained threat-model baseline for protocol, control plane, and release surfaces.
- `docs/security/VULNERABILITY_REMEDIATION_SLA.md` and `docs/security/VULNERABILITY_REMEDIATION_LOG.csv` provide tracked remediation evidence, and CI generates an SLA summary artifact.

Exit evidence:

- Threat model revision approved
- Security findings trend and SLA report

## Milestone M4: Certification Readiness

### Issue 7: Safety case package skeleton

Goal: prepare cert-ready structure for automotive and beyond.

Checklist:

- [x] Define safety argument structure and evidence mapping
- [x] Define toolchain baseline and change control policy
- [x] Add independent review checklist for safety artifacts
- [x] Create release evidence freeze procedure

Current status note:

- Repository now includes a safety case skeleton, toolchain and change-control policy, independent safety review checklist, and release evidence freeze procedure.
- These artifacts complete the repository-side Milestone 4 Issue 7 scaffolding, but do not replace formal assessor review or certified baseline governance execution.

Exit evidence:

- Safety case skeleton approved
- Controlled baseline release produced

### Issue 8: Compliance pre-assessment

Goal: validate readiness with external advisors.

Checklist:

- [ ] Run pre-assessment for target standard profile
- [x] Record findings and remediation plan
- [ ] Re-run pre-assessment after remediation
- [x] Publish readiness summary for stakeholders

Current status note:

- Repository now includes the pre-assessment plan, findings log template, and readiness summary template.
- Repository now includes `tools/quality/pre_assessment_readiness_gate.sh` and CI artifact publication for pre-assessment readiness evidence quality.
- Repository now includes `tools/quality/pre_assessment_remediation_plan.sh` and CI artifact publication for findings-linked remediation plan evidence.
- Initial internal round findings and stakeholder readiness summary are now recorded in `docs/readiness/PRE_ASSESSMENT_FINDINGS_LOG.csv` and `docs/readiness/PRE_ASSESSMENT_READINESS_SUMMARY.md`.
- The actual advisor-led pre-assessment and findings closure remain execution-time work outside the repository.

Exit evidence:

- Pre-assessment report
- Closed corrective actions list

## Grade 1 Remaining Gaps (Repository Scope)

These are the remaining Grade 1 blockers after current repository automation updates:

- 30-day soak and 90-day MTBF evidence from reference deployment operation
- Production pilot evidence with at least two external industrial users
- Formal release execution record with reviewer sign-off on CI baseline artifacts

Operational tracking artifacts:

- `docs/assurance/GRADE1_EXIT_CHECKLIST.md`
- `artifacts/quality/grade1_exit_report.md`
- `artifacts/quality/grade1_exit_summary.json`
