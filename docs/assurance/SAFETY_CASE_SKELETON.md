# Safety Case Skeleton

Purpose:

- define the repository-level safety argument structure for higher-assurance reviews
- map claims to existing and planned evidence artifacts
- provide a stable package outline for assessor-facing release baselines

Scope:

- repository-controlled ZOO SMB software artifacts
- quality, verification, and release-governance evidence already maintained in this repository
- gap tracking for later automotive and aerospace-grade expectations

## 1. Safety case intent

Top-level claim:

- ZOO SMB can be released for the approved deployment profile with risks understood, evidence reviewed, and known limitations explicitly recorded.

Supporting claim groups:

1. Product definition and boundaries are controlled.
2. Requirements and architecture are traceable.
3. Verification evidence supports release claims.
4. Security and vulnerability management are active.
5. Release execution and change control preserve evidence integrity.

## 2. Argument structure

### Claim C1: Product definition and operational boundary are controlled

Subclaims:

- C1.1 Architecture boundaries are documented.
- C1.2 Compatibility expectations are documented.
- C1.3 Known limitations and intended deployment assumptions are recorded.

Current evidence:

- `docs/architecture/ARCHITECTURE_DESIGN_SPECIFICATION.md`
- `docs/readiness/ARCHITECTURE_REQUIREMENTS_ASSESSMENT.md`
- `docs/requirements/PROTOCOL_COMPATIBILITY_POLICY.md`
- `docs/release/records/M3-RC1_RELEASE_NOTES.md`

### Claim C2: Requirements, design, and verification are traceable

Subclaims:

- C2.1 Safety-relevant requirements have unique identifiers.
- C2.2 Requirements are linked to design and implementation.
- C2.3 Requirements have verification evidence or explicit gaps.

Current evidence:

- `docs/requirements/REQUIREMENTS_CATALOG.md`
- `docs/assurance/TRACEABILITY_GUIDE.md`
- `artifacts/quality/traceability_matrix.csv`
- `artifacts/quality/traceability_audit_sample.md`
- `artifacts/quality/traceability_gate_summary.json`

### Claim C3: Verification evidence supports current release claims

Subclaims:

- C3.1 Reliability evidence exists.
- C3.2 Performance evidence exists.
- C3.3 Fault and malformed-input evidence exists.
- C3.4 Evidence gaps are identified for higher-grade profiles.

Current evidence:

- `artifacts/quality/soak_summary.json`
- `artifacts/quality/reliability_until_fail_summary.json`
- `artifacts/quality/benchmark_summary.json`
- `artifacts/quality/fuzz_summary.json`
- `docs/readiness/BENCHMARK_BASELINE_PROCESS.md`

### Claim C4: Security management supports the release decision

Subclaims:

- C4.1 Threats and trust boundaries are documented.
- C4.2 Vulnerability handling is tracked against explicit targets.
- C4.3 Static analysis and dependency scanning are part of controlled review.

Current evidence:

- `docs/security/THREAT_MODEL.md`
- `docs/security/VULNERABILITY_REMEDIATION_SLA.md`
- `docs/security/VULNERABILITY_REMEDIATION_LOG.csv`
- `artifacts/quality/vulnerability_sla_summary.json`

### Claim C5: Release and change control preserve evidence integrity

Subclaims:

- C5.1 Release scorecard and release notes are completed per candidate.
- C5.2 Toolchain and change control policy are defined.
- C5.3 Evidence freeze procedure exists.
- C5.4 Independent review workflow exists.

Current evidence:

- `docs/release/process/RELEASE_QUALITY_SCORECARD.md`
- `docs/release/records/M3-RC1_RELEASE_NOTES.md`
- `docs/requirements/TOOLCHAIN_BASELINE_AND_CHANGE_CONTROL.md`
- `docs/release/process/RELEASE_EVIDENCE_FREEZE_PROCEDURE.md`
- `docs/assurance/INDEPENDENT_SAFETY_REVIEW_CHECKLIST.md`

## 3. Evidence mapping table

| Claim | Required evidence type | Current repository artifact | Gap status |
|---|---|---|---|
| C1 | architecture and scope | architecture docs, release notes | Partial for higher-grade deployment constraints |
| C2 | requirements traceability | catalog, guide, traceability artifacts | Present at repository gate level |
| C3 | verification and benchmark evidence | soak, reliability, benchmark, fuzz artifacts | Partial for broader scenario and timing evidence |
| C4 | security governance | threat model, SLA policy, tracked log, CI artifacts | Present for repository gate scope |
| C5 | release and change control | scorecard, release notes, change-control and freeze docs | Present as process skeleton |

## 4. Mandatory safety case package contents

- release quality scorecard
- release notes for the candidate
- traceability gate summary and audit sample
- benchmark, soak, reliability, and fuzz summaries
- threat model and vulnerability SLA records
- toolchain baseline and change-control policy
- release evidence freeze record
- independent review checklist and sign-off record

## 5. Known repository gaps for later certification profiles

- no formal hazard analysis and risk assessment package yet
- no tool qualification plan yet
- no structural coverage policy or report yet
- no worst-case timing analysis for safety-critical paths yet
- no independent third-party assessor report yet

## 6. Approval workflow

The safety case package is considered review-ready only when:

1. the release scorecard records a `Go` decision with known limitations listed
2. the freeze procedure has been completed for the candidate baseline
3. the independent review checklist has named reviewers and disposition notes
4. unresolved gaps are tracked as explicit accepted risks or blocking findings

## 7. Revision rule

Update this document whenever:

- a new release evidence type becomes mandatory
- the argument structure changes for the target assurance profile
- an assessor or internal review board requests different package grouping