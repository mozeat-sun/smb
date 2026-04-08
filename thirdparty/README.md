# Third-Party Dependencies

Canonical top-level location for vendored third-party dependencies.

## Inventory

| Component | Local path | License | Version signal | Upstream |
| --- | --- | --- | --- | --- |
| Unity | `thirdparty/unity` | MIT | unity.h reports 2.6.2 | https://github.com/ThrowTheSwitch/Unity |
| CMock | `thirdparty/CMock` | MIT | vendored snapshot (see repo history) | https://github.com/ThrowTheSwitch/CMock |

## Provenance and update policy

- Vendored dependencies must keep their original upstream LICENSE files.
- Dependency updates should be done as explicit commits with upstream reference
	(tag, commit, or release URL) in the commit message or PR description.
- Local modifications to vendored code should be avoided; if unavoidable,
	document rationale and patch scope in the updating PR.
- Security fixes in vendored dependencies should be prioritized and tracked.

Compatibility note:
- Existing build scripts currently use `thirdparty`.
- `thirdparty` is introduced to match standard repository layout.
- Migration can be completed incrementally.
