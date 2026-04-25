# Release Evidence Freeze Procedure

Purpose:

- define when a release candidate baseline is frozen
- prevent accidental drift between reviewed artifacts and the tagged source baseline
- describe the minimum steps to thaw and re-freeze a candidate when changes are required

## 1. Freeze trigger

Start a freeze when all of the following are true:

- the intended release commit is tagged
- release notes and scorecard are populated for the candidate
- planned quality workflows for the candidate are identified
- no known blocking code changes remain for the candidate scope

## 2. Freeze record

For each candidate, record at minimum:

- release tag
- source commit hash
- freeze timestamp
- person or role initiating freeze
- list of required artifacts to archive
- known limitations and accepted risks at freeze time

## 3. Frozen artifact set

The minimum frozen artifact set is:

- release scorecard
- release notes
- benchmark summary
- soak summary
- reliability-until-fail summary
- traceability gate summary and audit sample
- vulnerability SLA summary
- fuzz summary
- protocol compatibility matrix
- independent safety review checklist

## 4. Allowed actions during freeze

- archive artifacts
- obtain approvals
- prepare release publication metadata
- document findings that do not change the frozen baseline

## 5. Disallowed actions during freeze

- modify source code in the candidate baseline
- regenerate evidence from a different commit without updating the freeze record
- alter threshold or policy files without explicit thaw approval
- replace archived artifacts silently

## 6. Thaw conditions

Thaw the candidate if any of the following occurs:

- source code changes are required
- workflow definitions affecting evidence generation change
- blocking review finding requires corrected artifacts
- benchmark or reliability evidence is rerun from a different baseline

## 7. Thaw workflow

1. record the reason for thaw
2. identify whether the existing tag is superseded or must be replaced by a new candidate tag
3. regenerate all impacted artifacts
4. update release notes and scorecard if the evidence changed materially
5. run the freeze procedure again and archive the new artifact bundle

## 8. Freeze completion criteria

Freeze is considered complete only when:

- the archived artifact bundle matches the release tag
- approvals required by the release scorecard are attached
- the independent review checklist has a final disposition
- the release record includes rollback target and known limitations