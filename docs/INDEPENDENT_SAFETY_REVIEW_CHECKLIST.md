# Independent Safety Review Checklist

Use this checklist for internal independent review before external assessor engagement.

## Review metadata

- Candidate version:
- Candidate tag:
- Review date:
- Lead reviewer:
- Independent reviewer:
- Scope:

## Independence check

- [ ] Reviewer is not the primary author of the reviewed change set
- [ ] Reviewer has access to the release artifacts and source baseline
- [ ] Reviewer understands the target assurance profile for this review

## Package completeness

- [ ] Safety case skeleton is complete for the candidate scope
- [ ] Release scorecard is complete and consistent with artifacts
- [ ] Release notes include compatibility and limitation statements
- [ ] Evidence freeze record exists for the candidate

## Requirements and architecture review

- [ ] Requirements baseline is identified and stable
- [ ] Traceability artifacts cover required critical requirements
- [ ] Architecture documents reflect the current implementation slice
- [ ] Known architectural gaps are recorded rather than implied away

## Verification evidence review

- [ ] Reliability artifacts are present and interpretable
- [ ] Benchmark artifacts match the current baseline thresholds
- [ ] Fuzz and static-analysis artifacts are attached or referenced
- [ ] Failed or flaky evidence is either resolved or explicitly dispositioned

## Security and vulnerability review

- [ ] Threat model revision is current for the candidate scope
- [ ] Vulnerability SLA report shows no undispositioned blocking items
- [ ] Dependency-scan and static-analysis outputs are reviewed

## Release process review

- [ ] Toolchain baseline and change-control policy were followed
- [ ] Release candidate is identified by tag and immutable commit
- [ ] Archived artifacts correspond to the tagged release candidate
- [ ] Rollback target and known risk list are recorded

## Findings

For each finding, record:

- ID
- Severity
- Summary
- Required action
- Owner
- Closure evidence

## Disposition

- Review outcome: `Accepted`, `Accepted with actions`, or `Rejected`
- Blocking findings count:
- Non-blocking findings count:
- Reviewer signature or approval record:
- Follow-up due date: