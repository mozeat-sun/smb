# Toolchain Baseline And Change Control

Purpose:

- define the controlled build and verification baseline for release candidates
- describe how toolchain, dependency, and process changes are approved
- provide the minimum change-control record expected for certification-oriented baselines

## 1. Controlled baseline scope

The controlled baseline for a release candidate includes:

- source revision identified by commit and release tag
- CMake configuration and repository build scripts
- compiler and linker family used by CI and release verification
- static-analysis, dependency-scan, fuzz, soak, and benchmark scripts in `tools/quality/`
- baseline threshold files under `config/`
- release governance documents under `docs/`

## 2. Baseline identification record

Every controlled release baseline must record:

- release tag
- source commit hash
- build host or runner class
- compiler family and version
- CMake version
- primary OS image or container identity
- dependency scan and static-analysis job identity

## 3. Toolchain baseline policy

Minimum repository policy:

1. release evidence must be generated from a tagged commit
2. CI workflow definitions are part of the controlled baseline
3. changes to compiler family, runner image, or core quality scripts require explicit review
4. benchmark threshold changes require updated rationale in the related baseline file and release notes when material
5. generated artifacts must be archived with enough metadata to reconstruct the environment

## 4. Change classes

### Class A: Documentation-only changes

Examples:

- release notes wording
- scorecard clarification
- audit checklist wording

Required control:

- normal peer review

### Class B: Process or governance changes

Examples:

- release workflow changes
- new required artifact types
- checklist or approval-path changes

Required control:

- peer review
- release owner acknowledgement when affecting release execution

### Class C: Toolchain or evidence-generation changes

Examples:

- CI runner image change
- compiler family/version change
- new benchmark methodology
- modified traceability, fuzz, or vulnerability-gate behavior

Required control:

- peer review
- quality gate owner approval
- rebaseline or comparison note if historical metrics may shift

### Class D: Certified-baseline-affecting changes

Examples:

- change after evidence freeze
- compatibility policy change on a pending release candidate
- dependency provenance or release artifact handling change

Required control:

- release authority approval
- documented rationale
- explicit decision whether prior evidence is invalidated and must be regenerated

## 5. Required change record fields

For Class C and Class D changes, record:

- change summary
- reason for change
- impacted artifacts or workflows
- approval names or roles
- whether benchmark or reliability evidence must be regenerated
- whether the release freeze must be reset

## 6. Branch and release control expectations

- protected branch policies should prevent unreviewed changes to release-bound commits
- release tags should point only to reviewed commits
- post-freeze changes require a new review decision and usually a new tag or release-candidate suffix

## 7. Minimum release-time checklist

- confirm release tag and commit
- confirm workflow definitions used for evidence generation
- confirm no unreviewed toolchain or script changes landed after freeze
- confirm archived artifacts match the tagged commit

## 8. Out-of-scope gaps

This policy is a repository skeleton. The following still need later-grade expansion:

- full dependency provenance and signed-build chain
- formal tool qualification plan
- long-term support branch process for certified baselines