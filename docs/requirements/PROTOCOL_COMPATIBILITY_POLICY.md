# Protocol Compatibility Policy

This document defines the bus-wide compatibility contract for ZOO wire protocol v1.

## Scope

- This policy governs SMB wire compatibility between independently built ZOO components.
- This policy does not override source-level or ABI compatibility expectations for public libraries; those remain governed by semantic versioning at release level.
- The current frozen protocol family is wire major version `1`.

## Version Header

Every protocol endpoint and handshake-capable transport must expose a protocol version header with these logical fields:

- `wire_major`: incompatible-on-change protocol family identifier.
- `wire_minor`: backward-compatible feature revision within one major family.

For protocol v1, the release baseline is `1.0`.

## Compatibility Rules

Wire compatibility is determined by the tuple `(wire_major, wire_minor)`.

Accepted combinations:

- Same major version and same minor version: accepted.
- Same major version and peer minor lower than local minor: accepted. The higher-minor side must degrade to the lower common minor behavior.

Blocked combinations:

- Different major versions: blocked.
- Same major version with peer minor higher than local minor: blocked until the local side supports that minor revision.
- Missing, malformed, or out-of-range version header values: blocked.

## Fail-Fast Behavior

When a mismatch is detected, the implementation must fail before normal message exchange begins.

Required behavior:

- Reject the connection or session establishment attempt.
- Emit a deterministic mismatch reason that includes both local and peer version tuples when available.
- Avoid partial protocol negotiation that would allow undefined message interpretation.
- Surface the failure through logs and quality artifacts so release validation can prove enforcement.

Recommended mismatch reason categories:

- `protocol-major-mismatch`
- `protocol-minor-too-new`
- `protocol-header-invalid`
- `protocol-header-missing`

## Compatibility Matrix

The authoritative CI-generated compatibility matrix artifact is produced by:

- `tools/quality/protocol_compatibility_matrix.sh`

Default output location:

- `artifacts/quality/protocol_compatibility_matrix.md`

The matrix must be generated for pull requests and preserved as a workflow artifact.

## API Versioning And Releases

- Public API releases follow semantic versioning.
- Patch releases must not change wire protocol semantics.
- Minor releases may add backward-compatible API surface and may increment wire minor only when older peers remain supported under the rules above.
- Major releases may introduce wire-major changes only with an explicit migration plan and release declaration.

## Deprecation Lifecycle

Deprecations affecting API or wire behavior must follow this lifecycle:

1. Declare the deprecation in release notes with affected components, user impact, and migration path.
2. Preserve backward-compatible behavior for at least one subsequent minor release unless a documented security or safety exception is approved.
3. Remove deprecated behavior only in a later release that explicitly marks the breaking change.

Required release-note statements are defined in `docs/templates/RELEASE_NOTES_TEMPLATE.md`.

## Release Governance

Each applicable release must declare:

- API compatibility status
- Wire protocol compatibility status
- Breaking changes
- Deprecation notices

Release evidence should include:

- the compatibility matrix artifact
- the completed release notes template
- any migration guidance required by a breaking change or deprecation
